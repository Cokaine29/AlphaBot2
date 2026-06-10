#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "TRSensors.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Motor-L forward (IN2).
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

// Speed offsets to calibrate wheel imbalance (Configured to 0, 0 as requested)
#define LEFT_SPEED_OFFSET 0
#define RIGHT_SPEED_OFFSET 0

unsigned int sensorValues[NUM_SENSORS];
unsigned int last_proportional = 0;
unsigned int position;
long integral = 0;
byte value;
unsigned long lasttime = 0;
uint16_t j = 0;
const int maximum = 100; // Standard speed limit

void PCF8574Write(byte data);
byte PCF8574Read();
void forward();
void backward();
void right();
void left();
void stop();
uint32_t Wheel(byte WheelPos);
void handleJunction();

void setup() {
  Serial.begin(115200);
  
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
  display.println("Junctions");
  display.display();
  
  RGB.begin();
  RGB.setPixelColor(0, 0x00FF00); // Green
  RGB.setPixelColor(1, 0x00FF00);
  RGB.setPixelColor(2, 0x00FF00);
  RGB.setPixelColor(3, 0x00FF00);
  RGB.show();
  delay(1500);

  // Prompt to Calibrate
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Press Joystick Center");
  display.setCursor(0, 30);
  display.println("to start calibration");
  display.display();

  // Wait for joystick button pressed (Center = 0xEF)
  value = 0;
  while (value != 0xEF) {
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
  }

  // Configure Motor Pins
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT); 
  pinMode(BIN2, OUTPUT);
  
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN2, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  // Run Auto-Rotation Calibration
  display.clearDisplay();
  display.println("Calibrating...");
  display.display();
  
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
  }
  
  // Stop after calibration
  stop();
  RGB.setPixelColor(0, 0x0000FF); // Blue
  RGB.setPixelColor(1, 0x0000FF);
  RGB.setPixelColor(2, 0x0000FF);
  RGB.setPixelColor(3, 0x0000FF);
  RGB.show();

  // Show visual diagnostic bar
  value = 0;
  while (value != 0xEF) {
    PCF8574Write(0x1F | PCF8574Read());
    value = PCF8574Read() | 0xE0;
    position = trs.readLine(sensorValues) / 200;
    
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Calibration Done!");
    display.setCursor(0, 30);
    display.println("Press Center to GO");
    display.setCursor(0, 50);
    for (int i = 0; i < 21; i++) {
      display.print('_');
    }
    display.setCursor(position * 6, 50);
    display.print("**");
    display.display();
  }

  // Start Line Tracking
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("GO!");
  display.display();
  delay(500);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void loop() {
  // Read line position and sensor values
  position = trs.readLine(sensorValues);
  
  // Fetch calibrated readings
  unsigned int calValues[NUM_SENSORS];
  trs.readCalibrated(calValues);

  // Check if we hit a road junction / node:
  // A node is hit if the left sensor (S0) OR right sensor (S4) detects the line (calibrated reading > 750)
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
  }

  // Standard PID Line Following Calculations
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

  // Halt if robot runs completely off the line
  if (sensorValues[1] > 900 && sensorValues[2] > 900 && sensorValues[3] > 900) {
    analogWrite(PWMA, 0);
    analogWrite(PWMB, 0);
  }

  // Rainbow LED Effect
  if (millis() - lasttime > 100) {
    lasttime = millis();
    for (int i = 0; i < RGB.numPixels(); i++) {
      RGB.setPixelColor(i, Wheel(((i * 256 / RGB.numPixels()) + j) & 255));
    }
    RGB.show();
    if (j++ > 256 * 4)
      j = 0;
  }
}

// Handles stopping at the junction and prompting for direction input
void handleJunction() {
  stop();
  beep_on;
  delay(150);
  beep_off;

  // Flash LEDs orange to indicate waiting
  for(int i=0; i<4; i++) {
    RGB.setPixelColor(i, 0xFF8C00); // Dark Orange
  }
  RGB.show();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("JUNCTION DETECTED");
  display.println("-----------------");
  display.println("Choose Direction:");
  display.println("");
  display.println(" [W/UP]   -> Straight");
  display.println(" [A/LEFT] -> Turn Left");
  display.println(" [D/RGHT] -> Turn Right");
  display.display();

  // Block and poll PCF8574 for Joystick input
  byte key = 0xFF;
  while (true) {
    PCF8574Write(0x1F | PCF8574Read());
    key = PCF8574Read() | 0xE0;
    
    // Joystick UP (0xFE), LEFT (0xFB), or RIGHT (0xFD)
    if (key == 0xFE || key == 0xFB || key == 0xFD) {
      beep_on;
      delay(100);
      beep_off;
      break;
    }
    delay(10);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 20);

  unsigned int calValues[NUM_SENSORS];

  if (key == 0xFE) { // Drive Straight
    display.println("STRAIGHT");
    display.display();
    
    // Drive forward for a short duration to clear the intersection lines
    forward();
    delay(250);
  } 
  else if (key == 0xFB) { // Turn Left
    display.println("TURN LEFT");
    display.display();
    
    // Start spinning left
    left();
    delay(200); // Give it a head start to get off the current line
    
    // Keep spinning left until the middle sensor (S2) locks onto the line
    while (true) {
      trs.readCalibrated(calValues);
      if (calValues[2] > 700) { // Black line detected on center sensor
        break;
      }
      delay(5);
    }
    stop();
    delay(100);
  } 
  else if (key == 0xFD) { // Turn Right
    display.println("TURN RIGHT");
    display.display();
    
    // Start spinning right
    right();
    delay(200); // Give it a head start to get off the current line
    
    // Keep spinning right until the middle sensor (S2) locks onto the line
    while (true) {
      trs.readCalibrated(calValues);
      if (calValues[2] > 700) { // Black line detected on center sensor
        break;
      }
      delay(5);
    }
    stop();
    delay(100);
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
  analogWrite(PWMA, constrain(maximum + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(maximum + RIGHT_SPEED_OFFSET, 0, 255));
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void backward() {
  analogWrite(PWMA, constrain(maximum + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(maximum + RIGHT_SPEED_OFFSET, 0, 255));
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
