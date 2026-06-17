# 🧪 Experiment 05: Infrared (IR) Communication & NEC Protocol Decodes

## 🎯 Objectives
1. Understand the physics of infrared (IR) light communication and 38 kHz carrier modulation.
2. Analyze the structure of the **NEC IR Transmission Protocol** (Preamble, Address, Command, and Bit Coding).
3. Implement a custom bit-bang IR decoder in Arduino to read remote control key presses and navigate the robot.

---

## 🛠️ Theoretical Background

### 1. How Infrared Remotes Work
An IR remote control sends data by pulsing an infrared LED on and off. To prevent interference from ambient light (such as sunlight or fluorescent bulbs), the remote's signal is modulated at a carrier frequency of **38 kHz**. 

The receiver on the AlphaBot2 (connected to digital pin **D4**) is an integrated sensor that demodulates the 38 kHz signal, outputting a clean logic level to the microcontroller:
* **Idle (No Signal)**: Outputs a constant **HIGH (5V)**.
* **Active (IR Signal Detected)**: Outputs a **LOW (0V)**.

### 2. The NEC Transmission Protocol
The AlphaBot2 remote control uses the popular **NEC Protocol** for encoding button presses. An NEC message frame consists of:
1. **Leader Code (Preamble)**: A 9 ms LOW pulse followed by a 4.5 ms HIGH pulse, signaling the start of a transmission.
2. **Address & Inverse Address**: Two 8-bit bytes representing the manufacturer code. The inverse address is used as a checksum to verify transmission integrity.
3. **Command & Inverse Command**: Two 8-bit bytes representing the specific key pressed.
4. **End/Stop Bit**: A final 562.5 microsecond pulse to close the frame.

### 3. Bit Encoding (Pulse Distance Modulation)
Unlike serial UART which uses high/low levels for bit durations, the NEC protocol uses **Pulse Distance Modulation**. Every bit starts with a fixed-duration LOW pulse (562.5 microseconds), but the duration of the subsequent HIGH space determines if the bit is a 0 or a 1:
* **Logical 0**: 562.5 microseconds space (Total time = 1.125 ms).
* **Logical 1**: 1.69 ms space (Total time = 2.25 ms).

---

## 🧭 Remote Control Key Mapping (Hex Codes)

The 8-bit command byte received is mapped to the remote buttons:

| Key Label | Command Hex Code | Assigned Action |
| :--- | :---: | :--- |
| **Key 2** | `0x18` | Drive Forward |
| **Key 4** | `0x08` | Turn Left |
| **Key 5** | `0x1C` | Stop Motors |
| **Key 6** | `0x5A` | Turn Right |
| **Key 8** | `0x52` | Drive Backward |
| **VOL-** | `0x07` | Decrease Speed |
| **VOL+** | `0x15` | Increase Speed |
| **EQ** | `0x09` | Reset Speed to Default (150 PWM) |
| **Repeat Code** | `0xFF` | Handled when key is held down |

---

## 📂 Experiment Files

* **[05-IR-Remote.ino](file:///f:/AlphaBot2/R4Experiments/05-IR-Remote/05-IR-Remote.ino)**: Bit-bang logic reading the IR pin, decoding the NEC frame, verifying integrity, and mapping key presses to motor commands.

---

## 🧠 Software Protocol Validation (Checksums)

To guarantee the remote didn't miss a bit, the NEC protocol sends the command byte and its bitwise inverse byte. The program verifies this data using a checksum:
* `data[2] + data[3] == 0xFF` (Command + Inverse Command = 255)

If this checksum matches, the code executes the button action. If it fails, the packet is rejected as noise.

---

## 📝 Lab Procedure & Student Tasks

### Task 1: Calibrate and Verify IR Remote Decodes
1. Open [05-IR-Remote.ino](file:///f:/AlphaBot2/R4Experiments/05-IR-Remote/05-IR-Remote.ino) and upload it wirelessly over OTA.
2. Open the **Serial Monitor** at **115200 baud**.
3. Point your remote at the receiver sensor on the front of the AlphaBot2.
4. Press the navigation keys (2, 4, 5, 6, 8) and observe the corresponding hexadecimal command codes printed on the terminal.
5. Verify that the robot executes the movement matching the button pressed and stops when you release the key.

### Task 2: Analyze Pulse Timing
Look closely at the `IR_decode()` function in the sketch. Note how the code uses `delayMicroseconds(60)` loops to measure the duration of pulses.

---

## ❓ Post-Lab Questions for Students
1. Why does the NEC protocol send the inverse of the address and command bytes along with the data? 
2. Explain the difference between **Pulse Width Modulation (PWM)** and **Pulse Distance Modulation (PDM)**.
3. Why does sunlight sometimes cause infrared remote control systems to fail, and how does modulating the transmitter at 38 kHz help prevent this?
