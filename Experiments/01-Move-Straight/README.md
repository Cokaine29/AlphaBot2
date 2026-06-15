# 🧪 Experiment 01: Open-Loop Control & H-Bridge Motor Calibration

## 🎯 Objectives
1. Understand the operation of an **H-Bridge motor driver** using digital (direction) and PWM (speed) pins.
2. Observe why identical motors spin at different speeds due to physical hardware variances (**Open-Loop drift**).
3. Calibrate motor speeds using software **offsets** to make the robot move in a perfectly straight line.

---

## 🛠️ Theoretical Background

### 1. How DC Motors are Controlled (H-Bridge)
To move a robot, we need to control both the **direction** and **speed** of its DC motors. The AlphaBot2 uses an H-Bridge motor driver circuit. For each motor, it requires 3 microcontroller pins:
* **2 Direction Pins (Digital output)**: Set to `HIGH` / `LOW` to dictate clockwise (forward) or counter-clockwise (backward) rotation.
* **1 Speed Pin (PWM/Analog output)**: Receives a value from `0` (stopped) to `255` (full speed) using `analogWrite()` to control motor speed.

### 2. The "Curving Bot" Problem (Why physical systems differ)
If you program both motors to run at a PWM speed of `150`, you might expect the robot to move in a straight line. However, in the real world:
* Motors have slight variations in internal resistance, friction, and winding quality.
* The wheels might have different grip or alignment.
* The battery voltage drops, affecting each motor differently.

As a result, one wheel will spin faster than the other, causing the robot to curve or drift. To solve this in software without active sensors (closed-loop), we introduce **Calibration Offsets**.

---

## 🔌 Hardware Connections & Microcontroller Pins

The AlphaBot2 on-board H-Bridge is connected to the Arduino Uno pins as follows:

| Motor | Control Pin | Arduino Pin | Function | Direction Code |
| :--- | :--- | :--- | :--- | :--- |
| **Left** | **`PWMA`** | **`6`** | Left Motor Speed (`0` - `255`) | Speed Control |
| **Left** | **`AIN1`** | **`A1`** | Left Motor Direction 1 | `AIN1=LOW`, `AIN2=HIGH` $\rightarrow$ Forward <br> `AIN1=HIGH`, `AIN2=LOW` $\rightarrow$ Backward |
| **Left** | **`AIN2`** | **`A0`** | Left Motor Direction 2 | |
| **Right**| **`PWMB`** | **`5`** | Right Motor Speed (`0` - `255`) | Speed Control |
| **Right**| **`BIN1`** | **`A2`** | Right Motor Direction 1 | `BIN1=LOW`, `BIN2=HIGH` $\rightarrow$ Forward <br> `BIN1=HIGH`, `BIN2=LOW` $\rightarrow$ Backward |
| **Right**| **`BIN2`** | **`A3`** | Right Motor Direction 2 | |

---

## 🧠 Logical Flow & Pseudocode

### Dynamic Speed Offsets
To balance the wheels, we define offset values at the top of the program:
$$\text{Applied Left Speed} = \text{Target Speed} + \text{LEFT\_SPEED\_OFFSET}$$
$$\text{Applied Right Speed} = \text{Target Speed} + \text{RIGHT\_SPEED\_OFFSET}$$

If the robot drifts to the **right**, it means the right motor is too slow (or left is too fast). We can compensate by:
* Adding a positive offset to the right motor.
* Or subtracting an offset from the left motor.

### Pseudocode

```text
// Initialization
Set PWMA, AIN1, AIN2, PWMB, BIN1, BIN2 as OUTPUT pins

// Offset configuration (Adjust during calibration)
Define LEFT_SPEED_OFFSET = 0
Define RIGHT_SPEED_OFFSET = 0

Function SetSpeeds(leftSpeed, rightSpeed):
    // Calculate speed after offsets
    finalLeft = leftSpeed + LEFT_SPEED_OFFSET
    finalRight = rightSpeed + RIGHT_SPEED_OFFSET

    // Constrain speeds to safe ranges (0 to 255)
    finalLeft = Constrain(finalLeft, 0, 255)
    finalRight = Constrain(finalRight, 0, 255)

    // Send speed to H-Bridge PWM pins
    analogWrite(PWMA, finalLeft)
    analogWrite(PWMB, finalRight)

    // Set directions to Forward
    digitalWrite(AIN1, LOW)
    digitalWrite(AIN2, HIGH)
    digitalWrite(BIN1, LOW)
    digitalWrite(BIN2, HIGH)

Function Loop:
    // Move forward at target speed of 120 for 3 seconds, then stop
    Call SetSpeeds(120, 120)
    Delay 3000 milliseconds
    
    // Stop motors
    analogWrite(PWMA, 0)
    analogWrite(PWMB, 0)
    
    // End execution loop
    infinite_loop:
        Do nothing
```

---

## 📝 Lab Procedure & Student Tasks

### Task 1: Flashing the Baseline Code (Observe Drift)
1. Write the starter Arduino sketch using the H-Bridge pinout defined above. Set both speed offsets to `0`.
2. Place the robot on a flat surface along a straight grid line.
3. Power on the robot and upload the sketch. 
4. **Observe & Measure**: Measure the deviation (drift) to the left or right from the center line after the robot travels for 3 seconds.

### Task 2: Software Speed Calibration
1. Based on your observation in Task 1, adjust `LEFT_SPEED_OFFSET` or `RIGHT_SPEED_OFFSET` in your code.
   * *Tip*: If the robot veers right, increase `RIGHT_SPEED_OFFSET` or decrease `LEFT_SPEED_OFFSET`.
2. Upload the code and test again.
3. Iterate until the robot drives in a perfectly straight line over a distance of at least 2 meters.
4. Record your final offset values in the lab report.

---

## ❓ Post-Lab Questions for Students
1. Why does an H-Bridge require two direction pins per motor instead of just one? What happens if you set both direction pins of a motor to `HIGH`?
2. Why is this experiment called an **Open-Loop** system? What sensor could we add to the wheels to make it a **Closed-Loop** self-correcting system?
3. How do speed offsets affect the maximum speed of the robot? (Hint: What happens to a speed request of `250` if a motor has a `+15` offset?)
