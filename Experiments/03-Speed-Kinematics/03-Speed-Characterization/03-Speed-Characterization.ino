/*
   Experiment 03: Speed Characterization

   This sketch drives the AlphaBot2 forward at a specified PWM speed
   for exactly 3.0 seconds, then brakes sharply.

   Students will use this to measure the physical distance traveled
   at different PWM speeds to build their actuator curve.

   IMPORTANT: For accurate distance measurements, the robot must stop
   as sharply as possible — brakeMotors() is used instead of coastMotors()
   so the robot does not roll past its true stop position.

   STOP MODES (TB6612FNG):
   - coastMotors() : AIN1=LOW,  AIN2=LOW  → power cut, wheels roll free
   - brakeMotors() : AIN1=HIGH, AIN2=HIGH → motor shorted, stops sharply
*/

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// --- CHARACTERIZATION PARAMETER ---
// TODO: Change this value for each data point: 25, 30, 35, 40, 45
const int TEST_PWM = 25;

// Carry over your calibrated offsets from Experiment 01
const int LEFT_SPEED_OFFSET  = 0.5;
const int RIGHT_SPEED_OFFSET = 0;

void setup() {
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  coastMotors();
  delay(1500); // Give students time to place the robot on the start line
}

void loop() {
  // Drive forward at the test PWM value
  moveForward(TEST_PWM);

  // Drive for exactly 3.0 seconds (3000 ms)
  delay(3000);

  // Brake sharply — important for accurate distance measurement
  // Coast would let the robot roll past the true 3-second stop point
  brakeMotors();

  // Lock execution — mark stop position, then measure distance
  while (1) {
    delay(1000);
  }
}

/**
 * Commands both motors forward using the H-Bridge.
 * Applies calibrated offsets from Experiment 01.
 *
 * @param baseSpeed Target PWM value (0 to 255)
 */
void moveForward(int baseSpeed) {
  int finalLeftSpeed  = constrain(baseSpeed + LEFT_SPEED_OFFSET,  0, 255);
  int finalRightSpeed = constrain(baseSpeed + RIGHT_SPEED_OFFSET, 0, 255);

  // Left Motor Forward:  AIN1 = LOW,  AIN2 = HIGH
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  // Right Motor Forward: BIN1 = LOW, BIN2 = HIGH
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, finalLeftSpeed);
  analogWrite(PWMB, finalRightSpeed);
}

/**
 * COAST STOP: Cuts power — wheels roll free until friction stops them.
 * TB6612FNG: AIN1=LOW, AIN2=LOW → High-Z (freewheeling)
 * Not used at end of run — would cause distance measurement error.
 */
void coastMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}

/**
 * BRAKE STOP: Actively shorts the motor terminals via the H-Bridge.
 * The motor's back-EMF creates a braking force — robot stops sharply.
 * TB6612FNG: AIN1=HIGH, AIN2=HIGH → Short-circuit brake
 *
 * Used at end of 3-second run to give an accurate stop position
 * for distance measurement.
 */
void brakeMotors() {
  // Apply active brake
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, 255); // Full brake force
  analogWrite(PWMB, 255);

  // Hold brake for 300ms then release to coast
  delay(300);
  coastMotors();
}
