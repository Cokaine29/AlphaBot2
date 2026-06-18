# 🎓 AlphaBot2 Lab Experiments for Arduino UNO R4 WiFi

Welcome to the **R4Experiments** directory! This space contains laboratory experiments specifically updated and optimized for the **Arduino UNO R4 WiFi** board (Renesas RA4M1 32-bit ARM Cortex-M4 microcontroller) mounted on the Waveshare AlphaBot2.

These exercises bridge the gap between 8-bit AVR microcontrollers (like the R3) and modern 32-bit ARM-based embedded controllers, utilizing the R4's advanced capabilities (like separate hardware serial UARTs and onboard WiFi/BLE).

---

## 🚀 Key Advantages of Arduino UNO R4 WiFi in Labs

1. **Dual Hardware Serial Ports**:
   * **UNO R3**: Shared USB and Bluetooth on `Serial` (required unplugging the BLE module to upload code).
   * **UNO R4 WiFi**: USB runs on a virtual COM port (`Serial`), while the physical D0/D1 pins run on a separate hardware port (`Serial1`). **Students can flash code wirelessly or via USB without unplugging the Bluetooth module!**
2. **32-Bit ARM Processing Power**:
   * Runs at **48 MHz** (3x faster than the R3's 16 MHz).
   * 32-bit wide data register bus allowing faster logic computation and smoother control algorithms.
3. **Expanded Memory**:
   * **SRAM**: 32 KB (16x more than R3's 2 KB), eliminating stability issues and heap crashes during telemetry logging.
   * **Flash**: 256 KB (8x more than R3's 32 KB), allowing students to integrate complex communication stacks.

---

## 🛠️ Experiment Directory Structure

Each experiment in this directory contains:
- **`README.md`**: Laboratory manual detailing instructions, wiring pinouts, logic, and questions.
- **`SketchName.ino`**: Fully documented R4-compatible Arduino starter code using the `Serial1` hardware port.

---

## 🧪 Planned R4 Experiments

| # | Experiment Title | Key Concepts | Files / Path |
|---|---|---|---|
| **00** | Wireless (OTA) Bootloader Setup | ESP32-S3 co-processor, local Wi-Fi connection, over-the-air firmware uploads, network ports | [00-WiFi-OTA](file:///f:/AlphaBot2/R4Experiments/00-WiFi-OTA/README.md) |
| **01** | Open-Loop Control & Motor Calibration | H-Bridge pin mapping, PWM motor speed control, hardware calibration offsets, open-loop drift | [01-Move-Straight](file:///f:/AlphaBot2/R4Experiments/01-Move-Straight/README.md) |
| **02** | Differential-Drive Turning Concepts | Pivot Turn vs Swing Turn, calibration of Left, Right, and 360-degree turns | [02-Turning-Concepts](file:///f:/AlphaBot2/R4Experiments/02-Turning-Concepts/README.md) |
| **03** | Speed, Distance, and RPM Calibration | Kinematics of differential drive, empirical speed-to-PWM calibration, open-loop distance control | [03-Speed-Kinematics](file:///f:/AlphaBot2/R4Experiments/03-Speed-Kinematics/README.md) |
| **04** | I2C I/O Expansion & Joystick Control | PCF8574 expander address, quasi-bidirectional I/O, joystick mapping and masking, buzzer control | [04-Joystick-I2C](file:///f:/AlphaBot2/R4Experiments/04-Joystick-I2C/README.md) |
| **05** | Infrared (IR) Communication & NEC Decodes | Modulation at 38 kHz, pulse distance modulation (PDM), bit-bang NEC decoder, remote navigation | [05-IR-Remote](file:///f:/AlphaBot2/R4Experiments/05-IR-Remote/README.md) |
| **06** | Ultrasonic Ranging & Collision Avoidance | Time-of-Flight sound measurements, trigger/echo pulse timing, speed of sound physical modeling, closed-loop braking | [06-Ultrasonic-Ranging](file:///f:/AlphaBot2/R4Experiments/06-Ultrasonic-Ranging/README.md) |


