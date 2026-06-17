/*
   Experiment 02: Right-Turn (R4 WiFi)

   This sketch drives the AlphaBot2 in a Right Pivot Turn (on the spot).
   Students will calibrate the TURN_DURATION_MS constant so that the
   robot turns EXACTLY 90 degrees clockwise.

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

// --- CALIBRATION ---
// TODO: Adjust this delay (in milliseconds) until the turn is exactly 90 degrees!
const unsigned long TURN_DURATION_MS = 200; // Estimated baseline
const int TURN_SPEED = 60;                  // Fixed turning speed

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting OTA and Right Turn Setup...");

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
  TelnetStream.println("Sketch: R4-02-Right-Turn");
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
  delay(1000); // Wait 1 second before starting
}

void loop() {
  // Execute pivot right turn
  TelnetStream.println("[R4-02-Right-Turn] Executing right turn...");
  pivotRight(TURN_SPEED);

  // Keep turning for the calibrated duration, polling OTA and TelnetStream
  unsigned long turnStart = millis();
  while (millis() - turnStart < TURN_DURATION_MS) {
    ArduinoOTA.poll();
    TelnetStream.available();
    delay(10);
  }

  // Stop the motors
  stopMotors();
  Serial.println("Turn complete. Ready for wireless OTA uploads...");
  TelnetStream.println("[R4-02-Right-Turn] Turn complete. Ready for wireless OTA uploads...");

  // Hold execution forever, but keep polling OTA and TelnetStream
  unsigned long lastLog = 0;
  while (1) {
    ArduinoOTA.poll();
    TelnetStream.available();
    if (millis() - lastLog > 2000) {
      lastLog = millis();
      TelnetStream.println("Heartbeat: [R4-02-Right-Turn] Robot is idle. Ready for OTA uploads.");
    }
    delay(10);
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
