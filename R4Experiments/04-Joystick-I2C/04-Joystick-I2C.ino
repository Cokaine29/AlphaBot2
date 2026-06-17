/*
   Experiment 04: I2C I/O Expansion & Joystick Control (R4 WiFi)

   This sketch reads the 5-way joystick on the AlphaBot2 using the
   PCF8574 I2C I/O expander. Pressing the joystick moves the motors,
   sounds the buzzer, and changes the NeoPixel LED headlight colors.

   All actions are reported live to the Serial Monitor at 115200 baud.

   Compatible with Arduino UNO R4 WiFi.
*/

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
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

// NeoPixel LED Pin & count
#define RGB_PIN 7
#define NUM_LEDS 4

// PCF8574 I2C Address
#define PCF8574_ADDR 0x20

// Speed offsets to calibrate wheel imbalance
const int LEFT_SPEED_OFFSET = 0;
const int RIGHT_SPEED_OFFSET = 0;

// Motor speed during manual joystick control
const int JOYSTICK_SPEED = 80;

Adafruit_NeoPixel RGB = Adafruit_NeoPixel(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);
byte value;

// Function Prototypes
void PCF8574Write(byte data);
byte PCF8574Read();
void setBuzzer(bool state);
void setNeoPixelColor(uint32_t color);

void forward();
void backward();
void left();
void right();
void stopMotors();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("AlphaBot2 Experiment 04 - Joystick starting...");

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
  TelnetStream.println("Sketch: R4-04-Joystick-I2C");
  TelnetStream.println("Status: Setup Completed");
  TelnetStream.println("--------------------------------------------------");

  Wire.begin();

  // Initialize NeoPixel LEDs
  RGB.begin();
  setNeoPixelColor(RGB.Color(0, 0, 0)); // Turn off

  // Configure motor control pins
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  stopMotors();
  setBuzzer(false); // Make sure buzzer is silent

  Serial.println("Setup Complete. Move the joystick to control the robot!");
}

void loop() {
  // Constantly check/poll for incoming Wi-Fi firmware packets and Telnet client status
  ArduinoOTA.poll();
  TelnetStream.available();

  // Periodic heartbeat when idle (no button pressed)
  static unsigned long lastLog = 0;
  if (value == 0xFF && millis() - lastLog > 2000) {
    lastLog = millis();
    TelnetStream.println("Heartbeat: [R4-04-Joystick-I2C] Robot is idle. Ready for OTA uploads.");
  }

  // 1. Enable inputs (write 1 to pins P0-P4)
  PCF8574Write(0x1F | PCF8574Read());

  // 2. Read input pins and mask out unused top bits
  value = PCF8574Read() | 0xE0;

  // 3. Check if any button is pressed (0xFF means no button is pressed)
  if (value != 0xFF) {
    setBuzzer(true); // Turn buzzer ON

    switch (value) {
      case 0xFE: // P0 is LOW (UP)
        Serial.println("Joystick: UP -> Driving FORWARD");
        TelnetStream.println("[R4-04-Joystick-I2C] Joystick: UP -> Driving FORWARD");
        setNeoPixelColor(RGB.Color(0, 255, 0)); // Green
        forward();
        break;

      case 0xFD: // P1 is LOW (RIGHT)
        Serial.println("Joystick: RIGHT -> Pivoting RIGHT");
        TelnetStream.println("[R4-04-Joystick-I2C] Joystick: RIGHT -> Pivoting RIGHT");
        setNeoPixelColor(RGB.Color(0, 0, 255)); // Blue
        right();
        break;

      case 0xFB: // P2 is LOW (LEFT)
        Serial.println("Joystick: LEFT -> Pivoting LEFT");
        TelnetStream.println("[R4-04-Joystick-I2C] Joystick: LEFT -> Pivoting LEFT");
        setNeoPixelColor(RGB.Color(255, 255, 0)); // Yellow (Red + Green)
        left();
        break;

      case 0xF7: // P3 is LOW (DOWN)
        Serial.println("Joystick: DOWN -> Driving BACKWARD");
        TelnetStream.println("[R4-04-Joystick-I2C] Joystick: DOWN -> Driving BACKWARD");
        setNeoPixelColor(RGB.Color(255, 0, 0)); // Red
        backward();
        break;

      case 0xEF: // P4 is LOW (CENTER/OK)
        Serial.println("Joystick: CENTER -> Driving FORWARD (Alert Mode)");
        TelnetStream.println("[R4-04-Joystick-I2C] Joystick: CENTER -> Driving FORWARD (Alert Mode)");
        setNeoPixelColor(RGB.Color(0, 255, 255)); // Cyan
        forward();
        break;

      default:
        Serial.print("Joystick: UNKNOWN combination (Hex: 0x");
        Serial.println(value, HEX);
        TelnetStream.print("[R4-04-Joystick-I2C] Joystick: UNKNOWN combination (Hex: 0x");
        TelnetStream.println(value, HEX);
        break;
    }

    // 4. Wait until the joystick button is released
    while (value != 0xFF) {
      // Make sure OTA and Telnet work even while holding down the joystick
      ArduinoOTA.poll();
      TelnetStream.available();
      PCF8574Write(0x1F | PCF8574Read());
      value = PCF8574Read() | 0xE0;
      delay(10); // 10ms debounce delay
    }

    // 5. Button was released -> Stop everything
    Serial.println("Joystick: RELEASED -> Stopping motors");
    TelnetStream.println("[R4-04-Joystick-I2C] Joystick: RELEASED -> Stopping motors");
    stopMotors();
    setBuzzer(false); // Turn buzzer OFF
    setNeoPixelColor(RGB.Color(0, 0, 0)); // Turn off NeoPixels
  }
}

/**
 * Sends a byte of data to the PCF8574 expander over I2C
 */
void PCF8574Write(byte data) {
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(data);
  Wire.endTransmission();
}

/**
 * Reads a byte of data from the PCF8574 expander over I2C
 */
byte PCF8574Read() {
  int data = -1;
  Wire.requestFrom(PCF8574_ADDR, 1);
  if (Wire.available()) {
    data = Wire.read();
  }
  return data;
}

/**
 * Controls the active-low Buzzer on pin P5 of the PCF8574
 */
void setBuzzer(bool state) {
  byte currentPortState = PCF8574Read();
  if (state) {
    // Turn ON buzzer (set P5 to 0)
    PCF8574Write(0xDF & currentPortState); // 0xDF is 11011111 binary
  } else {
    // Turn OFF buzzer (set P5 to 1)
    PCF8574Write(0x20 | currentPortState); // 0x20 is 00100000 binary
  }
}

/**
 * Sets all 4 NeoPixel LEDs to a uniform color
 */
void setNeoPixelColor(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    RGB.setPixelColor(i, color);
  }
  RGB.show();
}

/* --- Motor Drive Functions --- */

void forward() {
  int leftSpeed = constrain(JOYSTICK_SPEED + LEFT_SPEED_OFFSET, 0, 255);
  int rightSpeed = constrain(JOYSTICK_SPEED + RIGHT_SPEED_OFFSET, 0, 255);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, leftSpeed);
  analogWrite(PWMB, rightSpeed);
}

void backward() {
  int leftSpeed = constrain(JOYSTICK_SPEED + LEFT_SPEED_OFFSET, 0, 255);
  int rightSpeed = constrain(JOYSTICK_SPEED + RIGHT_SPEED_OFFSET, 0, 255);

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  analogWrite(PWMA, leftSpeed);
  analogWrite(PWMB, rightSpeed);
}

void right() {
  // Pivot CW (Left forward, Right backward) at lower speed for stability
  int turnSpeed = constrain(JOYSTICK_SPEED / 2, 30, 255);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  analogWrite(PWMA, turnSpeed);
  analogWrite(PWMB, turnSpeed);
}

void left() {
  // Pivot CCW (Left backward, Right forward) at lower speed for stability
  int turnSpeed = constrain(JOYSTICK_SPEED / 2, 30, 255);

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, turnSpeed);
  analogWrite(PWMB, turnSpeed);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}
