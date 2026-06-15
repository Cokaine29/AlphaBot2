/*
   Experiment 03: Speed Characterization

   This sketch drives the AlphaBot2 forward at a specified PWM speed
   for exactly 3.0 seconds, then stops.

   Students will use this to measure the physical distance traveled
   at different PWM speeds to calibrate their robot.
*/

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// --- CHARACTERIZATION PARAMETER ---
// TODO: Change this value (80, 120, 160, 200) to collect different data points
const int TEST_PWM = 80;

// Base wheel balance offsets calibrated in Experiment 01
const int LEFT_SPEED_OFFSET = 0;
const int RIGHT_SPEED_OFFSET = 0;

void setup() {
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  stopMotors();
  delay(1500); // Give students time to place the bot on the start line
}

void loop() {
  // Drive forward at test speed
  moveForward(TEST_PWM);

  // Drive for exactly 3.0 seconds (3000 ms)
  delay(3000);

  // Stop the motors
  stopMotors();

  // Lock execution forever
  while (1) {
    delay(1000);
  }
}

/**
 * Commands both motors forward using the H-Bridge
 */
void moveForward(int baseSpeed) {
  int finalLeftSpeed = constrain(baseSpeed + LEFT_SPEED_OFFSET, 0, 255);
  int finalRightSpeed = constrain(baseSpeed + RIGHT_SPEED_OFFSET, 0, 255);

  // Left Motor Forward: AIN1 = LOW, AIN2 = HIGH
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  // Right Motor Forward: BIN1 = LOW, BIN2 = HIGH
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  // Apply speeds
  analogWrite(PWMA, finalLeftSpeed);
  analogWrite(PWMB, finalRightSpeed);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
