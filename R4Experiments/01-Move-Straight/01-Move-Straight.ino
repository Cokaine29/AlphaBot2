/*
   Experiment 01: Open-Loop Control & H-Bridge Motor Calibration (R4 WiFi)
   
   This sketch drives the AlphaBot2 forward for 5 seconds.
   Students will calibrate the LEFT_SPEED_OFFSET and RIGHT_SPEED_OFFSET
   constants to ensure the robot moves in a perfectly straight line.
   
   Compatible with Arduino UNO R4 WiFi.
*/

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// --- CALIBRATION OFFSETS ---
// TODO: Adjust these values during the lab to align your wheels!
// Range: -50 to 50
const int LEFT_SPEED_OFFSET = 0;
const int RIGHT_SPEED_OFFSET = 0;

void setup() {
  // Configure motor driver control pins as outputs
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Initialize motors to fully stopped
  stopMotors();
  delay(1000); // Wait 1 second before starting
}

void loop() {
  // Move forward at base speed of 60 (low speed for classroom safety)
  moveForward(60);

  // Keep moving for 5 seconds
  delay(5000);

  // Stop the motors
  stopMotors();

  // Stop program execution (infinite loop)
  while (1) {
    delay(1000);
  }
}

/**
 * Commands the H-bridge to drive both motors forward.
 * Applies the calibration speed offsets to balance rotation rates.
 *
 * @param baseSpeed Target PWM value (0 to 255)
 */
void moveForward(int baseSpeed) {
  // 1. Calculate speeds after offsets
  int finalLeftSpeed = baseSpeed + LEFT_SPEED_OFFSET;
  int finalRightSpeed = baseSpeed + RIGHT_SPEED_OFFSET;

  // 2. Constrain speeds to valid PWM range [0, 255]
  finalLeftSpeed = constrain(finalLeftSpeed, 0, 255);
  finalRightSpeed = constrain(finalRightSpeed, 0, 255);

  // 3. Set directions to Forward
  // Left Motor Forward: AIN1 = LOW, AIN2 = HIGH
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  // Right Motor Forward: BIN1 = LOW, BIN2 = HIGH
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  // 4. Output the analog PWM speeds to the H-Bridge speed pins
  analogWrite(PWMA, finalLeftSpeed);
  analogWrite(PWMB, finalRightSpeed);
}

/**
 * Instantly stops both motors by cutting PWM signals and pulling
 * direction pins LOW.
 */
void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
