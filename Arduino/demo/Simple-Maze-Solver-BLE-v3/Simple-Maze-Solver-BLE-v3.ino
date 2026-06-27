#include <Adafruit_NeoPixel.h>
#include "TRSensors.h"
#include <Wire.h>

#define PWMA   6
#define AIN2   A0
#define AIN1   A1
#define PWMB   5
#define BIN1   A2
#define BIN2   A3
#define PIN 7
#define NUM_SENSORS 5
#define Addr  0x20

TRSensors trs = TRSensors();
unsigned int sensorValues[NUM_SENSORS];
byte value;
unsigned long lasttime = 0;
Adafruit_NeoPixel RGB = Adafruit_NeoPixel(4, PIN, NEO_GRB + NEO_KHZ800);

void PCF8574Write(byte data);
byte PCF8574Read();
void SetSpeeds(int Aspeed,int Bspeed);
bool follow_segment();
void turn(unsigned char dir);
unsigned char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right);

// BLE Integration variables & functions
char cmdBuffer[48];
byte cmdIndex = 0;
volatile bool startCalibrate = false;
volatile bool startMaze = false;
volatile bool startSolve = false;

void parseAndExecuteCommand(char* command) {
  if (strstr(command, "\"Calibrate\":\"Start\"") != NULL) startCalibrate = true;
  else if (strstr(command, "\"Maze\":\"Learn\"") != NULL) startMaze = true;
  else if (strstr(command, "\"Maze\":\"Solve\"") != NULL) startSolve = true;
  else if (strstr(command, "\"BZ\":\"on\"") != NULL) PCF8574Write(0xDF & PCF8574Read());
  else if (strstr(command, "\"BZ\":\"off\"") != NULL) PCF8574Write(0x20 | PCF8574Read());
}

void checkSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '{') {
      cmdIndex = 0;
      cmdBuffer[cmdIndex++] = c;
    } else if (cmdIndex > 0) {
      if (cmdIndex < 47) {
        cmdBuffer[cmdIndex++] = c;
        if (c == '}') {
          cmdBuffer[cmdIndex] = '\0';
          parseAndExecuteCommand(cmdBuffer);
          cmdIndex = 0;
        }
      } else {
        cmdIndex = 0;
      }
    }
  }
}

// GRAPH ENGINE
#define MAX_NODES 40

struct Node {
  int8_t x, y;
  uint8_t edges[4]; // 0=N, 1=E, 2=S, 3=W
};

Node nodes[MAX_NODES];
uint8_t num_nodes = 0;
uint8_t prev_node = 0;
uint8_t current_dir = 0; // 0=N, 1=E, 2=S, 3=W
int8_t current_x = 0;
int8_t current_y = 0;

uint8_t get_node_at(int8_t x, int8_t y) {
  for (uint8_t i = 0; i < num_nodes; i++) {
    if (nodes[i].x == x && nodes[i].y == y) return i;
  }
  return 255;
}

uint8_t add_node(int8_t x, int8_t y) {
  if (num_nodes >= MAX_NODES) return 255;
  nodes[num_nodes].x = x;
  nodes[num_nodes].y = y;
  for (int i=0; i<4; i++) nodes[num_nodes].edges[i] = 255;
  
  Serial.print("NODE:"); Serial.print(num_nodes); Serial.print(","); 
  Serial.print(x); Serial.print(","); Serial.println(y);
  delay(10);
  
  num_nodes++;
  return num_nodes - 1;
}

void add_edge(uint8_t n1, uint8_t n2, uint8_t dir_from_n1) {
  if (n1 == 255 || n2 == 255) return;
  if (nodes[n1].edges[dir_from_n1] == n2) return; // Already linked
  
  nodes[n1].edges[dir_from_n1] = n2;
  nodes[n2].edges[(dir_from_n1 + 2) % 4] = n1;
  
  Serial.print("EDGE:"); Serial.print(n1); Serial.print(","); Serial.println(n2);
  delay(10);
}

void init_graph() {
  num_nodes = 0;
  current_dir = 0;
  current_x = 0;
  current_y = 0;
  prev_node = add_node(0, 0);
  Serial.print("BOT:"); Serial.println(prev_node);
}

uint8_t path_nodes[MAX_NODES];
uint8_t path_len = 0;

void run_dijkstra(uint8_t start, uint8_t target) {
  uint8_t dist[MAX_NODES];
  uint8_t prev[MAX_NODES];
  bool visited[MAX_NODES];
  
  for(int i=0; i<num_nodes; i++) {
    dist[i] = 255;
    prev[i] = 255;
    visited[i] = false;
  }
  
  dist[start] = 0;
  
  for(int i=0; i<num_nodes; i++) {
    uint8_t u = 255;
    uint8_t min_dist = 255;
    for(int j=0; j<num_nodes; j++) {
      if(!visited[j] && dist[j] < min_dist) {
        min_dist = dist[j];
        u = j;
      }
    }
    
    if(u == 255 || u == target) break;
    visited[u] = true;
    
    for(int d=0; d<4; d++) {
      uint8_t v = nodes[u].edges[d];
      if(v != 255 && !visited[v]) {
        if(dist[u] + 1 < dist[v]) {
          dist[v] = dist[u] + 1;
          prev[v] = u;
        }
      }
    }
  }
  
  path_len = 0;
  uint8_t curr = target;
  while(curr != 255 && curr != start) {
    path_nodes[path_len++] = curr;
    curr = prev[curr];
  }
  path_nodes[path_len++] = start;
  
  for(int i=0; i<path_len/2; i++) {
    uint8_t temp = path_nodes[i];
    path_nodes[i] = path_nodes[path_len-1-i];
    path_nodes[path_len-1-i] = temp;
  }
  
  Serial.print("SOLVE:");
  for(int i=0; i<path_len; i++) {
    Serial.print(path_nodes[i]);
    if(i < path_len-1) Serial.print(",");
  }
  Serial.println();
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  Wire.begin();
  pinMode(PWMA,OUTPUT); pinMode(AIN2,OUTPUT); pinMode(AIN1,OUTPUT);
  pinMode(PWMB,OUTPUT); pinMode(BIN1,OUTPUT); pinMode(BIN2,OUTPUT);  
  SetSpeeds(0,0);
  
  RGB.begin();
  for(int i=0;i<4;i++) RGB.setPixelColor(i,0x00FF00);
  RGB.show();
  
  value = 0; startCalibrate = false;
  while(value != 0xEF && !startCalibrate) {
    checkSerial();
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
    delay(10);
  }
  
  for(int i=0;i<4;i++) RGB.setPixelColor(i,0x00FF00);
  RGB.show(); delay(500);

  for (int i = 0; i < 100; i++) {
    if(i<25 || i >= 75) SetSpeeds(55,-55);
    else SetSpeeds(-55,55);
    trs.calibrate();
  }
  SetSpeeds(0,0); 
  
  for(int i=0;i<4;i++) RGB.setPixelColor(i,0x0000FF);
  RGB.show();
  
  value = 0; startMaze = false;
  while(value != 0xEF && !startMaze) {
    checkSerial();
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
    delay(10);
  }

  for(int i=0;i<4;i++) RGB.setPixelColor(i,0x00FF00);
  RGB.show(); delay(500);
}

bool follow_segment() {
  int last_proportional = 0;
  long integral=0;

  while(1) {
    unsigned int position = trs.readLine(sensorValues);
    int proportional = ((int)position) - 2000;
    int derivative = proportional - last_proportional;
    integral += proportional;
    last_proportional = proportional;

    int power_difference = proportional/20 + integral/10000 + derivative*10;
    const int maximum = 80;
    if (power_difference > maximum) power_difference = maximum;
    if (power_difference < -maximum) power_difference = - maximum;

    if (power_difference < 0) {
      analogWrite(PWMA,maximum + power_difference);
      analogWrite(PWMB,maximum);
    } else {
      analogWrite(PWMA,maximum);
      analogWrite(PWMB,maximum - power_difference);
    }

   if(millis() - lasttime >100) {
    if (sensorValues[1] < 150 && sensorValues[2] < 150 && sensorValues[3] < 150) return false;
    else if (sensorValues[0] > 600 || sensorValues[4] > 600) return true;
   }
  }
}

void turn(unsigned char dir) {
  switch(dir) {
    case 'L':
      SetSpeeds(-100, 100); delay(150);
      while(1) { trs.readLine(sensorValues); if(sensorValues[2]>500||sensorValues[1]>500||sensorValues[3]>500) break; }
      break;
    case 'R':
      SetSpeeds(100, -100); delay(150);
      while(1) { trs.readLine(sensorValues); if(sensorValues[2]>500||sensorValues[1]>500||sensorValues[3]>500) break; }
      break;
    case 'B':
      SetSpeeds(100, -100); delay(250);
      while(1) { trs.readLine(sensorValues); if(sensorValues[2]>500||sensorValues[1]>500||sensorValues[3]>500) break; }
      break;
    case 'S': break;
  }
  SetSpeeds(0, 0);
  Serial.write(dir); Serial.println();
  lasttime = millis();   
}

unsigned char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right) {
  if (found_left) return 'L';
  else if (found_straight) return 'S';
  else if (found_right) return 'R';
  else return 'B';
}

void process_arrival() {
  int8_t dx = 0, dy = 0;
  if (current_dir == 0) dy = 1;
  else if (current_dir == 1) dx = 1;
  else if (current_dir == 2) dy = -1;
  else if (current_dir == 3) dx = -1;
  
  current_x += dx;
  current_y += dy;
  
  uint8_t arrived_node = get_node_at(current_x, current_y);
  if (arrived_node == 255) {
    arrived_node = add_node(current_x, current_y);
  }
  
  add_edge(prev_node, arrived_node, current_dir);
  prev_node = arrived_node;
  Serial.print("BOT:"); Serial.println(prev_node);
}

void loop() {
  init_graph();

  while (1) {
    bool hit_intersection = follow_segment();
    process_arrival();

    if (!hit_intersection) {
      // Exhausted map detection:
      // If we hit a dead end AND our current coordinate is the Start Node (Node 0),
      // it means we have completely backed out of the maze and hit the wall behind the starting line.
      if (prev_node == 0) {
        SetSpeeds(0, 0);
        break;
      }

      SetSpeeds(0, 0); delay(50);
      SetSpeeds(-70, -70); delay(180);
      SetSpeeds(0, 0); delay(50);

      turn('B');
      current_dir = (current_dir + 2) % 4;
      lasttime = millis();
      continue;
    }

    SetSpeeds(30, 30); delay(40);

    unsigned char found_left = 0, found_straight = 0, found_right = 0;
    trs.readLine(sensorValues);
    if (sensorValues[0] > 600) found_left = 1;
    if (sensorValues[4] > 600) found_right = 1;

    SetSpeeds(30, 30); delay(100);

    trs.readLine(sensorValues);
    if (sensorValues[1] > 600 || sensorValues[2] > 600 || sensorValues[3] > 600) found_straight = 1;

    if (sensorValues[1] > 600 && sensorValues[2] > 600 && sensorValues[3] > 600) {
      SetSpeeds(0, 0);
      break;
    }

    unsigned char dir = select_turn(found_left, found_straight, found_right);
    turn(dir);
    
    if (dir == 'L') current_dir = (current_dir + 3) % 4;
    else if (dir == 'R') current_dir = (current_dir + 1) % 4;

    lasttime = millis();
  }

  bool foundEnd = (sensorValues[1] > 600 && sensorValues[2] > 600 && sensorValues[3] > 600);

  if (foundEnd) {
    run_dijkstra(0, prev_node);
    
    while (1) {
      SetSpeeds(0, 0);
      Serial.println("End !!!");
      
      for(int i=0; i<4; i++) RGB.setPixelColor(i,0x0000FF);
      RGB.show(); delay(250);
      for(int i=0; i<4; i++) RGB.setPixelColor(i,0x00FF00);
      RGB.show(); delay(250);

      value = 0; startSolve = false;
      while(value != 0xEF && !startSolve) {
        checkSerial();
        PCF8574Write(0x1F | PCF8574Read());
        value = PCF8574Read() | 0xE0;
        delay(10);
      }
      delay(1000);

      for(int i=0; i<4; i++) RGB.setPixelColor(i,0x00FF00);
      RGB.show();

      current_dir = 0;
      prev_node = 0;
      follow_segment();
      prev_node = path_nodes[1];
      
      for (int i = 1; i < path_len - 1; i++) {
        uint8_t curr = path_nodes[i];
        uint8_t target = path_nodes[i+1];
        
        uint8_t d = 0;
        for (d = 0; d < 4; d++) {
          if (nodes[curr].edges[d] == target) break;
        }
        
        int turn_diff = (d - current_dir + 4) % 4;
        unsigned char turn_char = 'S';
        if (turn_diff == 1) turn_char = 'R';
        else if (turn_diff == 2) turn_char = 'B';
        else if (turn_diff == 3) turn_char = 'L';
        
        SetSpeeds(30, 30); delay(40);
        SetSpeeds(30, 30); delay(150);
        
        if (turn_char != 'S') turn(turn_char);
        
        follow_segment();
        current_dir = d;
      }
    }
  } else {
    while (1) {
      SetSpeeds(0, 0);
      Serial.println("No End Found!");
      
      for(int i=0; i<4; i++) RGB.setPixelColor(i,0xFF0000);
      RGB.show(); delay(250);
      RGB.clear(); RGB.show(); delay(250);

      value = 0; startMaze = false;
      while(value != 0xEF && !startMaze) {
        checkSerial();
        PCF8574Write(0x1F | PCF8574Read());
        value = PCF8574Read() | 0xE0;
        delay(10);
      }
      delay(1000);
      return; 
    }
  }
}

void SetSpeeds(int Aspeed,int Bspeed) {
  if(Aspeed < 0) { digitalWrite(AIN1,HIGH); digitalWrite(AIN2,LOW); analogWrite(PWMA,-Aspeed); }
  else { digitalWrite(AIN1,LOW); digitalWrite(AIN2,HIGH); analogWrite(PWMA,Aspeed); }
  if(Bspeed < 0) { digitalWrite(BIN1,HIGH); digitalWrite(BIN2,LOW); analogWrite(PWMB,-Bspeed); }
  else { digitalWrite(BIN1,LOW); digitalWrite(BIN2,HIGH); analogWrite(PWMB,Bspeed); }
}

void PCF8574Write(byte data) { Wire.beginTransmission(Addr); Wire.write(data); Wire.endTransmission(); }
byte PCF8574Read() { int data = -1; Wire.requestFrom(Addr, 1); if(Wire.available()) data = Wire.read(); return data; }
