/*
   Experiment 02: Spin-360

   This sketch drives the AlphaBot2 in a full 360-degree rotation.
   Students will calibrate the SPIN_DURATION_MS constant so that the
   robot does exactly one full spin and ends up facing the starting direction.
*/

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// --- CALIBRATION ---
// TODO: Adjust this delay (in milliseconds) until the robot completes exactly
// one 360-degree rotation!
const unsigned long SPIN_DURATION_MS = 800; // Estimated baseline
const int SPIN_SPEED = 60;                  // Fixed spinning speed

void setup() {
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
  // Spin clockwise
  pivotRight(SPIN_SPEED);

  // Keep spinning for the calibrated duration
  delay(SPIN_DURATION_MS);

  // Stop the motors
  stopMotors();

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
