/*
   Experiment 00: Arduino UNO R4 WiFi OTA (Over-The-Air) Bootloader
   
   This sketch connects to your local Wi-Fi network and starts the ArduinoOTA
   listener. Once uploaded via USB, it enables subsequent firmware uploads 
   to happen wirelessly over Wi-Fi.
   
   Prerequisites:
   - Install the "ArduinoOTA" library by Juraj Andrassy in the Arduino IDE.
*/

#include <WiFiS3.h>
#include <ArduinoOTA.h>

// --- WIFI CONFIGURATION ---
// TODO: Replace with your network credentials!
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";

// --- OTA CONFIGURATION ---
const char* ota_hostname = "AlphaBot2-R4";
const char* ota_password = "admin"; // Password for uploading code

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting OTA Setup...");

  // 1. Connect to Wi-Fi
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // 2. Start the ArduinoOTA Listener
  // Parameters: Local IP, MDNS Hostname, Auth Password, Target Storage
  ArduinoOTA.begin(WiFi.localIP(), ota_hostname, ota_password, InternalStorage);
  
  Serial.println("OTA Listener started.");
  Serial.println("You can now upload code wirelessly via your IDE's Network Ports.");
}

void loop() {
  // 3. Constantly check/poll for incoming Wi-Fi firmware packets
  ArduinoOTA.poll();

  // Student motor code or other functions should be integrated here!
}
