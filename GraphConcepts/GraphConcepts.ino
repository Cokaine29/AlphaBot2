#include <Adafruit_NeoPixel.h>
#include "TRSensors.h"
#include <Wire.h>
#include <ArduinoBLE.h>

#define PWMA   6           //Left Motor Speed pin (ENA)
#define AIN2   A0          //Motor-L forward (IN2).
#define AIN1   A1          //Motor-L backward (IN1)
#define PWMB   5           //Right Motor Speed pin (ENB)
#define BIN1   A2          //Motor-R forward (IN3)
#define BIN2   A3          //Motor-R backward (IN4)
#define PIN 7
#define NUM_SENSORS 5
#define Addr  0x20

#define beep_on PCF8574Write(0xDF & PCF8574Read())
#define beep_off PCF8574Write(0x20 | PCF8574Read())

TRSensors trs = TRSensors();
unsigned int sensorValues[NUM_SENSORS];
unsigned int position;
uint16_t i, j;
byte value;
unsigned long lasttime = 0;
Adafruit_NeoPixel RGB = Adafruit_NeoPixel(4, PIN, NEO_GRB + NEO_KHZ800);

void PCF8574Write(byte data);
byte PCF8574Read();
uint32_t Wheel(byte WheelPos);
void SetSpeeds(int Aspeed,int Bspeed);
bool follow_segment();
void turn(unsigned char dir);
unsigned char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right);
void simplify_path();

// BLE Service & Characteristics Configuration (Nordic UART Service)
BLEService uartService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
BLECharacteristic rxCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite | BLEWriteWithoutResponse, 48);
BLECharacteristic txCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLENotify, 20);

// BLE Integration variables & functions
char cmdBuffer[48];
byte cmdIndex = 0;
volatile bool startCalibrate = false;
volatile bool startMaze = false;
volatile bool startSolve = false;

void parseAndExecuteCommand(char* command) {
  if (strstr(command, "\"Calibrate\":\"Start\"") != NULL) {
    startCalibrate = true;
  }
  else if (strstr(command, "\"Maze\":\"Learn\"") != NULL) {
    startMaze = true;
  }
  else if (strstr(command, "\"Maze\":\"Solve\"") != NULL) {
    startSolve = true;
  }
  else if (strstr(command, "\"BZ\":\"on\"") != NULL) {
    beep_on;
  }
  else if (strstr(command, "\"BZ\":\"off\"") != NULL) {
    beep_off;
  }
}

// Emulates BLE incoming buffer exactly like Serial
void checkBLE() {
  BLE.poll();
  if (rxCharacteristic.written()) {
    int len = rxCharacteristic.valueLength();
    const byte* val = rxCharacteristic.value();
    for (int i = 0; i < len; i++) {
      char c = (char)val[i];
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
}

// Helper to write serial data out to BLE NUS characteristic
void writeBLE(const char* data) {
  if (txCharacteristic.subscribed()) {
    txCharacteristic.writeValue((const uint8_t*)data, strlen(data));
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200); // USB Serial Monitor
  Wire.begin();
  
  pinMode(PWMA,OUTPUT);                     
  pinMode(AIN2,OUTPUT);      
  pinMode(AIN1,OUTPUT);
  pinMode(PWMB,OUTPUT);       
  pinMode(BIN1,OUTPUT);     
  pinMode(BIN2,OUTPUT);  
  SetSpeeds(0,0);
  
  RGB.begin();
  RGB.setPixelColor(0,0x00FF00 );
  RGB.setPixelColor(1,0x00FF00 );
  RGB.setPixelColor(2,0x00FF00 );
  RGB.setPixelColor(3,0x00FF00);
  RGB.show(); // Green LEDs indicating ready

  // Initialize built-in BLE
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1) {
      delay(1000);
    }
  }

  BLE.setLocalName("AlphaBot2-R4");
  BLE.setAdvertisedService(uartService);
  uartService.addCharacteristic(rxCharacteristic);
  uartService.addCharacteristic(txCharacteristic);
  BLE.addService(uartService);
  BLE.advertise();

  Serial.println("BLE Nordic UART Service advertised. Waiting for connection...");
  
  value = 0;
  startCalibrate = false;
  while(value != 0xEF && !startCalibrate)  //wait button pressed or BLE command
  {
    checkBLE();
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
    delay(10);
  }
  
  // Start Calibration
  RGB.setPixelColor(0,0x00FF00 );
  RGB.setPixelColor(1,0x00FF00 );
  RGB.setPixelColor(2,0x00FF00 );
  RGB.setPixelColor(3,0x00FF00);
  RGB.show(); 
  delay(500);

  for (int i = 0; i < 100; i++)
  {
    if(i<25 || i >= 75)
    {
      SetSpeeds(55,-55);
    }
    else
    {
        SetSpeeds(-55,55);
    }
    trs.calibrate();       // reads all sensors 100 times
  }
  SetSpeeds(0,0); 
  
  // Calibration Done (Turn LEDs blue)
  RGB.setPixelColor(0,0x0000FF );
  RGB.setPixelColor(1,0x0000FF );
  RGB.setPixelColor(2,0x0000FF );
  RGB.setPixelColor(3,0x0000FF);
  RGB.show();
  
  value = 0;
  startMaze = false;
  while(value != 0xEF && !startMaze)  //wait button pressed or BLE command
  {
    checkBLE();
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
    delay(10);
  }

  // Go! (Turn LEDs green)
  RGB.setPixelColor(0,0x00FF00 );
  RGB.setPixelColor(1,0x00FF00 );
  RGB.setPixelColor(2,0x00FF00 );
  RGB.setPixelColor(3,0x00FF00);
  RGB.show();
  delay(500);
}

bool follow_segment()
{
  int last_proportional = 0;
  long integral=0;

  while(1)
  {
    checkBLE(); // Ensure we poll BLE and handle commands during tracking
    unsigned int position = trs.readLine(sensorValues);
    int proportional = ((int)position) - 2000;
    int derivative = proportional - last_proportional;
    integral += proportional;
    last_proportional = proportional;

    int power_difference = proportional/20 + integral/10000 + derivative*10;

    const int maximum = 80; // the maximum speed (matched to MazeSolver.ino)
    if (power_difference > maximum)
      power_difference = maximum;
    if (power_difference < -maximum)
      power_difference = - maximum;

    if (power_difference < 0)
    {
      analogWrite(PWMA,maximum + power_difference);
      analogWrite(PWMB,maximum);
    }
    else
    {
      analogWrite(PWMA,maximum);
      analogWrite(PWMB,maximum - power_difference);
    }

   if(millis() - lasttime >100)
   {
    if (sensorValues[1] < 150 && sensorValues[2] < 150 && sensorValues[3] < 150)
    {
      return false; // dead end
    }
    else if (sensorValues[0] > 600 || sensorValues[4] > 600)
    {
      return true; // intersection
    }
   }
  }
}

void turn(unsigned char dir)
{
  switch(dir)
  {
    case 'L':
      SetSpeeds(-100, 100);
      delay(150); // blind turn to get off current line
      while(1)
      {
        trs.readLine(sensorValues);
        if(sensorValues[2] > 500 || sensorValues[1] > 500 || sensorValues[3] > 500)
          break;
      }
      break;
    case 'R':
      SetSpeeds(100, -100);
      delay(150); // blind turn to get off current line
      while(1)
      {
        trs.readLine(sensorValues);
        if(sensorValues[2] > 500 || sensorValues[1] > 500 || sensorValues[3] > 500)
          break;
      }
      break;
    case 'B':
      SetSpeeds(100, -100);
      delay(250); // blind turn to sweep past the end of the line
      while(1)
      {
        trs.readLine(sensorValues);
        if(sensorValues[2] > 500 || sensorValues[1] > 500 || sensorValues[3] > 500)
          break;
      }
      break;
    case 'S':
      break;
  }
  SetSpeeds(0, 0);
  Serial.write(dir);
  Serial.println();
  
  // Send the turn direction over BLE NUS
  char dirStr[3] = {(char)dir, '\n', '\0'};
  writeBLE(dirStr);

  lasttime = millis();   
}

unsigned char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right)
{
  if (found_left)
    return 'L';
  else if (found_straight)
    return 'S';
  else if (found_right)
    return 'R';
  else
    return 'B';
}

char path[100] = "";
unsigned char path_length = 0;

void simplify_path()
{
  if (path_length < 3 || path[path_length-2] != 'B')
    return;

  int total_angle = 0;
  int i;
  for (i = 1; i <= 3; i++)
  {
    switch (path[path_length - i])
    {
    case 'R':
      total_angle += 90;
      break;
    case 'L':
      total_angle += 270;
      break;
    case 'B':
      total_angle += 180;
      break;
    }
  }

  total_angle = total_angle % 360;

  switch (total_angle)
  {
  case 0:
    path[path_length - 3] = 'S';
    break;
  case 90:
    path[path_length - 3] = 'R';
    break;
  case 180:
    path[path_length - 3] = 'B';
    break;
  case 270:
    path[path_length - 3] = 'L';
    break;
  }

  path_length -= 2;
}

void loop() {
  while (1)
  {
    if (!follow_segment())
    {
      // Hit a dead end! Stop and back up slightly
      SetSpeeds(0, 0);
      delay(50);
      SetSpeeds(-70, -70);
      delay(180);
      SetSpeeds(0, 0);
      delay(50);

      // Turn around (U-turn)
      turn('B');

      // Store 'B' in the path
      path[path_length] = 'B';
      path_length++;
      simplify_path();
      
      continue;
    }

    // Drive straight a bit.
    SetSpeeds(30, 30);
    delay(40);

    unsigned char found_left = 0;
    unsigned char found_straight = 0;
    unsigned char found_right = 0;

    trs.readLine(sensorValues);

    if (sensorValues[0] > 600)
      found_left = 1;
    if (sensorValues[4] > 600)
      found_right = 1;

    SetSpeeds(30, 30);
    delay(100);

    trs.readLine(sensorValues);
    if (sensorValues[1] > 600 || sensorValues[2] > 600 || sensorValues[3] > 600)
      found_straight = 1;

    if (sensorValues[1] > 600 && sensorValues[2] > 600 && sensorValues[3] > 600)
    {
      SetSpeeds(0, 0);
      break;
    }

    unsigned char dir = select_turn(found_left, found_straight, found_right);
    turn(dir);

    path[path_length] = dir;
    path_length++;
    simplify_path();
  }

  // Solved the maze! (Flash LEDs Blue/Green to indicate success)
  while (1)
  {
    SetSpeeds(0, 0);
    Serial.println("End !!!");
    writeBLE("End !!!\n");
    
    // Cycle LED colors
    RGB.setPixelColor(0,0x0000FF);
    RGB.setPixelColor(1,0x00FF00);
    RGB.setPixelColor(2,0x0000FF);
    RGB.setPixelColor(3,0x00FF00);
    RGB.show();
    delay(250);
    RGB.setPixelColor(0,0x00FF00);
    RGB.setPixelColor(1,0x0000FF);
    RGB.setPixelColor(2,0x00FF00);
    RGB.setPixelColor(3,0x0000FF);
    RGB.show();
    delay(250);

    value = 0;
    startSolve = false;
    while(value != 0xEF && !startSolve)  //wait button pressed or BLE command
    {
      checkBLE();
      PCF8574Write(0x1F | PCF8574Read());
      value = PCF8574Read() | 0xE0;
      delay(10);
    }
    delay(1000);

    // Green lights for solve re-run
    RGB.setPixelColor(0,0x00FF00 );
    RGB.setPixelColor(1,0x00FF00 );
    RGB.setPixelColor(2,0x00FF00 );
    RGB.setPixelColor(3,0x00FF00);
    RGB.show();

    int i;
    for (i = 0; i < path_length; i++)
    {
      follow_segment();
      SetSpeeds(30, 30);
      delay(40);
      SetSpeeds(30, 30);
      delay(150);
      turn(path[i]);
    }
    follow_segment();
  }
}

void SetSpeeds(int Aspeed,int Bspeed)
{
  if(Aspeed < 0)
  {
    digitalWrite(AIN1,HIGH);
    digitalWrite(AIN2,LOW);
    analogWrite(PWMA,-Aspeed);      
  }
  else
  {
    digitalWrite(AIN1,LOW); 
    digitalWrite(AIN2,HIGH);
    analogWrite(PWMA,Aspeed);  
  }
  
  if(Bspeed < 0)
  {
    digitalWrite(BIN1,HIGH);
    digitalWrite(BIN2,LOW);
    analogWrite(PWMB,-Bspeed);      
  }
  else
  {
    digitalWrite(BIN1,LOW); 
    digitalWrite(BIN2,HIGH);
    analogWrite(PWMB,Bspeed);  
  }
}

void PCF8574Write(byte data)
{
  Wire.beginTransmission(Addr);
  Wire.write(data);
  Wire.endTransmission(); 
}

byte PCF8574Read()
{
  int data = -1;
  Wire.requestFrom(Addr, 1);
  if(Wire.available()) {
    data = Wire.read();
  }
  return data;
}
