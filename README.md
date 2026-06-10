# AlphaBot2 Arduino IDE Environment & User Guide

Welcome to your self-contained Arduino development environment for the **Waveshare AlphaBot2**! 

This environment is configured directly inside **Antigravity (VS Code)**. You do not need to install the external Arduino IDE to write, compile, flash, or monitor your code.

---

## 🛠️ Environment Shortcuts & Tasks

All commands run via the local `arduino-cli` wrapped in the `manage.ps1` helper script.

| Action | Shortcut / Command | Description |
| :--- | :--- | :--- |
| **Verify/Compile** | `Ctrl + Shift + B` (Default Build Task) | Compiles the sketch directory containing the currently active `.ino` editor file. |
| **Upload/Flash** | Command Palette (`Ctrl+Shift+P`) -> `Run Task` -> `Arduino: Upload/Flash Active Sketch` | Automatically detects the board's COM port, compiles, and flashes the active sketch. |
| **Serial Monitor** | Command Palette (`Ctrl+Shift+P`) -> `Run Task` -> `Arduino: Open Serial Monitor` | Opens a live serial port reader at `115200` baud in a new VS Code terminal. |
| **Auto-Detect Port** | Command Palette (`Ctrl+Shift+P`) -> `Run Task` -> `Arduino: Auto-Detect Board/Port` | Scans for connected boards and writes the COM port to `.arduino-config.json`. |

---

## ⚙️ How to Change Board & Port Settings

The environment settings are located in `.arduino-config.json` in the root of the workspace:
```json
{
  "fqbn": "arduino:avr:uno",
  "port": "COM7",
  "baud": 115200
}
```
- **`fqbn`**: The Fully Qualified Board Name. (Default is `arduino:avr:uno` for Arduino Uno).
- **`port`**: The target COM port (e.g. `COM7`). **If you leave this empty, the helper script will automatically detect the port when uploading!**
- **`baud`**: The baud rate for the Serial Monitor.

---

## 📋 Pin Mapping Reference (AlphaBot2-Ar)

### 1. Motors (TB6612FNG Driver)
| Control Line | Function | Arduino Pin |
| :--- | :--- | :--- |
| `PWMA` | Left Motor Speed (PWM) | **D6** |
| `AIN1` | Left Motor Direction 1 | **A1** |
| `AIN2` | Left Motor Direction 2 | **A0** |
| `PWMB` | Right Motor Speed (PWM) | **D5** |
| `BIN1` | Right Motor Direction 1 | **A2** |
| `BIN2` | Right Motor Direction 2 | **A3** |

### 2. Onboard Peripherals
| Component | Interface / Pin | Description |
| :--- | :--- | :--- |
| **RGB LEDs (x4)** | **D7** | Serial control of 4 x WS2812B RGB LEDs. |
| **Buzzer** | **I2C (SDA/SCL)** | Connected via the **PCF8574** I/O Expander. |
| **Line Sensors (x5)** | **I2C (SDA/SCL)** | Analog outputs read via the **TLC1543** 10-bit AD converter. |
| **Joystick & Keys** | **I2C (SDA/SCL)** | 5-way joystick connected via the **PCF8574** I/O Expander. |
| **OLED Screen** | **I2C (SDA/SCL)** | 0.96-inch SSD1306 OLED display. |

---

## 📚 AlphaBot2 Projects Guide (All 15 Demos)

All demo codes are located inside the `Arduino/demo/` folder. Open any `.ino` file in these folders and press `Ctrl+Shift+B` to compile.

### 1. Motor Test (`Arduino/demo/Run_Test`)
- **Phenomenon**: The car moves forward.
- **Direction Correction**: If a wheel turns backward:
  - If **left wheel** rotates wrong: swap `AIN1` and `AIN2` pin numbers in the code.
  - If **right wheel** rotates wrong: swap `BIN1` and `BIN2` pin numbers in the code.

### 2. Five-direction Remote Sensing (`Arduino/demo/Joystick`)
- **Phenomenon**: Uses the joystick on the top board. Pressing Up/Down/Left/Right spins the motors accordingly, and pressing OK sounds the buzzer.
- **Monitor**: Open the Serial Monitor (`115200` baud) to see the key state printed in real-time.

### 3. Infrared Remote Control Car (`Arduino/demo/IR`)
- **Phenomenon**: Controls the car using an IR remote.
- **Keys**: 
  - `2` (Forward), `8` (Backward), `4` (Turn Left), `6` (Turn Right), `5` (Stop).
  - `-` / `+`: Decrease/increase speed.
  - `EQ`: Restore default speed.

### 4. Infrared Obstacle Avoidance (`Arduino/demo/Infrared-Obstacle-Avoidance`)
- **Phenomenon**: Car goes straight until it detects an obstacle, then turns right. Front green LEDs light up when an obstacle is detected.
- **Calibration**: If the front LEDs are always on or always off, adjust the two potentiometers on the bottom chassis.

### 5. Ultrasonic Ranging (`Arduino/demo/Ultrasionc_Ranging`)
- **Phenomenon**: Measures distances using the ultrasonic sensor.
- **Monitor**: Displays the distance in centimeters in the Serial Monitor.

### 6. Ultrasonic Obstacle Avoidance (`Arduino/demo/Ultrasionc-Obstacle-Avoidance`)
- **Phenomenon**: Car runs forward, measuring distance. If an obstacle is detected, it turns right to avoid it.

### 7. Ultrasonic IR Obstacle Avoidance (`Arduino/demo/Ultrasionc-Infrared-Obstacle-Avoidance`)
- **Phenomenon**: Combines both ultrasonic ranging and front IR sensors to navigate obstacle courses.

### 8. Tracking Sensor Test (`Arduino/demo/TRSensorExample`)
- **Calibration (CRITICAL)**: When you run this program, pick up the car and **shake/swipe it left and right over a black line on white paper/KT board**. This allows the sensor to record the minimum (white) and maximum (black) reflective values.
- **Monitor**: After calibration, it displays the calibration bounds and prints real-time sensor states. The last column shows the line position:
  - `2000`: Black line is centered.
  - `0`: Line is on the far left.
  - `4000`: Line is on the far right.

### 9. IR Tracking Demo (`Arduino/demo/Infrared-Line-Tracking`)
- **Phenomenon**: Calibrates first, then automatically follows a black line track.
- **Action**: Place the car on the track. When starting, hold it and sweep it left and right over the line to calibrate. Once calibrated, the wheels will begin spinning. Let go, and the robot will track the line!

### 10. RGB LED (`Arduino/demo/W2812`)
- **Phenomenon**: Cycles the 4 WS2812B LEDs at the bottom through Red, Green, Blue, and Yellow colors.

### 11. OLED (`Arduino/demo/oled`)
- **Phenomenon**: Demonstrates text rendering and geometric drawings (lines, circles, rectangles) on the 0.96-inch screen.

### 12. Comprehensive Program (`Arduino/demo/Line-Tracking`)
- **Phenomenon**: Fully integrated application.
  1. Displays `AlphaBot2` on OLED.
  2. Placing the car on the black line and pressing the button initiates auto-calibration (RGB LEDs turn Green).
  3. RGB turns Blue when calibration finishes. The OLED displays a `**` slider indicating the black line position.
  4. Pressing the button again starts line tracking. The car stops on obstacles and beeps, then resumes when clear. It halts if picked up.

### 13. Smart Car Walks the Maze (`Arduino/demo/MazeSolver`)
- **Phenomenon**: Solves line-maze layouts without loops (using right-angle turns).
  1. First Run: Automatically calibrates, moves through the maze, exploring corners and turning around until it finds the endpoint, mapping the shortest path.
  2. Second Run: Put the car back at the starting point, press the key, and it runs directly to the endpoint using the shortest path!

### 14. Control the Car By Bluetooth (`Arduino/demo/Bluetooth`)
- **⚠️ CRITICAL**: **You must unplug the Bluetooth module during flashing/uploading!** The module shares serial pins `D0` and `D1` with the USB interface; uploading while it is connected will fail.
- **APP**: Scan the wiki QR code to download the app. Scan for Bluetooth devices:
  - iOS: select `Waveshare_BLE`.
  - Android: select `Waveshare_ERD` (or MAC address ending in `00:0E:0E`).
- Choose **Remote Mode** to control speed and direction.

### 15. Bluetooth JSON (`Arduino/demo/Bluetooth-json`)
- **Phenomenon**: Advanced Bluetooth control that accepts JSON packets to control the buzzer, movement, and RGB LED colors via the app's Peripheral Control panel.
