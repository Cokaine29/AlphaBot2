# 🧪 Experiment 01: Open-Loop Control & H-Bridge Motor Calibration (R4 WiFi Edition)

## 🎯 Objectives
1. Understand the operation of a dual H-Bridge motor driver on the AlphaBot2 chassis using an **Arduino UNO R4 WiFi**.
2. Observe open-loop speed drift caused by physical hardware variances in DC motors.
3. Configure speed calibration **offsets** in software to ensure the robot travels in a straight line.

---

## 🛠️ Theoretical Background

### 1. Motor Speed & H-Bridge Control
To move the AlphaBot2, the Arduino must communicate with the TB6612FNG H-bridge chip on the bottom chassis. For each motor:
* **2 Digital Pins** control direction (forward, backward, coast, or brake).
* **1 Analog (PWM) Pin** controls speed by switching the power rapidly on and off. The average voltage is determined by the **Duty Cycle** (a PWM value from 0 to 255).

The UNO R4 WiFi's Renesas RA4M1 microcontroller generates these PWM signals on pins D5 and D6.

### 2. Motor Variances & Calibration Offsets
Although both motors are identical models (N20 gear motors), physical differences in gear friction, coil resistance, and manufacturing tolerances cause them to spin at slightly different rates when given the same PWM power.

To make the robot drive in a perfectly straight line without active sensors, we apply a static offset in software:
* Applied Left Speed = Base Speed + LEFT_SPEED_OFFSET
* Applied Right Speed = Base Speed + RIGHT_SPEED_OFFSET

---

## 🔌 Hardware Connections & Microcontroller Pins

The H-bridge pins map to the Arduino UNO R4 WiFi pins identically to the R3:

| Motor | Control Line | Arduino Pin | Pin Function |
| :--- | :--- | :--- | :--- |
| **Left** | **`PWMA`** | **`D6`** | Speed Control (PWM) |
| **Left** | **`AIN1`** | **`A1`** | Direction Line 1 |
| **Left** | **`AIN2`** | **`A0`** | Direction Line 2 |
| **Right** | **`PWMB`** | **`D5`** | Speed Control (PWM) |
| **Right** | **`BIN1`** | **`A2`** | Direction Line 1 |
| **Right** | **`BIN2`** | **`A3`** | Direction Line 2 |

* **Direction Code (Forward)**:
  * Left Motor: `AIN1 = LOW`, `AIN2 = HIGH`
  * Right Motor: `BIN1 = LOW`, `BIN2 = HIGH`

---

## 🧠 Logical Flow & Pseudocode

```text
Initialize digital output pins: PWMA, AIN1, AIN2, PWMB, BIN1, BIN2

Define LEFT_SPEED_OFFSET = 0
Define RIGHT_SPEED_OFFSET = 0

Function moveForward(baseSpeed):
    finalLeft = baseSpeed + LEFT_SPEED_OFFSET
    finalRight = baseSpeed + RIGHT_SPEED_OFFSET

    // Clamp speed limits to safe ranges (0 to 255)
    finalLeft = Constrain(finalLeft, 0, 255)
    finalRight = Constrain(finalRight, 0, 255)

    // Set Forward directions
    digitalWrite(AIN1, LOW)
    digitalWrite(AIN2, HIGH)
    digitalWrite(BIN1, LOW)
    digitalWrite(BIN2, HIGH)

    // Output speed values
    analogWrite(PWMA, finalLeft)
    analogWrite(PWMB, finalRight)

Function setup():
    Set output modes for motor pins
    Stop all motors
    Delay 1 second

Function loop():
    moveForward(60)
    Delay 5 seconds
    Stop all motors
    
    // Halt execution
    Loop forever doing nothing
```

---

## 📝 Lab Procedure & Student Tasks

### Task 1: Flashing & Observing Drift
1. Open the [01-Move-Straight.ino](file:///f:/AlphaBot2/R4Experiments/01-Move-Straight/01-Move-Straight.ino) sketch in the editor.
2. Connect your Arduino UNO R4 WiFi to your computer.
3. Make sure the board in your configuration is set to **`arduino:renesas_uno:uno_r4_wifi`**.
4. Upload the code. **Note**: Unlike the UNO R3, you do NOT need to unplug the Bluetooth module when flashing! The upload goes through a separate USB virtual port, leaving the Bluetooth UART free.
5. Place the robot on a flat, smooth surface next to a line template.
6. Turn the power on. Observe the path the robot takes during its 5-second drive.
7. Measure and record how far the robot drifted to the left or right from a straight line.

### Task 2: Offsets Tuning
1. Adjust the constants `LEFT_SPEED_OFFSET` and `RIGHT_SPEED_OFFSET` in the code.
   * *Tip*: If the robot drifts to the left, increase the left motor's offset or decrease the right motor's offset.
2. Upload the updated code and run the test.
3. Repeat the adjustments until the robot travels perfectly straight for the entire 5 seconds.

---

## ❓ Post-Lab Questions for Students
1. Why does the Arduino UNO R4 WiFi allow you to upload code without unplugging the Bluetooth module, unlike the older UNO R3? (Hint: Think about hardware serial ports).
2. What happens to the motors if you output a PWM value of `255` but have a positive offset configured in the code? How does the `constrain()` function protect the microcontroller from errors?
3. Why does the robot drift more on rough carpets than on smooth tiled floors? Can a static calibration offset fully correct this?
