/*
   Experiment 05: Infrared (IR) Communication & NEC Protocol Decodes (R4 WiFi)

   This sketch reads the infrared remote control signal using digital
   pin D4. It uses a custom bit-bang decoder to decode the NEC protocol,
   checks checksum integrity, and navigates the robot at adjustable speeds.

   Decoded bytes and operations are logged to the Serial Monitor at 115200 baud.

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

// IR Receiver Pin
#define IR_PIN 4

// NEC Protocol Command Key Mapping
#define KEY2 0x18       // Forward
#define KEY8 0x52       // Backward
#define KEY4 0x08       // Left
#define KEY6 0x5A       // Right
#define KEY5 0x1C       // Stop
#define SpeedDown 0x07  // VOL-
#define SpeedUp 0x15    // VOL+
#define ResetSpeed 0x09 // EQ
#define Repeat 0xFF     // Continuous button hold

// Calibration speed offsets
const int LEFT_SPEED_OFFSET = 0;
const int RIGHT_SPEED_OFFSET = 0;

int currentSpeed = 150; // Initial default speed
unsigned long lastPacketTime = 0;
unsigned char decodedCommand;
bool activeMovement = false;

// Function Prototypes
bool IR_decode(unsigned char *code);
void executeIRCommand();
void forward();
void backward();
void left();
void right();
void stopMotors();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("AlphaBot2 Experiment 05 - IR Control starting...");

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
  TelnetStream.println("Sketch: R4-05-IR-Remote");
  TelnetStream.println("Status: Setup Completed");
  TelnetStream.println("--------------------------------------------------");

  pinMode(IR_PIN, INPUT);

  // Configure motor driver pins
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  stopMotors();
  Serial.println("Setup Complete. Point the remote and press a key!");
}

void loop() {
  // Constantly check/poll for incoming Wi-Fi firmware packets and Telnet client status
  ArduinoOTA.poll();
  TelnetStream.available();

  // Periodic heartbeat when idle (no active movement)
  static unsigned long lastLog = 0;
  if (!activeMovement && millis() - lastLog > 2000) {
    lastLog = millis();
    TelnetStream.println("Heartbeat: [R4-05-IR-Remote] Robot is idle. Ready for OTA uploads.");
  }

  // Check if a new IR command frame has been received
  if (IR_decode(&decodedCommand)) {
    activeMovement = true;
    lastPacketTime = millis(); // Refresh packet timestamp
    executeIRCommand();
  } else {
    // If we were moving, but have not received any new command or repeat signal
    // for more than 150ms, assume the button was released and stop the robot.
    if (activeMovement) {
      if (millis() - lastPacketTime > 150) {
        activeMovement = false;
        stopMotors();
        Serial.println("Stop (Key Released)");
        TelnetStream.println("[R4-05-IR-Remote] Stop (Key Released)");
      }
    }
  }
}

/**
 * Custom bit-bang decoder for the standard NEC IR Protocol.
 * Checks start leader pulse, decodes 32 data bits using pulse distance timing,
 * and validates transmission using inverse checksum verification.
 */
bool IR_decode(unsigned char *code) {
  unsigned int count = 0;
  unsigned char i;
  unsigned char index = 0;
  unsigned char bitCount = 0;
  unsigned char data[4] = {0, 0, 0, 0};

  // The receiver pin is normally HIGH. A signal pulls it LOW (leader pulse).
  if (digitalRead(IR_PIN) == LOW) {
    // 1. Verify 9ms leader LOW pulse (9000us / 60us = 150 cycles)
    count = 0;
    while (digitalRead(IR_PIN) == LOW && count++ < 200) {
      delayMicroseconds(60);
    }
    if (count < 80) return false; // Too short to be a valid leader pulse

    // 2. Verify 4.5ms leader HIGH pulse (4500us / 60us = 75 cycles)
    count = 0;
    while (digitalRead(IR_PIN) == HIGH && count++ < 100) {
      delayMicroseconds(60);
    }

    // Check for Repeat Code (approx 2.25ms HIGH instead of 4.5ms)
    if (count > 25 && count < 55) {
      *code = Repeat;
      return true;
    }
    if (count < 55) return false; // Invalid preamble

    // 3. Decode 32 data bits (4 bytes)
    for (i = 0; i < 32; i++) {
      // Wait for the end of the constant 562.5us LOW spacer
      count = 0;
      while (digitalRead(IR_PIN) == LOW && count++ < 25) {
        delayMicroseconds(60);
      }

      // Measure the duration of the HIGH data interval
      count = 0;
      while (digitalRead(IR_PIN) == HIGH && count++ < 50) {
        delayMicroseconds(60);
      }

      // If the HIGH phase was longer than 20 cycles (~1.2ms), it is a logical '1'
      if (count > 20) {
        data[index] |= (1 << bitCount);
      }

      // Move to the next byte after collecting 8 bits
      if (bitCount == 7) {
        bitCount = 0;
        index++;
      } else {
        bitCount++;
      }
    }

    // 4. Validate Data Integrity Checksums
    // Address (data[0]) + Inverse Address (data[1]) must equal 255 (0xFF)
    // Command (data[2]) + Inverse Command (data[3]) must equal 255 (0xFF)
    if ((data[0] + data[1] == 0xFF) && (data[2] + data[3] == 0xFF)) {
      *code = data[2]; // Return the successfully validated command byte
      return true;
    }
  }
  return false;
}

/**
 * Triggers actions based on the validated hexadecimal code
 */
void executeIRCommand() {
  switch (decodedCommand) {
    case KEY2:
      Serial.println("IR Cmd: FORWARD");
      TelnetStream.println("[R4-05-IR-Remote] IR Cmd: FORWARD");
      forward();
      break;
    case KEY4:
      Serial.println("IR Cmd: LEFT");
      TelnetStream.println("[R4-05-IR-Remote] IR Cmd: LEFT");
      left();
      break;
    case KEY5:
      Serial.println("IR Cmd: STOP");
      TelnetStream.println("[R4-05-IR-Remote] IR Cmd: STOP");
      stopMotors();
      break;
    case KEY6:
      Serial.println("IR Cmd: RIGHT");
      TelnetStream.println("[R4-05-IR-Remote] IR Cmd: RIGHT");
      right();
      break;
    case KEY8:
      Serial.println("IR Cmd: BACKWARD");
      TelnetStream.println("[R4-05-IR-Remote] IR Cmd: BACKWARD");
      backward();
      break;
    case SpeedUp:
      currentSpeed += 15;
      currentSpeed = constrain(currentSpeed, 0, 250);
      Serial.print("IR Cmd: SPEED UP -> Current Speed: ");
      Serial.println(currentSpeed);
      TelnetStream.print("[R4-05-IR-Remote] IR Cmd: SPEED UP -> Current Speed: ");
      TelnetStream.println(currentSpeed);
      break;
    case SpeedDown:
      currentSpeed -= 15;
      currentSpeed = constrain(currentSpeed, 0, 250);
      Serial.print("IR Cmd: SPEED DOWN -> Current Speed: ");
      Serial.println(currentSpeed);
      TelnetStream.print("[R4-05-IR-Remote] IR Cmd: SPEED DOWN -> Current Speed: ");
      TelnetStream.println(currentSpeed);
      break;
    case ResetSpeed:
      currentSpeed = 150;
      Serial.print("IR Cmd: RESET SPEED -> Current Speed: ");
      Serial.println(currentSpeed);
      TelnetStream.print("[R4-05-IR-Remote] IR Cmd: RESET SPEED -> Current Speed: ");
      TelnetStream.println(currentSpeed);
      break;
    case Repeat:
      // When key is held down, do not log or change anything, just maintain current state
      break;
    default:
      Serial.print("IR Cmd: UNKNOWN CODE (Hex: 0x");
      Serial.println(decodedCommand, HEX);
      TelnetStream.print("[R4-05-IR-Remote] IR Cmd: UNKNOWN CODE (Hex: 0x");
      TelnetStream.println(decodedCommand, HEX);
      break;
  }
}

/* --- Motor Drive Functions --- */

void forward() {
  int leftSpeed = constrain(currentSpeed + LEFT_SPEED_OFFSET, 0, 255);
  int rightSpeed = constrain(currentSpeed + RIGHT_SPEED_OFFSET, 0, 255);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, leftSpeed);
  analogWrite(PWMB, rightSpeed);
}

void backward() {
  int leftSpeed = constrain(currentSpeed + LEFT_SPEED_OFFSET, 0, 255);
  int rightSpeed = constrain(currentSpeed + RIGHT_SPEED_OFFSET, 0, 255);

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  analogWrite(PWMA, leftSpeed);
  analogWrite(PWMB, rightSpeed);
}

void right() {
  // Spin turn CW
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  analogWrite(PWMA, 80);
  analogWrite(PWMB, 80);
}

void left() {
  // Spin turn CCW
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, 80);
  analogWrite(PWMB, 80);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
