# 🧪 Experiment 04: I2C I/O Expansion & Joystick Control

## 🎯 Objectives
1. Understand how **I2C communication** is used to expand GPIO pins using the **PCF8574 I/O Expander**.
2. Learn how quasi-bidirectional pins are configured as inputs using software pull-ups.
3. Read the state of a 5-way joystick over I2C and trigger motor directions and RGB LED colors accordingly.

---

## 🛠️ Theoretical Background

### 1. What is I2C?
I2C (Inter-Integrated Circuit) is a synchronous, multi-controller/multi-target, packet-switched, single-ended, serial communication bus. It uses only two wires:
* **SDA (Serial Data)**: For transmitting data bits.
* **SCL (Serial Clock)**: For synchronizing the data transmission.

On the Arduino Uno, SDA is on analog pin **A4** and SCL is on analog pin **A5**.

### 2. GPIO Expansion via PCF8574
An Arduino Uno has a limited number of GPIO pins. To connect line sensors, motors, NeoPixels, a buzzer, and a joystick, the AlphaBot2 uses a **PCF8574 I/O Expander** chip connected to the I2C bus. 
The PCF8574 gives us 8 additional digital pins (P0 to P7) controlled using just the 2 I2C wires.

On the AlphaBot2, these 8 pins are mapped as follows:
* **P0**: Joystick UP
* **P1**: Joystick RIGHT
* **P2**: Joystick LEFT
* **P3**: Joystick DOWN
* **P4**: Joystick CENTER (OK)
* **P5**: Buzzer Control (Active LOW: write 0 to beep, write 1 to stop)
* **P6 & P7**: Unused

The I2C address of this expander chip is fixed at **0x20**.

### 3. Reading Quasi-Bidirectional Input Pins
To read a pin on the PCF8574 as an input, we must first write a binary **1** (HIGH) to that pin to enable its internal weak pull-up resistor. 
The joystick buttons are normally open. When you press the joystick in a direction, it shorts the corresponding pin to Ground (0V / LOW).

When we read a byte from the PCF8574:
* If a joystick pin reads **1** (HIGH): Button is **released**.
* If a joystick pin reads **0** (LOW): Button is **pressed**.

---

## 🧭 I2C Data Structures & Binary Masks

The read byte has the format: `P7 P6 P5 P4 P3 P2 P1 P0`.
Since we only care about the joystick pins (P0 to P4), we mask out the top 3 bits by bitwise ORing with `0xE0` (binary `11100000`). This sets the top bits to 1, letting us focus on the lower 5 bits:

| Pressed Direction | Pin State (P4 P3 P2 P1 P0) | Hex Value (with 0xE0 mask) | Binary Value |
| :--- | :---: | :---: | :---: |
| **UP (P0)** | `1 1 1 1 0` | **0xFE** | `11111110` |
| **RIGHT (P1)** | `1 1 1 0 1` | **0xFD** | `11111101` |
| **LEFT (P2)** | `1 1 0 1 1` | **0xFB** | `11111011` |
| **DOWN (P3)** | `1 0 1 1 1` | **0xF7** | `11110111` |
| **CENTER (P4)** | `0 1 1 1 1` | **0xEF** | `11101111` |
| **NONE** | `1 1 1 1 1` | **0xFF** | `11111111` |

---

## 📂 Experiment Files

* **[04-Joystick-I2C.ino](file:///f:/AlphaBot2/R4Experiments/04-Joystick-I2C/04-Joystick-I2C.ino)**: Read joystick buttons over I2C, output status to Serial Monitor, ring the buzzer, and control motor movements and NeoPixel colors.

---

## 🧠 Software Control Flow

1. **Write Input Pull-ups**: Send a byte to the PCF8574 with 1s on bits P0-P4 to enable input reading.
   `PCF8574Write(0x1F | PCF8574Read())`
2. **Read Port Byte**: Read the 8-bit state from the chip.
   `value = PCF8574Read() | 0xE0`
3. **Check Press**: If `value != 0xFF`, a button is pressed.
4. **Trigger Actions**:
   * Turn ON buzzer: Write 0 to pin P5.
   * Move motors (Forward/Backward/Left/Right).
   * Update RGB NeoPixel headlight colors to match the direction.
5. **Debounce / Release Wait**: Loop until the button is released (`value == 0xFF`), then stop motors, turn off buzzer, and reset lights.

---

## 📝 Lab Procedure & Student Tasks

> [!IMPORTANT]
> To ensure the robot runs immediately on boot without networking delay or connection timeouts, Wi-Fi, OTA, and Telnet Stream have been disabled in this sketch. The robot will read and react to joystick inputs immediately upon boot/reset. Status monitoring is done entirely over the **USB Serial Monitor** (115200 baud).

### Task 1: Upload and Verify Joystick Inputs
1. Open [04-Joystick-I2C.ino](file:///f:/AlphaBot2/R4Experiments/04-Joystick-I2C/04-Joystick-I2C.ino).
2. Upload the code to the robot using PowerShell:
   ```powershell
   .\manage.ps1 upload R4Experiments\04-Joystick-I2C
   ```
3. Open the **Serial Monitor** at **115200 baud**.
4. Move the joystick in all 5 directions. Verify that the correct direction is printed in the Serial Monitor and the buzzer sounds.
5. Check that the NeoPixel LEDs light up:
   * **UP** -> Green
   * **DOWN** -> Red
   * **LEFT** -> Yellow
   * **RIGHT** -> Blue
   * **CENTER** -> Cyan (Light Blue)

### Task 2: Change Speed Settings
Modify the `Speed` variable in the sketch and observe how the joystick responsiveness is maintained.

---

## ❓ Post-Lab Questions for Students
1. Why does the I2C bus only require 2 pins regardless of how many expanders or sensors you add to it?
2. What is the difference between active-high and active-low logic? Why does setting pin P5 to `0` (LOW) turn the buzzer ON?
3. Why did we write `0x1F` to the PCF8574 before reading the joystick values? What would happen if we omitted that step?
