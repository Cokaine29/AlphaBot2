/*
   Experiment 02: Left-Turn

   This sketch drives the AlphaBot2 in a Left Pivot Turn (on the spot).
   Students will calibrate the TURN_DURATION_MS constant so that the
   robot turns EXACTLY 90 degrees counter-clockwise.
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
const unsigned long TURN_DURATION_MS = 200; 
const int TURN_SPEED = 60;                  // Fixed turning speed

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
  // Execute pivot left turn
  pivotLeft(TURN_SPEED);

  // Keep turning for the calibrated duration
  delay(TURN_DURATION_MS);

  // Stop the motors
  stopMotors();

  // Hold execution forever
  while (1) {
    delay(1000);
  }
}

/**
 * Commands the H-Bridge to turn the left wheel backward and the right wheel
 * forward. This spins the robot counter-clockwise around its center (Pivot
 * Turn).
 */
void pivotLeft(int speed) {
  // Left Motor Backward: AIN1 = HIGH, AIN2 = LOW
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  // Right Motor Forward: BIN1 = LOW, BIN2 = HIGH
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

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
