/*
   Experiment 06: Ultrasonic Ranging & Basic Collision Avoidance (R4 WiFi - WiFi/OTA Disabled)

   This sketch measures physical distance using the ultrasonic sensor on pins D2 (ECHO) 
   and D3 (TRIG). The measured distance in centimeters is printed to the Serial Monitor.
   
   It also initializes the motor control pins so students can implement autonomous collision 
   avoidance logic in their tasks.

   All data is outputted over the USB Serial connection at 115200 baud.

   Compatible with Arduino UNO R4 WiFi.
*/

// Ultrasonic Sensor Pins
#define ECHO 2
#define TRIG 3

// H-Bridge Pin Definitions (initialized for future tasks)
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

int distance = 0;
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 250; // Measure distance every 250ms

// Function Prototypes
int readDistance();
void stopMotors();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("AlphaBot2 Experiment 06 - Ultrasonic Ranging starting (Wi-Fi/OTA Disabled)...");

  // Configure Ultrasonic pins
  pinMode(ECHO, INPUT);
  pinMode(TRIG, OUTPUT);

  // Configure H-bridge pins
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Ensure motors start stopped
  stopMotors();

  Serial.println("Setup Complete. Place obstacles in front of the sensor to measure distance.");
}

void loop() {
  // Non-blocking timing loop to read distance periodically
  if (millis() - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = millis();
    distance = readDistance();

    // Log the distance to the Serial Monitor
    if (distance > 2 && distance < 400) {
      Serial.print("Distance = ");
      Serial.print(distance);
      Serial.println(" cm");
    } else {
      Serial.println("!!! Out of range");
    }
  }
}

/**
 * Sends a trigger pulse to the ultrasonic sensor and returns the measured distance in cm.
 */
int readDistance() {
  // 1. Ensure a clean LOW signal on the Trigger pin first
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  // 2. Pulse the Trigger pin HIGH for 10 microseconds to emit sound burst
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // 3. Measure the duration (in microseconds) of the Echo pin HIGH pulse
  // Note: pulseIn has a default timeout of 1 second, but we can shorten it to 30000us (~5m max range) for responsiveness.
  float pulseDuration = pulseIn(ECHO, HIGH, 30000);

  // If pulseIn timed out, return -1 (out of range)
  if (pulseDuration == 0) {
    return -1;
  }

  // 4. Convert travel time to physical distance (in cm)
  // Distance = (Duration * Speed of Sound) / 2
  // For Speed of Sound = 343 m/s (0.0343 cm/us), Distance = Duration / 58.3
  float physicalDistance = pulseDuration / 58.0;

  return (int)physicalDistance;
}

/**
 * Commands both motors to stop
 */
void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
