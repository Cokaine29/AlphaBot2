# 🎓 AlphaBot2 Lab Experiments for B.Tech Students

Welcome to the **Experiments** directory! This space is designed for educational laboratory experiments tailored for Bachelor of Technology (B.Tech) students in fields such as Electronics, Electrical, Computer Science, and Robotics Engineering. 

These experiments use the Waveshare AlphaBot2 platform to bridge the gap between theory and practical engineering, covering key concepts in Embedded Systems, Control Systems, Real-Time Software, and Wireless IoT.

---

## 📚 Core Learning Objectives

1. **Control Theory (PID)**:
   * Understanding Proportional (P), Integral (I), and Derivative (D) control.
   * Real-time tuning, error computation, and PWM duty cycle modulation.
2. **Embedded Hardware Interfaces**:
   * GPIO control, analog-to-digital converters (ADC) for sensors, and motor drivers (H-Bridge).
   * I2C Communication (using the PCF8574 I/O expander).
   * Actuators: DC Motors, NeoPixel addressable LEDs, and buzzers.
3. **Sensor Integration & Processing**:
   * Calibration of infrared (IR) reflection sensors.
   * Eliminating noise, threshold detection, and sensor fusion.
4. **Wireless IoT & Telemetry**:
   * BLE (Bluetooth Low Energy) communication protocol design.
   * Parsing JSON commands on microcontrollers.
   * Designing client-side web interfaces (Web Bluetooth API) for remote control.

---

## 🛠️ Experiment Directory Structure

Each experiment will be housed in its own subdirectory with:
- **`README.md`**: Student laboratory sheet outlining objectives, theoretical background, circuit connection, tasks, and lab report questions.
- **`SketchName.ino`**: Fully commented starter or reference code for the Arduino.
- **`WebController.html`**: (Optional) Interactive HTML/JS dashboard for wireless control and live graphing.

## 🧪 Planned Experiments

| # | Experiment Title | Key Concepts | Files / Path |
|---|---|---|---|
| **01** | Open-Loop Control & Motor Calibration | H-Bridge logic, PWM speeds, calibration offsets, open-loop drift | [01-Move-Straight](file:///f:/AlphaBot2/Experiments/01-Move-Straight/README.md) |
| **02** | Differential-Drive Turning Concepts | Pivot turn vs. Swing turn, turning physics, timing-based calibration | [02-Turning-Concepts](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/README.md) |
| **03** | Speed, Distance, and RPM Calibration | Wheel dimensions, speed modeling, time-distance kinematics, actuator characterization | [03-Speed-Kinematics](file:///f:/AlphaBot2/Experiments/03-Speed-Kinematics/README.md) |



