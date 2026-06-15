# 📡 Setting Up Wireless (OTA) Uploads on Arduino UNO R4 WiFi

This guide outlines how to configure the **Arduino UNO R4 WiFi** to accept wireless sketch uploads over your local Wi-Fi network (Over-the-Air, or OTA), eliminating the need for a physical USB cable during testing.

We use the third-party **ArduinoOTA** library by Juraj Andrassy, which operates locally and does not require cloud registrations.

---

## 🛠️ Step-by-Step Setup Guide

### Step 1: Install the ArduinoOTA Library
1. Open the Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries...**
3. Search for **`ArduinoOTA`** (specifically the one written by **Juraj Andrassy**).
4. Click **Install**.

---

### Step 2: Configure the Board Package for Network Uploads
Because the official Renesas board package does not support network uploads by default, the `ArduinoOTA` library requires you to copy a local configuration file into your Arduino hardware directory:

1. Locate the Arduino board package path on your computer. On Windows, it is typically:
   `C:\Users\<Your-Username>\AppData\Local\Arduino15\packages\arduino\hardware\renesas_uno\<Version-Number>\`
2. Open the `ArduinoOTA` library installation directory (usually in your `Documents/Arduino/libraries/ArduinoOTA/extras/`).
3. Copy the file **`platform.local.txt`** from the library's `extras/` folder.
4. Paste it directly into the `renesas_uno/<Version-Number>/` folder you located in Step 1.
5. Restart your Arduino IDE.

---

### Step 3: Flash the Initial OTA Sketch (Via USB)
1. Open the [00-WiFi-OTA.ino](file:///f:/AlphaBot2/R4Experiments/00-WiFi-OTA/00-WiFi-OTA.ino) sketch.
2. Replace `"YOUR_WIFI_SSID"` and `"YOUR_WIFI_PASSWORD"` with your local Wi-Fi credentials.
3. Connect the robot to your computer using a USB cable.
4. Upload the sketch.
5. Open the **Serial Monitor** at **115200 baud** and verify that the board connects to your Wi-Fi and prints its local IP address (e.g., `192.168.1.50`).

Once the IP is printed, you can disconnect the USB cable.

---

### Step 4: Perform Wireless Uploads
1. Ensure your computer and the AlphaBot2 are connected to the **same Wi-Fi network**.
2. In the Arduino IDE, go to **Tools > Port**.
3. Under the **Network Ports** section, you should see:
   `AlphaBot2-R4 at 192.168.1.50` (or whichever IP address was assigned).
4. Select this network port.
5. Write your robot code (ensuring you keep `ArduinoOTA.poll();` in the `loop()` function so it continues listening for wireless uploads!).
6. Click **Upload**.
7. The IDE will prompt you for the password. Enter **`admin`** (defined as `ota_password` in the sketch).
8. The code will compile and upload wirelessly! The robot will automatically reset and start running your new code.

---

## ⚠️ Critical Maintenance Rules
To keep OTA active on your robot, **every sketch you write from now on must include the OTA boilerplate code**:
1. `#include <ArduinoOTA.h>` at the top.
2. `ArduinoOTA.begin(...)` in `setup()`.
3. `ArduinoOTA.poll();` in `loop()`.

If you upload a sketch that *does not* call `ArduinoOTA.poll()`, the robot will execute the code, but you will lose the wireless connection and must plug the USB cable back in to restore OTA!
