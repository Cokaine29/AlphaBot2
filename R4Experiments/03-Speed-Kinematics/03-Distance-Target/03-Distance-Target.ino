/*
   Experiment 03: Distance Target Execution (R4 WiFi)

   This sketch calculates the required run duration in milliseconds
   for the robot to travel a user-defined distance in centimeters,
   runs the motors, and stops.

   Compatible with Arduino UNO R4 WiFi.
*/

#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>

// --- WIFI CONFIGURATION ---
const char *ssid = "REDMI_NEW";
const char *pass = "password";

// --- OTA CONFIGURATION ---
const char *ota_hostname = "AlphaBot2-R4";
const char *ota_password = "admin";

// H-Bridge Pin Definitions
#define PWMA 6  // Left Motor Speed pin (ENA)
#define AIN2 A0 // Left Motor Direction 2
#define AIN1 A1 // Left Motor Direction 1
#define PWMB 5  // Right Motor Speed pin (ENB)
#define BIN1 A2 // Right Motor Direction 1
#define BIN2 A3 // Right Motor Direction 2

// --- CALIBRATED CONSTANTS ---
// TODO: Input the speed (in cm/s) that you measured for PWM = 120 in Task 1.
// E.g. If the bot went 96 cm in 3 seconds, set this to 32.0
const float CALIBRATED_SPEED_CMS = 32.0;
const int TARGET_PWM = 120;

// --- TARGET DISTANCE ---
// TODO: Set the distance (in centimeters) you want the robot to travel!
const float TARGET_DISTANCE_CM = 150.0;

// Base wheel balance offsets calibrated in Experiment 01
const int LEFT_SPEED_OFFSET = 0;
const int RIGHT_SPEED_OFFSET = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting OTA and Distance Target Setup...");

  // 1. Connect to Wi-Fi
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // 2. Start the ArduinoOTA Listener
  ArduinoOTA.onStart([]() {
    TelnetStream.stop();
  });
  ArduinoOTA.begin(WiFi.localIP(), ota_hostname, ota_password, InternalStorage);
  Serial.println("OTA Listener started.");

  // 3. Start TelnetStream
  TelnetStream.begin();
  Serial.println("Telnet Stream started on port 23.");
  TelnetStream.println("--------------------------------------------------");
  TelnetStream.println("Sketch: R4-03-Distance-Target");
  TelnetStream.println("Status: Setup Completed");
  TelnetStream.println("--------------------------------------------------");

  // Configure motor driver pins
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
  // Calculate run duration: t = d / v
  // Multiply by 1000 to convert seconds to milliseconds
  float runDurationSeconds = TARGET_DISTANCE_CM / CALIBRATED_SPEED_CMS;
  unsigned long runDurationMillis = (unsigned long)(runDurationSeconds * 1000.0);

  // Drive forward at target PWM speed
  TelnetStream.print("[R4-03-Distance-Target] Driving ");
  TelnetStream.print(TARGET_DISTANCE_CM);
  TelnetStream.print(" cm at speed ");
  TelnetStream.print(TARGET_PWM);
  TelnetStream.print(" for ");
  TelnetStream.print(runDurationMillis);
  TelnetStream.println(" ms...");
  moveForward(TARGET_PWM);

  // Run for the computed duration, polling OTA and TelnetStream
  unsigned long runStart = millis();
  while (millis() - runStart < runDurationMillis) {
    ArduinoOTA.poll();
    TelnetStream.available();
    delay(10);
  }

  // Stop the motors
  stopMotors();
  Serial.println("Target run complete. Ready for wireless OTA uploads...");
  TelnetStream.println("[R4-03-Distance-Target] Target run complete. Ready for wireless OTA uploads...");

  // Lock execution forever, but keep polling OTA and TelnetStream
  unsigned long lastLog = 0;
  while (1) {
    ArduinoOTA.poll();
    TelnetStream.available();
    if (millis() - lastLog > 2000) {
      lastLog = millis();
      TelnetStream.println("Heartbeat: [R4-03-Distance-Target] Robot is idle. Ready for OTA uploads.");
    }
    delay(10);
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
