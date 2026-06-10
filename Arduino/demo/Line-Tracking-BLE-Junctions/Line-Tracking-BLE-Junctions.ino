#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "TRSensors.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <aJSON.h>

#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Motor-L forward (IN2)
#define AIN1 A1 // Motor-L backward (IN1)
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Motor-R forward (IN3)
#define BIN2 A3 // Motor-R backward (IN4)
#define PIN 7   // RGB NeoPixel Pin
#define NUM_SENSORS 5
#define OLED_RESET 9
#define OLED_SA0 8
#define Addr 0x20 // PCF8574 Address

#define beep_on PCF8574Write(0xDF & PCF8574Read())
#define beep_off PCF8574Write(0x20 | PCF8574Read())

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
TRSensors trs = TRSensors();
Adafruit_NeoPixel RGB = Adafruit_NeoPixel(4, PIN, NEO_GRB + NEO_KHZ800);
aJsonStream serial_stream(&Serial);

// Speed offsets to calibrate wheel imbalance (Configured to 0, 0 as requested)
#define LEFT_SPEED_OFFSET 0
#define RIGHT_SPEED_OFFSET 0

unsigned int sensorValues[NUM_SENSORS];
unsigned int last_proportional = 0;
unsigned int position;
long integral = 0;
byte value;
unsigned long lastTelemetryTime = 0;
unsigned long lastRainbowTime = 0;
uint16_t j = 0;
const int maximum = 100; // Speed limit for line tracking
int Speed = 150;        // Speed for manual D-pad controls

bool isTracking = false;
bool isCalibrated = false;

void PCF8574Write(byte data);
byte PCF8574Read();
void forward();
void backward();
void right();
void left();
void stop();
void runCalibration();
void handleJunction();
void ComExecution(aJsonObject *msg);
uint32_t Wheel(byte WheelPos);

void setup() {
  Serial.begin(9600); // Default baud for standard 6-pin BLE modules
  
  // Set I2C Address Pin (SA0) to LOW to select 0x3C address
  pinMode(OLED_SA0, OUTPUT);
  digitalWrite(OLED_SA0, LOW);

  Wire.begin();

  // Initialize OLED Screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("AlphaBot2");
  display.setCursor(10, 35);
  display.println("BLE-Node");
  display.display();
  
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

  // Show status on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Ready. Awaiting BLE");
  display.setCursor(0, 25);
  display.println("or Joystick Center");
  display.setCursor(0, 40);
  display.println("to start calibration.");
  display.display();

  Serial.println("{\"State\":\"Stopped\"}");
}

void loop() {
  // 1. Read commands from BLE Serial
  if (serial_stream.available()) {
    serial_stream.skip();
  }
  if (serial_stream.available()) {
    aJsonObject *msg = aJson.parse(&serial_stream);
    if (msg) {
      ComExecution(msg);
      aJson.deleteItem(msg);
    }
  }

  // 2. Read physical Joystick inputs
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
        Serial.println("{\"State\":\"Tracking\"}");
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(20, 20);
        display.println("TRACKING");
        display.display();
        
        // Reset direction pins
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);
        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, HIGH);
      } else {
        stop();
        Serial.println("{\"State\":\"Stopped\"}");
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(20, 20);
        display.println("STOPPED");
        display.display();
      }
    }
    // Wait for button release
    while (value == 0xEF) {
      PCF8574Write(0x1F | PCF8574Read());
      value = PCF8574Read() | 0xE0;
      delay(10);
    }
  }

  // 3. Autonomous Line Tracking Action
  if (isTracking) {
    position = trs.readLine(sensorValues);
    
    unsigned int calValues[NUM_SENSORS];
    trs.readCalibrated(calValues);

    // Send live telemetry to BLE interface every 250ms
    if (millis() - lastTelemetryTime > 250) {
      lastTelemetryTime = millis();
      Serial.print("{\"Sensors\":[");
      Serial.print(calValues[0]); Serial.print(",");
      Serial.print(calValues[1]); Serial.print(",");
      Serial.print(calValues[2]); Serial.print(",");
      Serial.print(calValues[3]); Serial.print(",");
      Serial.print(calValues[4]);
      Serial.print("],\"Pos\":");
      Serial.print(position);
      Serial.print(",\"State\":\"Tracking\"}\n");
    }

    // Check if we hit a road junction / node:
    // A node is hit if left sensor (S0) OR right sensor (S4) detects line (calibrated reading > 750)
    if (calValues[0] > 750 || calValues[4] > 750) {
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
  Serial.println("{\"State\":\"Calibrating\"}");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Calibrating...");
  display.println("Auto-rotating...");
  display.display();
  
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
  Serial.println("{\"State\":\"Calibrated\"}");

  // Diagnostic calibration alignment checker
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Calibration Done!");
  display.setCursor(0, 25);
  display.println("Ready to Track!");
  display.display();
  delay(1000);
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

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("JUNCTION DETECTED");
  display.println("-----------------");
  display.println("Waiting for BLE...");
  display.display();

  // Send junction detected state to BLE
  Serial.println("{\"State\":\"Junction\"}");

  String choice = "";
  byte joystickKey = 0xFF;

  // Block and poll until direction is chosen via BLE OR Joystick
  while (true) {
    // 1. Check physical joystick as backup
    PCF8574Write(0x1F | PCF8574Read());
    joystickKey = PCF8574Read() | 0xE0;
    
    if (joystickKey == 0xFE) { choice = "Straight"; break; }
    if (joystickKey == 0xFB) { choice = "Left"; break; }
    if (joystickKey == 0xFD) { choice = "Right"; break; }

    // 2. Check BLE serial stream
    if (serial_stream.available()) {
      serial_stream.skip();
    }
    if (serial_stream.available()) {
      aJsonObject *msg = aJson.parse(&serial_stream);
      if (msg) {
        aJsonObject *goObj = aJson.getObjectItem(msg, "Go");
        if (goObj) {
          choice = goObj->valuestring;
        }
        aJson.deleteItem(msg);
      }
      if (choice == "Straight" || choice == "Left" || choice == "Right") {
        break;
      }
    }
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

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 20);

  unsigned int calValues[NUM_SENSORS];

  if (choice == "Straight") {
    display.println("STRAIGHT");
    display.display();
    
    forward();
    delay(250); // Drive straight to clear junction line
  } 
  else if (choice == "Left") {
    display.println("TURN LEFT");
    display.display();
    
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
  else if (choice == "Right") {
    display.println("TURN RIGHT");
    display.display();
    
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

// Executes incoming JSON commands from BLE
void ComExecution(aJsonObject *msg) {
  // 1. Calibration trigger
  aJsonObject *calObj = aJson.getObjectItem(msg, "Calibrate");
  if (calObj) {
    String str = calObj->valuestring;
    if (str.equals("Start")) {
      runCalibration();
    }
  }

  // 2. Line tracking state controls
  aJsonObject *ltObj = aJson.getObjectItem(msg, "LineTracking");
  if (ltObj) {
    String str = ltObj->valuestring;
    if (str.equals("Start")) {
      if (!isCalibrated) {
        runCalibration();
      }
      isTracking = true;
      Serial.println("{\"State\":\"Tracking\"}");
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(20, 20);
      display.println("TRACKING");
      display.display();
      
      // Reset PID direction pins
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    } else if (str.equals("Stop")) {
      isTracking = false;
      stop();
      Serial.println("{\"State\":\"Stopped\"}");
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(20, 20);
      display.println("STOPPED");
      display.display();
    }
  }

  // 3. Manual override controls (Only allowed when not line-tracking)
  if (!isTracking) {
    aJsonObject *Forward = aJson.getObjectItem(msg, "Forward");
    if (Forward) {
      String str = Forward->valuestring;
      if (str.equals("Down")) { forward(); }
      else if (str.equals("Up")) { stop(); }
    }
    aJsonObject *Backward = aJson.getObjectItem(msg, "Backward");
    if (Backward) {
      String str = Backward->valuestring;
      if (str.equals("Down")) { backward(); }
      else if (str.equals("Up")) { stop(); }
    }
    aJsonObject *Left = aJson.getObjectItem(msg, "Left");
    if (Left) {
      String str = Left->valuestring;
      if (str.equals("Down")) { left(); }
      else if (str.equals("Up")) { stop(); }
    }
    aJsonObject *Right = aJson.getObjectItem(msg, "Right");
    if (Right) {
      String str = Right->valuestring;
      if (str.equals("Down")) { right(); }
      else if (str.equals("Up")) { stop(); }
    }
    aJsonObject *Stop = aJson.getObjectItem(msg, "Stop");
    if (Stop) {
      stop();
    }
  }

  // 4. Utility commands (Active in any mode)
  aJsonObject *Low = aJson.getObjectItem(msg, "Low");
  if (Low) { Speed = 100; }
  aJsonObject *Medium = aJson.getObjectItem(msg, "Medium");
  if (Medium) { Speed = 150; }
  aJsonObject *High = aJson.getObjectItem(msg, "High");
  if (High) { Speed = 200; }

  aJsonObject *buzzer = aJson.getObjectItem(msg, "BZ");
  if (buzzer) {
    String str = buzzer->valuestring;
    if (str.equals("on")) { beep_on; }
    else if (str.equals("off")) { beep_off; }
  }

  aJsonObject *rgb = aJson.getObjectItem(msg, "RGB");
  if (rgb) {
    byte R, G, B;
    String str = rgb->valuestring;
    int temp = str.indexOf(',');
    int temp2 = str.lastIndexOf(',');
    R = str.substring(0, temp).toInt(); 
    G = str.substring(temp+1, temp2).toInt();
    B = str.substring(temp2+1, str.length()).toInt();
    for (int i = 0; i < 4; i++) {
      RGB.setPixelColor(i, RGB.Color(R, G, B));
    }
    RGB.show();
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
