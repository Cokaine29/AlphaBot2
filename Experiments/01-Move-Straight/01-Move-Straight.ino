/*
   Experiment 01: Open-Loop Control & H-Bridge Motor Calibration

   This sketch drives the AlphaBot2 forward for 5 seconds, then brakes.
   Students will calibrate the LEFT_SPEED_OFFSET and RIGHT_SPEED_OFFSET
   constants to ensure the robot moves in a perfectly straight line.

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

// --- CALIBRATION OFFSETS ---
// TODO: Adjust these values during the lab to align your wheels!
// If robot drifts LEFT  → increase LEFT_SPEED_OFFSET
// If robot drifts RIGHT → increase RIGHT_SPEED_OFFSET
const int LEFT_SPEED_OFFSET  = 0.5; // Start small by trying 0.5 
const int RIGHT_SPEED_OFFSET = 0; // Start small by trying 0.5 

void setup() {
  // Configure motor driver control pins as outputs
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Initialize motors to fully stopped (coast)
  coastMotors();
  delay(1000); // Wait 1 second before starting
}

void loop() {
  // Move forward at base PWM = 60 (~23% speed)
  // Slow speed makes drift easier to observe during calibration
  moveForward(60);

  // Keep moving for 4 seconds — observe and measure drift
  delay(4000);

  // Brake the motors sharply (active H-Bridge short-circuit braking)
  brakeMotors();

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
  // 1. Calculate individual motor speeds after applying offsets
  int finalLeftSpeed  = baseSpeed + LEFT_SPEED_OFFSET;
  int finalRightSpeed = baseSpeed + RIGHT_SPEED_OFFSET;

  // 2. Constrain to valid PWM range [0, 255]
  finalLeftSpeed  = constrain(finalLeftSpeed,  0, 255);
  finalRightSpeed = constrain(finalRightSpeed, 0, 255);

  // 3. Set direction pins to FORWARD
  // Left Motor Forward:  AIN1 = LOW,  AIN2 = HIGH
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  // Right Motor Forward: BIN1 = LOW, BIN2 = HIGH
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  // 4. Output PWM signals to set motor speeds
  analogWrite(PWMA, finalLeftSpeed);
  analogWrite(PWMB, finalRightSpeed);
}

/**
 * COAST STOP: Cuts power to both motors.
 * The wheels roll freely until friction brings them to rest.
 * TB6612FNG: AIN1 = LOW, AIN2 = LOW → High-Z (freewheeling)
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
 * The motor's own back-EMF creates a braking force, stopping sharply.
 * TB6612FNG: AIN1 = HIGH, AIN2 = HIGH → Short-circuit brake
 *
 * Note: Hold brake for 300ms then coast to avoid sustained current draw.
 */
void brakeMotors() {
  // Apply active brake — short both motor terminals
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, 255); // Full brake force
  analogWrite(PWMB, 255);

  // Hold the brake for 300ms — robot decelerates sharply
  delay(300);

  // Then coast to release (avoids sustained current draw through H-Bridge)
  coastMotors();
}
