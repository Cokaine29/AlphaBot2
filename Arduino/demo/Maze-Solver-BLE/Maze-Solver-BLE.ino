#include "TRSensors.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Motor-L forward (IN2)
#define AIN1 A1 // Motor-L backward (IN1)
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Motor-R forward (IN3)
#define BIN2 A3 // Motor-R backward (IN4)
#define PIN 7   // RGB NeoPixel Pin
#define NUM_SENSORS 5
#define Addr 0x20 // PCF8574 Address

#define beep_on PCF8574Write(0xDF & PCF8574Read())
#define beep_off PCF8574Write(0x20 | PCF8574Read())

TRSensors trs = TRSensors();
Adafruit_NeoPixel RGB = Adafruit_NeoPixel(4, PIN, NEO_GRB + NEO_KHZ800);

// Speed offsets to calibrate wheel imbalance
#define LEFT_SPEED_OFFSET 0
#define RIGHT_SPEED_OFFSET 0

enum RobotMode {
  MODE_STOPPED,
  MODE_CALIBRATING,
  MODE_LEARNING,
  MODE_SOLVING
};

RobotMode currentMode = MODE_STOPPED;
bool isCalibrated = false;

unsigned int sensorValues[NUM_SENSORS];
int Speed = 150;        // Speed for manual override driving
const int trackingSpeed = 100; // Standard line-following speed limit

// Path planning storage
char raw_path[100] = "";
unsigned char raw_path_length = 0;
char path[100] = "";
unsigned char path_length = 0;

// Command parser buffers
char cmdBuffer[48];
byte cmdIndex = 0;

// Function Prototypes
void PCF8574Write(byte data);
byte PCF8574Read();
void SetSpeeds(int Aspeed, int Bspeed);
void forward();
void backward();
void right();
void left();
void stop();
void beep(int duration);
void runCalibration();
void learnMaze();
void solveMaze();
bool follow_segment();
bool execute_turn(unsigned char dir);
unsigned char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right);
void simplify_path();
void parseAndExecuteCommand(char* command);
void checkSerial();
void checkJoystick();
void sendLiveTelemetry(unsigned int* calValues, unsigned int position);
void sendStateTelemetry();
void sendSolveStepTelemetry(int stepIndex);
void playIdleAnimation();
uint32_t Wheel(byte WheelPos);

void setup() {
  Serial.begin(9600); // Standard HM-10 / BLE baud rate
  Wire.begin();
  
  RGB.begin();
  RGB.setPixelColor(0, 0x00FF00); // Green indicating ready
  RGB.setPixelColor(1, 0x00FF00);
  RGB.setPixelColor(2, 0x00FF00);
  RGB.setPixelColor(3, 0x00FF00);
  RGB.show();
  delay(1500);

  // Configure Motor Pins
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT); 
  pinMode(BIN2, OUTPUT);
  
  stop();
  sendStateTelemetry();
}

void loop() {
  checkSerial();
  checkJoystick();

  if (currentMode == MODE_LEARNING) {
    learnMaze();
  }
  else if (currentMode == MODE_SOLVING) {
    solveMaze();
  }
  else {
    // Mode is Stopped
    static unsigned long lastTelemetryTime = 0;
    if (millis() - lastTelemetryTime > 1000) {
      lastTelemetryTime = millis();
      sendStateTelemetry();
    }

    playIdleAnimation();
  }
}

// Runs physical joystick inputs (debounced check)
void checkJoystick() {
  static unsigned long lastJoystickCheck = 0;
  if (millis() - lastJoystickCheck > 200) {
    lastJoystickCheck = millis();
    PCF8574Write(0x1F | PCF8574Read());
    byte value = PCF8574Read() | 0xE0;
    
    if (value == 0xEF) { // Center button pressed
      beep(150);
      if (!isCalibrated) {
        runCalibration();
      } else {
        if (currentMode == MODE_STOPPED) {
          raw_path_length = 0;
          raw_path[0] = '\0';
          path_length = 0;
          path[0] = '\0';
          currentMode = MODE_LEARNING;
        } else {
          currentMode = MODE_STOPPED;
          stop();
        }
      }
      // Wait for release
      while (value == 0xEF) {
        PCF8574Write(0x1F | PCF8574Read());
        value = PCF8574Read() | 0xE0;
        delay(10);
      }
    }
  }
}

// Learn Maze exploration routine (Left-hand-on-the-wall)
void learnMaze() {
  Serial.println(F("{\"State\":\"Learning\",\"Info\":\"Starting exploration...\"}"));
  
  // NeoPixel headlights solid orange indicating autonomous run
  for (int i = 0; i < 4; i++) {
    RGB.setPixelColor(i, 0xFF8C00); // Dark Orange
  }
  RGB.show();

  while (currentMode == MODE_LEARNING) {
    // 1. Follow line segment until a junction, dead end, or endpoint is hit
    if (!follow_segment()) {
      break;
    }

    // 2. Drive straight a bit to center the robot over the intersection line
    SetSpeeds(30, 30);
    unsigned long start_delay = millis();
    while (millis() - start_delay < 40) {
      checkSerial();
      if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return; }
      delay(5);
    }

    // 3. Scan sensors to check for left/right branches
    unsigned int calValues[NUM_SENSORS];
    trs.readLine(sensorValues);
    trs.readCalibrated(calValues);

    unsigned char found_left = 0;
    unsigned char found_straight = 0;
    unsigned char found_right = 0;

    if (calValues[0] > 600) found_left = 1;
    if (calValues[4] > 600) found_right = 1;

    // 4. Drive straight a bit more to align wheels with the intersection axis
    SetSpeeds(30, 30);
    start_delay = millis();
    while (millis() - start_delay < 100) {
      checkSerial();
      if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return; }
      delay(5);
    }

    // 5. Scan middle sensors to check if a straight path exists
    trs.readLine(sensorValues);
    trs.readCalibrated(calValues);

    if (calValues[1] > 600 || calValues[2] > 600 || calValues[3] > 600) {
      found_straight = 1;
    }

    // 6. Check for ending spot (all 3 middle sensors dark black / target)
    if (calValues[1] > 600 && calValues[2] > 600 && calValues[3] > 600) {
      SetSpeeds(0, 0);
      currentMode = MODE_STOPPED;
      beep(150);
      delay(100);
      beep(150);
      sendStateTelemetry(); // Sends finalized solved state
      break;
    }

    // 7. Make turn decision using left-hand-on-the-wall rule
    unsigned char dir = select_turn(found_left, found_straight, found_right);

    // 8. Execute the turn
    if (!execute_turn(dir)) {
      break;
    }

    // 9. Record and simplify path
    if (raw_path_length < 99) {
      raw_path[raw_path_length] = dir;
      raw_path_length++;
      raw_path[raw_path_length] = '\0';
    }
    if (path_length < 99) {
      path[path_length] = dir;
      path_length++;
      path[path_length] = '\0';
      simplify_path();
    }

    // 10. Send live path telemetry
    sendStateTelemetry();
  }

  // Set back to Stopped
  currentMode = MODE_STOPPED;
  stop();
}

// Traverses maze using the stored simplified path
void solveMaze() {
  Serial.println(F("{\"State\":\"Solving\",\"Info\":\"Starting short-path traversal...\"}"));

  if (path_length == 0) {
    currentMode = MODE_STOPPED;
    Serial.println(F("{\"State\":\"Calibrated\",\"Info\":\"No path stored!\"}"));
    return;
  }

  // NeoPixel headlights solid green indicating solving run
  for (int i = 0; i < 4; i++) {
    RGB.setPixelColor(i, 0x00FF00); 
  }
  RGB.show();

  for (int i = 0; i < path_length; i++) {
    if (currentMode != MODE_SOLVING) break;

    // Send active step index
    sendSolveStepTelemetry(i);

    // 1. Follow line segment
    if (!follow_segment()) {
      break;
    }

    // 2. Drive straight slightly
    SetSpeeds(30, 30);
    unsigned long start_delay = millis();
    while (millis() - start_delay < 40) {
      checkSerial();
      if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return; }
      delay(5);
    }

    // 3. Align wheels
    SetSpeeds(30, 30);
    start_delay = millis();
    while (millis() - start_delay < 150) {
      checkSerial();
      if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return; }
      delay(5);
    }

    // 4. Turn according to the pre-calculated step
    if (!execute_turn(path[i])) {
      break;
    }
  }

  if (currentMode == MODE_SOLVING) {
    // Follow the final segment up to the finish line
    sendSolveStepTelemetry(path_length);
    if (follow_segment()) {
      SetSpeeds(0, 0);
      currentMode = MODE_STOPPED;
      beep(150);
      delay(100);
      beep(150);
      Serial.println(F("{\"State\":\"Calibrated\",\"Info\":\"Maze Solved!\"}"));
    }
  }

  currentMode = MODE_STOPPED;
  stop();
}

// Follow line segment until a junction or dead end is reached
bool follow_segment() {
  int last_proportional = 0;
  long integral = 0;
  unsigned long last_telemetry = 0;
  unsigned long entry_time = millis();

  // Reset direction pins
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  while (currentMode == MODE_LEARNING || currentMode == MODE_SOLVING) {
    checkSerial();
    if (currentMode == MODE_STOPPED) {
      SetSpeeds(0, 0);
      return false;
    }

    unsigned int position = trs.readLine(sensorValues);
    unsigned int calValues[NUM_SENSORS];
    trs.readCalibrated(calValues);

    // Telemetry output to dashboard (250ms interval)
    if (millis() - last_telemetry > 250) {
      last_telemetry = millis();
      sendLiveTelemetry(calValues, position);
    }

    // PID Steering offset calculations
    int proportional = (int)position - 2000;
    int derivative = proportional - last_proportional;
    integral += proportional;
    last_proportional = proportional;

    int power_difference = proportional / 20 + integral / 10000 + derivative * 10;

    if (power_difference > trackingSpeed) power_difference = trackingSpeed;
    if (power_difference < -trackingSpeed) power_difference = -trackingSpeed;

    if (power_difference < 0) {
      SetSpeeds(trackingSpeed + power_difference, trackingSpeed);
    } else {
      SetSpeeds(trackingSpeed, trackingSpeed - power_difference);
    }

    // Safe threshold delays to clear the line we just turned off
    if (millis() - entry_time > 150) {
      // Dead End detection (All center sensors see white/reflective background)
      if (calValues[1] < 150 && calValues[2] < 150 && calValues[3] < 150) {
        return true;
      }
      // Left/Right branch line detection
      if (calValues[0] > 600 || calValues[4] > 600) {
        return true;
      }
    }

    delay(2);
  }

  SetSpeeds(0, 0);
  return false;
}

// Lock-on Turn sequence using center line sensor (S2)
bool execute_turn(unsigned char dir) {
  unsigned int calValues[NUM_SENSORS];
  int turnSpeed = 95;

  switch(dir) {
    case 'L':
      SetSpeeds(-turnSpeed, turnSpeed);
      // Blind turn for 200ms to spin off current line
      {
        unsigned long start = millis();
        while (millis() - start < 200) {
          checkSerial();
          if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return false; }
          delay(5);
        }
      }
      // Spin until S2 detects black line
      while (true) {
        checkSerial();
        if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return false; }
        trs.readCalibrated(calValues);
        if (calValues[2] > 700) break;
        delay(5);
      }
      break;

    case 'R':
      SetSpeeds(turnSpeed, -turnSpeed);
      // Blind turn for 200ms
      {
        unsigned long start = millis();
        while (millis() - start < 200) {
          checkSerial();
          if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return false; }
          delay(5);
        }
      }
      // Spin until S2 detects line
      while (true) {
        checkSerial();
        if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return false; }
        trs.readCalibrated(calValues);
        if (calValues[2] > 700) break;
        delay(5);
      }
      break;

    case 'B':
      SetSpeeds(turnSpeed, -turnSpeed);
      // Blind turn for 400ms to clear line backwards
      {
        unsigned long start = millis();
        while (millis() - start < 400) {
          checkSerial();
          if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return false; }
          delay(5);
        }
      }
      // Spin until S2 detects line
      while (true) {
        checkSerial();
        if (currentMode == MODE_STOPPED) { SetSpeeds(0,0); return false; }
        trs.readCalibrated(calValues);
        if (calValues[2] > 700) break;
        delay(5);
      }
      break;

    case 'S':
      // Straight: skip rotation, let line tracking pick up line
      break;
  }

  SetSpeeds(0, 0);
  delay(100); // Allow physical momentum to damp down
  return true;
}

// Left-hand-on-the-wall decision priority
unsigned char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right) {
  if (found_left)
    return 'L';
  else if (found_straight)
    return 'S';
  else if (found_right)
    return 'R';
  else
    return 'B';
}

// Path simplification algorithm (removes dead end backtracks: xBx)
void simplify_path() {
  if (path_length < 3 || path[path_length-2] != 'B')
    return;

  int total_angle = 0;
  for (int i = 1; i <= 3; i++) {
    switch (path[path_length - i]) {
      case 'R': total_angle += 90; break;
      case 'L': total_angle += 270; break;
      case 'B': total_angle += 180; break;
    }
  }

  total_angle = total_angle % 360;

  switch (total_angle) {
    case 0:   path[path_length - 3] = 'S'; break;
    case 90:  path[path_length - 3] = 'R'; break;
    case 180: path[path_length - 3] = 'B'; break;
    case 270: path[path_length - 3] = 'L'; break;
  }

  path_length -= 2;
  path[path_length] = '\0';
}

// Receives, buffers, and executes incoming BLE packages
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

// Executes incoming D-Pad, state triggers, utility and calibration codes
void parseAndExecuteCommand(char* command) {
  if (strstr(command, "\"Calibrate\":\"Start\"") != NULL) {
    runCalibration();
  }
  else if (strstr(command, "\"Maze\":\"Learn\"") != NULL) {
    if (!isCalibrated) {
      runCalibration();
    }
    raw_path_length = 0;
    raw_path[0] = '\0';
    path_length = 0;
    path[0] = '\0';
    currentMode = MODE_LEARNING;
  }
  else if (strstr(command, "\"Maze\":\"Solve\"") != NULL) {
    if (!isCalibrated) {
      runCalibration();
    }
    currentMode = MODE_SOLVING;
  }
  else if (strstr(command, "\"Maze\":\"Stop\"") != NULL) {
    currentMode = MODE_STOPPED;
    stop();
    sendStateTelemetry();
  }
  else if (strstr(command, "\"Maze\":\"Clear\"") != NULL) {
    raw_path_length = 0;
    raw_path[0] = '\0';
    path_length = 0;
    path[0] = '\0';
    sendStateTelemetry();
  }
  else if (currentMode == MODE_STOPPED) {
    if (strstr(command, "\"Forward\":\"Down\"") != NULL) { forward(); }
    else if (strstr(command, "\"Forward\":\"Up\"") != NULL) { stop(); }
    else if (strstr(command, "\"Backward\":\"Down\"") != NULL) { backward(); }
    else if (strstr(command, "\"Backward\":\"Up\"") != NULL) { stop(); }
    else if (strstr(command, "\"Left\":\"Down\"") != NULL) { left(); }
    else if (strstr(command, "\"Left\":\"Up\"") != NULL) { stop(); }
    else if (strstr(command, "\"Right\":\"Down\"") != NULL) { right(); }
    else if (strstr(command, "\"Right\":\"Up\"") != NULL) { stop(); }
    else if (strstr(command, "\"Stop\":\"Down\"") != NULL) { stop(); }
  }

  if (strstr(command, "\"Low\":\"Down\"") != NULL) { Speed = 100; }
  else if (strstr(command, "\"Medium\":\"Down\"") != NULL) { Speed = 150; }
  else if (strstr(command, "\"High\":\"Down\"") != NULL) { Speed = 200; }

  if (strstr(command, "\"BZ\":\"on\"") != NULL) { beep_on; }
  else if (strstr(command, "\"BZ\":\"off\"") != NULL) { beep_off; }

  if (strstr(command, "\"RGB\":\"") != NULL) {
    char* rgbValStart = strstr(command, "\"RGB\":\"") + 7;
    char* rgbValEnd = strchr(rgbValStart, '\"');
    if (rgbValEnd != NULL) {
      *rgbValEnd = '\0';
      char* comma1 = strchr(rgbValStart, ',');
      if (comma1 != NULL) {
        *comma1 = '\0';
        char* comma2 = strchr(comma1 + 1, ',');
        if (comma2 != NULL) {
          *comma2 = '\0';
          byte R = atoi(rgbValStart);
          byte G = atoi(comma1 + 1);
          byte B = atoi(comma2 + 1);
          for (int i = 0; i < 4; i++) {
            RGB.setPixelColor(i, RGB.Color(R, G, B));
          }
          RGB.show();
        }
      }
    }
  }
}

// Drives motors with custom speeds
void SetSpeeds(int Aspeed, int Bspeed) {
  Aspeed = constrain(Aspeed, -255, 255);
  Bspeed = constrain(Bspeed, -255, 255);

  if(Aspeed < 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, -Aspeed);      
  } else {
    digitalWrite(AIN1, LOW); 
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, Aspeed);  
  }
  
  if(Bspeed < 0) {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    analogWrite(PWMB, -Bspeed);      
  } else {
    digitalWrite(BIN1, LOW); 
    digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, Bspeed);  
  }
}

void forward() {
  analogWrite(PWMA, constrain(Speed + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(Speed + RIGHT_SPEED_OFFSET, 0, 255));
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void backward() {
  analogWrite(PWMA, constrain(Speed + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(Speed + RIGHT_SPEED_OFFSET, 0, 255));
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
}

void right() {
  analogWrite(PWMA, 75);
  analogWrite(PWMB, 75);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
}

void left() {
  analogWrite(PWMA, 75);
  analogWrite(PWMB, 75);
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void stop() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}

void runCalibration() {
  Serial.println(F("{\"State\":\"Calibrating\"}"));
  for (int i = 0; i < 4; i++) {
    RGB.setPixelColor(i, 0x0000FF); // Blue
  }
  RGB.show();
  
  // Pivot left and right to sweep over black line
  for (int i = 0; i < 100; i++) {
    if (i < 25 || i >= 75) {
      SetSpeeds(80, -80);
    } else {
      SetSpeeds(-80, 80);
    }
    trs.calibrate();
    delay(20);
  }
  stop();

  isCalibrated = true;
  Serial.println(F("{\"State\":\"Calibrated\"}"));
}

void beep(int duration) {
  beep_on;
  delay(duration);
  beep_off;
}

// Telemetry Transmitters
void sendLiveTelemetry(unsigned int* calValues, unsigned int position) {
  Serial.print(F("{\"Sensors\":["));
  Serial.print(calValues[0]); Serial.print(",");
  Serial.print(calValues[1]); Serial.print(",");
  Serial.print(calValues[2]); Serial.print(",");
  Serial.print(calValues[3]); Serial.print(",");
  Serial.print(calValues[4]);
  Serial.print(F("],\"Pos\":"));
  Serial.print(position);
  Serial.print(F(",\"State\":\""));
  if (currentMode == MODE_LEARNING) Serial.print(F("Learning"));
  else if (currentMode == MODE_SOLVING) Serial.print(F("Solving"));
  else if (currentMode == MODE_STOPPED) Serial.print(isCalibrated ? F("Calibrated") : F("Stopped"));
  Serial.print(F("\",\"Path\":\""));
  Serial.print(raw_path);
  Serial.print(F("\",\"SimPath\":\""));
  Serial.print(path);
  Serial.println(F("\"}"));
}

void sendStateTelemetry() {
  Serial.print(F("{\"State\":\""));
  if (currentMode == MODE_STOPPED) {
    Serial.print(isCalibrated ? F("Calibrated") : F("Stopped"));
  } else if (currentMode == MODE_CALIBRATING) {
    Serial.print(F("Calibrating"));
  } else if (currentMode == MODE_LEARNING) {
    Serial.print(F("Learning"));
  } else if (currentMode == MODE_SOLVING) {
    Serial.print(F("Solving"));
  }
  Serial.print(F("\",\"Path\":\""));
  Serial.print(raw_path);
  Serial.print(F("\",\"SimPath\":\""));
  Serial.print(path);
  Serial.println(F("\"}"));
}

void sendSolveStepTelemetry(int stepIndex) {
  Serial.print(F("{\"State\":\"Solving\",\"Step\":"));
  Serial.print(stepIndex);
  Serial.print(F(",\"Path\":\""));
  Serial.print(raw_path);
  Serial.print(F("\",\"SimPath\":\""));
  Serial.print(path);
  Serial.println(F("\"}"));
}

void playIdleAnimation() {
  static unsigned long lastRainbowTime = 0;
  static uint16_t j = 0;
  if (millis() - lastRainbowTime > 100) {
    lastRainbowTime = millis();
    for (int i = 0; i < RGB.numPixels(); i++) {
      RGB.setPixelColor(i, Wheel(((i * 256 / RGB.numPixels()) + j) & 255));
    }
    RGB.show();
    if (j++ > 256 * 4) j = 0;
  }
}

uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return RGB.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return RGB.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return RGB.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void PCF8574Write(byte data) {
  Wire.beginTransmission(Addr);
  Wire.write(data);
  Wire.endTransmission(); 
}

byte PCF8574Read() {
  int data = -1;
  Wire.requestFrom(Addr, 1);
  if(Wire.available()) {
    data = Wire.read();
  }
  return data;
}
