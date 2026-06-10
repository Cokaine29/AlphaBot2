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

// Speed offsets to calibrate wheel imbalance (Configured to 0, 0 as requested)
#define LEFT_SPEED_OFFSET 0
#define RIGHT_SPEED_OFFSET 0

int last_proportional = 0;
long integral = 0;
byte value;
unsigned long lastTelemetryTime = 0;
unsigned long lastRainbowTime = 0;
uint16_t j = 0;
const int maximum = 100; // Speed limit for line tracking
int Speed = 150;        // Speed for manual D-pad controls

bool isTracking = false;
bool isCalibrated = false;

// Fixed-size buffer to prevent heap fragmentation
char cmdBuffer[48];
byte cmdIndex = 0;

void PCF8574Write(byte data);
byte PCF8574Read();
void forward();
void backward();
void right();
void left();
void stop();
void runCalibration();
void handleJunction();
void parseAndExecuteCommand(char* command);
void checkSerial();
uint32_t Wheel(byte WheelPos);

void setup() {
  Serial.begin(9600); // Default baud for standard 6-pin BLE modules
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

  Serial.println(F("{\"State\":\"Stopped\"}"));
}

void loop() {
  // 1. Read commands from BLE Serial (non-blocking brace-to-brace character parser)
  checkSerial();

  // 2. Read physical Joystick inputs (throttled to once every 200ms to prevent I2C bus flooding)
  static unsigned long lastJoystickCheck = 0;
  if (millis() - lastJoystickCheck > 200) {
    lastJoystickCheck = millis();
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
    if (value == 0xEF) { // Center button pressed
      beep_on;
      delay(150);
      beep_off;
      if (!isCalibrated) {
        runCalibration();
      } else {
        isTracking = !isTracking;
        if (isTracking) {
          Serial.println(F("{\"State\":\"Tracking\"}"));
          
          // Reset direction pins
          digitalWrite(AIN1, LOW);
          digitalWrite(AIN2, HIGH);
          digitalWrite(BIN1, LOW);
          digitalWrite(BIN2, HIGH);
        } else {
          stop();
          Serial.println(F("{\"State\":\"Stopped\"}"));
        }
      }
      // Wait for button release
      while (value == 0xEF) {
        PCF8574Write(0x1F | PCF8574Read());
        value = PCF8574Read() | 0xE0;
        delay(10);
      }
    }
  }

  // 3. Autonomous Line Tracking Action
  if (isTracking) {
    unsigned int sensorValues[NUM_SENSORS];
    unsigned int position = trs.readLine(sensorValues);
    
    unsigned int calValues[NUM_SENSORS];
    trs.readCalibrated(calValues);

    // Send live telemetry to BLE interface every 250ms
    if (millis() - lastTelemetryTime > 250) {
      lastTelemetryTime = millis();
      Serial.print(F("{\"Sensors\":["));
      Serial.print(calValues[0]); Serial.print(",");
      Serial.print(calValues[1]); Serial.print(",");
      Serial.print(calValues[2]); Serial.print(",");
      Serial.print(calValues[3]); Serial.print(",");
      Serial.print(calValues[4]);
      Serial.print(F("],\"Pos\":"));
      Serial.print(position);
      Serial.print(F(",\"State\":\"Tracking\"}\n"));
    }

    // Check if we hit a road junction / node:
    // A node is hit if an outer sensor (S0 or S4) detects the line (calibrated reading > 750)
    // AND at least one of the inner sensors (S1, S2, or S3) also detects the line (calibrated reading > 600).
    // This prevents false junction triggers when the robot simply drifts sideways during normal line following.
    if ((calValues[0] > 750 || calValues[4] > 750) && 
        (calValues[1] > 600 || calValues[2] > 600 || calValues[3] > 600)) {
      handleJunction();
      
      // Reset PID values after stopping/turning to prevent integral windup
      last_proportional = 0;
      integral = 0;
      
      // Set direction pins back to forward
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    } else {
      // Standard PID calculations
      int proportional = (int)position - 2000;
      int derivative = proportional - last_proportional;
      integral += proportional;
      last_proportional = proportional;

      int power_difference = proportional / 20 + integral / 10000 + derivative * 10;

      if (power_difference > maximum)
        power_difference = maximum;
      if (power_difference < -maximum)
        power_difference = -maximum;

      if (power_difference < 0) {
        analogWrite(PWMA, constrain(maximum + power_difference + LEFT_SPEED_OFFSET, 0, 255));
        analogWrite(PWMB, constrain(maximum + RIGHT_SPEED_OFFSET, 0, 255));
      } else {
        analogWrite(PWMA, constrain(maximum + LEFT_SPEED_OFFSET, 0, 255));
        analogWrite(PWMB, constrain(maximum - power_difference + RIGHT_SPEED_OFFSET, 0, 255));
      }
    }

    // Halt if robot runs completely off the line
    if (sensorValues[1] > 900 && sensorValues[2] > 900 && sensorValues[3] > 900) {
      analogWrite(PWMA, 0);
      analogWrite(PWMB, 0);
    }
  } else {
    // Heartbeat telemetry (every 1 second when NOT tracking) to sync browser UI state
    if (millis() - lastTelemetryTime > 1000) {
      lastTelemetryTime = millis();
      if (isCalibrated) {
        Serial.println(F("{\"State\":\"Calibrated\"}"));
      } else {
        Serial.println(F("{\"State\":\"Stopped\"}"));
      }
    }

    // Rainbow NeoPixel effect when connected/idle
    if (millis() - lastRainbowTime > 100) {
      lastRainbowTime = millis();
      for (int i = 0; i < RGB.numPixels(); i++) {
        RGB.setPixelColor(i, Wheel(((i * 256 / RGB.numPixels()) + j) & 255));
      }
      RGB.show();
      if (j++ > 256 * 4) j = 0;
    }
  }
}

// Runs sensor auto-rotation calibration
void runCalibration() {
  Serial.println(F("{\"State\":\"Calibrating\"}"));
  
  // NeoPixels turn solid blue during calibration
  for (int i = 0; i < 4; i++) {
    RGB.setPixelColor(i, 0x0000FF);
  }
  RGB.show();
  
  // Rotate back and forth to calibrate
  analogWrite(PWMA, 80);
  analogWrite(PWMB, 80);
  for (int i = 0; i < 100; i++) {
    if (i < 25 || i >= 75) {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    } else {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
    }
    trs.calibrate();
    delay(20);
  }
  stop();

  isCalibrated = true;
  Serial.println(F("{\"State\":\"Calibrated\"}"));
}

// Halts robot at junction and blocks until user sends direction via BLE or Joystick
void handleJunction() {
  stop();
  beep_on;
  delay(150);
  beep_off;

  // Pulse LEDs orange to indicate waiting
  for(int i = 0; i < 4; i++) {
    RGB.setPixelColor(i, 0xFF8C00); // Dark Orange
  }
  RGB.show();

  // Send junction detected state to BLE
  Serial.println(F("{\"State\":\"Junction\"}"));

  // Clear any existing partial buffer
  cmdIndex = 0;
  char choice[12] = "";
  byte joystickKey = 0xFF;

  // Block and poll until direction is chosen via BLE OR Joystick
  while (true) {
    // 1. Check physical joystick as backup
    PCF8574Write(0x1F | PCF8574Read());
    joystickKey = PCF8574Read() | 0xE0;
    
    if (joystickKey == 0xFE) { strcpy(choice, "Straight"); break; }
    if (joystickKey == 0xFB) { strcpy(choice, "Left"); break; }
    if (joystickKey == 0xFD) { strcpy(choice, "Right"); break; }

    // 2. Check BLE serial stream for steering choice (brace-to-brace parser)
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
            if (strstr(cmdBuffer, "\"Go\":\"Straight\"") != NULL) { strcpy(choice, "Straight"); }
            else if (strstr(cmdBuffer, "\"Go\":\"Left\"") != NULL) { strcpy(choice, "Left"); }
            else if (strstr(cmdBuffer, "\"Go\":\"Right\"") != NULL) { strcpy(choice, "Right"); }
            cmdIndex = 0;
            if (choice[0] != '\0') break;
          }
        } else {
          cmdIndex = 0;
        }
      }
    }
    if (choice[0] != '\0') break;
    delay(10);
  }

  // Double beep to confirm selection
  beep_on;
  delay(80);
  beep_off;
  delay(80);
  beep_on;
  delay(80);
  beep_off;

  unsigned int calValues[NUM_SENSORS];

  if (strcmp(choice, "Straight") == 0) {
    forward();
    delay(250); // Drive straight to clear junction line
  } 
  else if (strcmp(choice, "Left") == 0) {
    left();
    delay(200); // Blind start to clear current line
    
    while (true) {
      trs.readCalibrated(calValues);
      if (calValues[2] > 700) { // S2 center sensor finds new line
        break;
      }
      delay(5);
    }
    stop();
    delay(100);
  } 
  else if (strcmp(choice, "Right") == 0) {
    right();
    delay(200); // Blind start to clear current line
    
    while (true) {
      trs.readCalibrated(calValues);
      if (calValues[2] > 700) { // S2 center sensor finds new line
        break;
      }
      delay(5);
    }
    stop();
    delay(100);
  }
}

// Non-blocking serial accumulator (brace-to-brace)
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

// Executes incoming commands from BLE
void parseAndExecuteCommand(char* command) {
  // 1. Calibration trigger
  if (strstr(command, "\"Calibrate\":\"Start\"") != NULL) {
    runCalibration();
  }

  // 2. Line tracking state controls
  else if (strstr(command, "\"LineTracking\":\"Start\"") != NULL) {
    if (!isCalibrated) {
      runCalibration();
    }
    isTracking = true;
    Serial.println(F("{\"State\":\"Tracking\"}"));
    
    // Reset PID direction pins
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
  } else if (strstr(command, "\"LineTracking\":\"Stop\"") != NULL) {
    isTracking = false;
    stop();
    Serial.println(F("{\"State\":\"Stopped\"}"));
  }

  // 3. Manual override controls (Only allowed when not line-tracking)
  else if (!isTracking) {
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

  // 4. Utility commands (Active in any mode)
  if (strstr(command, "\"Low\":\"Down\"") != NULL) { Speed = 100; }
  else if (strstr(command, "\"Medium\":\"Down\"") != NULL) { Speed = 150; }
  else if (strstr(command, "\"High\":\"Down\"") != NULL) { Speed = 200; }

  if (strstr(command, "\"BZ\":\"on\"") != NULL) { beep_on; }
  else if (strstr(command, "\"BZ\":\"off\"") != NULL) { beep_off; }

  if (strstr(command, "\"RGB\":\"") != NULL) {
    char* rgbValStart = strstr(command, "\"RGB\":\"") + 7;
    char* rgbValEnd = strchr(rgbValStart, '\"');
    if (rgbValEnd != NULL) {
      *rgbValEnd = '\0'; // Temporarily null-terminate inside command string
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

void PCF8574Write(byte data) {
  Wire.beginTransmission(Addr);
  Wire.write(data);
  Wire.endTransmission();
}

byte PCF8574Read() {
  int data = -1;
  Wire.requestFrom(Addr, 1);
  if (Wire.available()) {
    data = Wire.read();
  }
  return data;
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
  analogWrite(PWMA, 70);
  analogWrite(PWMB, 70);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
}

void left() {
  analogWrite(PWMA, 70);
  analogWrite(PWMB, 70);
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
