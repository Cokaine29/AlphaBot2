# 🧪 Experiment 03: Speed, Distance, and RPM Calibration

## 🎯 Objectives
1. Understand the kinematics of a differential-drive robot (relationship between time, speed, and distance).
2. Mathematically link wheel angular speed (RPM) to linear robot speed (cm/s).
3. Create an empirical calibration curve (Speed vs. PWM) to predict and control distance open-loop.

---

## 🛠️ Theoretical Background

In this experiment, we will build a mathematical model of our robot to control how far it drives. Because our standard AlphaBot2 base kit does not have built-in wheel encoders, we cannot count wheel turns in real-time. Instead, we use **Empirical Calibration**—measuring the physical performance of the robot at different power levels and using those values to calculate travel times.

### 1. Connecting Wheel Spin (RPM) to Robot Travel (cm/s)
When a wheel makes one complete rotation, the distance the robot travels is equal to the wheel's circumference:
* **Wheel Diameter (D)** = 4.2 cm
* **Wheel Circumference (C)** = pi * D = 3.14159 * 4.2 cm = 13.19 cm

If a wheel rotates at **RPM** (Revolutions Per Minute), it turns RPM / 60 times per second.
The linear speed of the robot **v** (in cm/s) is:
* v = (RPM / 60) * Circumference
* v = (RPM / 60) * 13.19 cm

Conversely, if we know the linear speed **v** (in cm/s), we can calculate the wheel's RPM:
* RPM = (v * 60) / 13.19
* RPM = v * 4.55

### 2. Controlling Distance via Timing
To drive the robot a target distance **d** (in cm) at a calibrated speed **v** (in cm/s), the time **t** (in seconds) the motors must run is:
* t = d / v

In our Arduino code, we specify delays in milliseconds, so:
* Run Duration (ms) = (d / v) * 1000

---

## 📂 Experiment Subdirectories

This experiment consists of two Arduino sketches:

1. **[03-Speed-Characterization](file:///f:/AlphaBot2/R4Experiments/03-Speed-Kinematics/03-Speed-Characterization/03-Speed-Characterization.ino)**: Drives the robot forward at a selectable PWM speed for exactly 3.0 seconds. Used to collect calibration data.
2. **[03-Distance-Target](file:///f:/AlphaBot2/R4Experiments/03-Speed-Kinematics/03-Distance-Target/03-Distance-Target.ino)**: Takes a target distance and PWM speed, computes the required run duration using the student's calibrated speed, drives the robot, and stops.

---

## 📝 Lab Procedure & Student Tasks

### Task 1: Calibrate Speed vs. PWM (Characterization)
1. Mark a clear starting line on a flat floor. Lay out a tape measure next to it.
2. Open the [03-Speed-Characterization](file:///f:/AlphaBot2/R4Experiments/03-Speed-Kinematics/03-Speed-Characterization/03-Speed-Characterization.ino) sketch.
3. In the sketch, set `TEST_PWM = 80`. Upload the code to the robot wirelessly via OTA.
4. Place the wheels exactly on the start line, turn the robot on, and let it run for 3 seconds.
5. Measure the exact distance traveled in centimeters and record it in your table.
6. Repeat steps 3–5 for PWM values of `120`, `160`, and `200`.
7. Calculate the Speed (v = Distance / 3.0 seconds) and RPM (RPM = v * 4.55) for each test.

| PWM | Test Time (t) | Distance (d) | Speed (v = d/t) | Wheel RPM |
| :--- | :---: | :---: | :---: | :---: |
| **80** | 3.0 s | *Measure* cm | *Calc* cm/s | *Calc* |
| **120** | 3.0 s | *Measure* cm | *Calc* cm/s | *Calc* |
| **160** | 3.0 s | *Measure* cm | *Calc* cm/s | *Calc* |
| **200** | 3.0 s | *Measure* cm | *Calc* cm/s | *Calc* |

### Task 2: Plot the Actuator Curve
* Plot **PWM** on the X-axis and **Speed (cm/s)** on the Y-axis.
* *Observation*: Does the speed increase linearly? At what PWM value does the robot fail to move at all (dead band)?

### Task 3: Driving a Target Distance
1. Open the [03-Distance-Target](file:///f:/AlphaBot2/R4Experiments/03-Speed-Kinematics/03-Distance-Target/03-Distance-Target.ino) sketch.
2. In the code, input your calibrated speed value for PWM 120 (e.g., if you measured 32 cm/s, set `CALIBRATED_SPEED = 32.0;`).
3. Set the target distance: `TARGET_DISTANCE_CM = 150.0;`
4. Upload the code, run it, and measure the actual distance traveled.
5. Calculate the percentage error:
   Error % = (|Actual Distance - Target Distance| / Target Distance) * 100

---

## ❓ Post-Lab Questions for Students
1. Why is there a minimum PWM value below which the robot does not move? What is this region called?
2. If the battery level drops, how does it affect your calibrated speed value?
3. How could you implement closed-loop control to make the robot drive exactly 1.5 meters regardless of battery level or floor friction?
