#include <Arduino.h>

#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Motor-L forward (IN2).
#define AIN1 A1 // Motor-L backward (IN1)
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Motor-R forward (IN3)
#define BIN2 A3 // Motor-R backward (IN4)

// =======================================================
// EDIT THESE VALUES TO PERFECTLY CALIBRATE YOUR BOT
// =======================================================
// If the bot veers LEFT, the left wheel is slower. Increase LEFT_SPEED_OFFSET.
// If the bot veers RIGHT, the right wheel is slower. Increase RIGHT_SPEED_OFFSET.
#define LEFT_SPEED_OFFSET 1
#define RIGHT_SPEED_OFFSET 0

// The base speed the bot will travel at during calibration
const int base_speed = 40;

void setup() {
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT); 
  pinMode(BIN2, OUTPUT); 
  
  // Ensure motors are stopped initially
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  
  // Set motor controller direction to FORWARD
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  // Wait 5 seconds so you can place the bot on a flat surface
  delay(4000);
}

void loop() {
  // Continuously apply the base speed + your calibration offsets
  analogWrite(PWMA, constrain(base_speed + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(base_speed + RIGHT_SPEED_OFFSET, 0, 255));
  
  delay(10);
}
