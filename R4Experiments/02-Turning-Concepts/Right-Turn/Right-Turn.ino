/*
   Experiment 02: Right-Turn (R4 WiFi - WiFi/OTA Disabled)

   This sketch drives the AlphaBot2 in a Right Pivot Turn (on the spot).
   Students will calibrate the TURN_DURATION_MS constant so that the
   robot turns EXACTLY 90 degrees clockwise.

   Compatible with Arduino UNO R4 WiFi.
*/

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// --- CALIBRATION ---
// TODO: Adjust this delay (in milliseconds) until the turn is exactly 90
// degrees!
const unsigned long TURN_DURATION_MS = 200; // Estimated baseline
const int TURN_SPEED = 65;                  // Fixed turning speed

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Right Turn Setup (Wi-Fi/OTA Disabled)...");

  // Configure motor driver pins
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  stopMotors();
  delay(1000); // Wait 1 second before starting
}

void loop() {
  // Execute pivot right turn
  Serial.println("Executing right turn...");
  pivotRight(TURN_SPEED);

  // Keep turning for the calibrated duration
  delay(TURN_DURATION_MS);

  // Stop the motors
  stopMotors();
  Serial.println("Turn complete.");

  // Hold execution forever
  while (1) {
    delay(1000);
  }
}

/**
 * Commands the H-Bridge to turn the left wheel forward and the right wheel
 * backward. This spins the robot clockwise around its center (Pivot Turn).
 */
void pivotRight(int speed) {
  // Left Motor Forward: AIN1 = LOW, AIN2 = HIGH
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  // Right Motor Backward: BIN1 = HIGH, BIN2 = LOW
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  // Apply speeds
  analogWrite(PWMA, speed);
  analogWrite(PWMB, speed);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
