/*
   Experiment 07: Line Tracking (R4 WiFi - WiFi/OTA Disabled, OLED Removed)

   This sketch reads the 5-channel TRSensors reflectance array to calibrate 
   black-vs-white reflectivity thresholds and track a black line on a white floor
   using a Proportional-Derivative (PD) control loop.

   User prompts and telemetry are printed to the Serial Monitor at 115200 baud.
   Calibration is initiated and started using the onboard joystick center button.

   Compatible with Arduino UNO R4 WiFi.
*/

#include "TRSensors.h"
#include <Wire.h>

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// PCF8574 I2C Expander Address
#define PCF8574_ADDR 0x20

#define NUM_SENSORS 5
TRSensors trs = TRSensors();
unsigned int sensorValues[NUM_SENSORS];

// Speed offsets to calibrate wheel imbalance
const int LEFT_SPEED_OFFSET = 0;
const int RIGHT_SPEED_OFFSET = 0;

// PID Tracking Constants
const int MAX_SPEED = 100; // Safe base speed for tracking
int lastProportional = 0;

// Function Prototypes
void PCF8574Write(byte data);
byte PCF8574Read();
void beep(int durationMs);
void waitForJoystickCenter();
void stopMotors();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("AlphaBot2 Experiment 07 - Line Tracking starting (Wi-Fi/OTA Disabled)...");

  Wire.begin();

  // Configure motor control pins
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Ensure robot is stopped
  stopMotors();

  // Phase 1: Calibration
  Serial.println("==================================================");
  Serial.println("STEP 1: Place the robot's middle sensor over the black line.");
  Serial.println("Press the joystick CENTER key to start calibration.");
  Serial.println("==================================================");
  waitForJoystickCenter();

  Serial.println("\nCalibration started! The robot will pivot back and forth for 5 seconds.");
  beep(100);

  // Rotate left/right to sweep sensors across the black line
  for (int i = 0; i < 100; i++) {
    if (i < 25 || i >= 75) {
      // Pivot Right: Left motor forward, Right motor backward
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
    } else {
      // Pivot Left: Left motor backward, Right motor forward
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    }
    analogWrite(PWMA, 80);
    analogWrite(PWMB, 80);

    // Read sensors and register min/max calibration thresholds
    trs.calibrate();
    delay(50); // Total duration: 100 * 50ms = 5000ms (5 seconds)
  }

  stopMotors();
  beep(100);
  delay(100);
  beep(100);
  Serial.println("Calibration Complete!");

  // Phase 2: Start Navigation
  Serial.println("==================================================");
  Serial.println("STEP 2: Place the robot on the tracking course.");
  Serial.println("Press the joystick CENTER key to start Line Tracking.");
  Serial.println("==================================================");
  waitForJoystickCenter();
  beep(300);

  // Set default forward direction for both motors
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void loop() {
  // Read the line position: returns a value from 0 to 4000
  // (2000 is perfectly centered on the black line)
  unsigned int position = trs.readLine(sensorValues);

  // Print telemetry to Serial Monitor for student analysis
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(sensorValues[i]);
    Serial.print("\t");
  }
  Serial.print("Pos: ");
  Serial.println(position);

  // Check for "No Line" condition
  // If the middle 3 sensors read very light values (white floor), stop the robot.
  if (sensorValues[1] > 900 && sensorValues[2] > 900 && sensorValues[3] > 900) {
    stopMotors();
    Serial.println("Line lost! Stopping motors.");
    return;
  }

  // Ensure direction pins are set back to Forward in case we stopped or pivoted
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  // Proportional Term: Deviation from center (2000)
  int proportional = (int)position - 2000;

  // Derivative Term: Speed of change
  int derivative = proportional - lastProportional;
  lastProportional = proportional;

  // Calculate power difference using PD coefficients (Kp = 1/20, Kd = 15)
  // Adjust Kp (proportional divisor) and Kd (derivative multiplier) to tune performance
  int powerDifference = proportional / 20 + derivative * 15;

  // Constrain power difference to prevent oversteering
  if (powerDifference > MAX_SPEED) {
    powerDifference = MAX_SPEED;
  }
  if (powerDifference < -MAX_SPEED) {
    powerDifference = -MAX_SPEED;
  }

  // Apply differential steering speed adjustments
  if (powerDifference < 0) {
    // Turn Left: Slow down the Left motor, keep Right motor at base speed
    analogWrite(PWMA, constrain(MAX_SPEED + powerDifference + LEFT_SPEED_OFFSET, 0, 255));
    analogWrite(PWMB, constrain(MAX_SPEED + RIGHT_SPEED_OFFSET, 0, 255));
  } else {
    // Turn Right: Keep Left motor at base speed, slow down the Right motor
    analogWrite(PWMA, constrain(MAX_SPEED + LEFT_SPEED_OFFSET, 0, 255));
    analogWrite(PWMB, constrain(MAX_SPEED - powerDifference + RIGHT_SPEED_OFFSET, 0, 255));
  }
}

/**
 * Sends a byte of data to the PCF8574 I/O expander over I2C
 */
void PCF8574Write(byte data) {
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(data);
  Wire.endTransmission();
}

/**
 * Reads a byte of data from the PCF8574 I/O expander over I2C
 */
byte PCF8574Read() {
  int data = -1;
  Wire.requestFrom(PCF8574_ADDR, 1);
  if (Wire.available()) {
    data = Wire.read();
  }
  return data;
}

/**
 * Rings the active-low buzzer for a specified duration in milliseconds
 */
void beep(int durationMs) {
  byte currentPort = PCF8574Read();
  PCF8574Write(0xDF & currentPort); // Set P5 LOW to turn ON buzzer
  delay(durationMs);
  PCF8574Write(0x20 | currentPort); // Set P5 HIGH to turn OFF buzzer
}

/**
 * Halts execution and blocks until the joystick center key (P4 / 0xEF) is pressed
 */
void waitForJoystickCenter() {
  byte keyState = 0xFF;
  while (keyState != 0xEF) {
    // Enable pull-ups on expander pins P0-P4
    PCF8574Write(0x1F | PCF8574Read());
    // Read pins and mask top 3 unused bits
    keyState = PCF8574Read() | 0xE0;
    delay(50);
  }
  
  // Wait for button release (debounce)
  while (keyState == 0xEF) {
    PCF8574Write(0x1F | PCF8574Read());
    keyState = PCF8574Read() | 0xE0;
    delay(10);
  }
}

/**
 * Stops both motors immediately
 */
void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
