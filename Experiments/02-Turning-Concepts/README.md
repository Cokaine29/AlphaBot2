# 🧪 Experiment 02: Differential-Drive Turning Concepts

## 🎯 Objectives
1. Understand the difference between a **Pivot Turn (Spin on Spot)** and a **Swing Turn (Radius Turn)** on a differential-drive robot.
2. Implement Left, Right, and 360-degree turns using discrete Arduino sketches.
3. Learn how to calibrate turn angles in an open-loop system using motor speed and **timing delays**.

---

## 🛠️ Theoretical Background

On a differential-drive robot like the AlphaBot2, there is no steering wheel. Steering is achieved entirely by driving the left and right wheels at different speeds or in different directions.

There are two primary ways to turn:

### 1. Pivot Turn (Spin on the Spot)
* **How it works**: The left and right wheels rotate at the same speed but in **opposite directions**.
* **Result**: The robot rotates around its geometric center. It has a turning radius of zero, making it highly maneuverable in tight spaces (like mazes).
* **Code Logic**:
  * **Left Pivot**: Left Motor $\rightarrow$ Backward, Right Motor $\rightarrow$ Forward
  * **Right Pivot**: Left Motor $\rightarrow$ Forward, Right Motor $\rightarrow$ Backward

### 2. Swing Turn (Single-Wheel Pivot)
* **How it works**: One wheel is stopped (speed = 0) while the opposite wheel rotates forward.
* **Result**: The robot pivots around the stationary wheel, tracing an arc.
* **Code Logic**:
  * **Left Swing**: Left Motor $\rightarrow$ Stopped, Right Motor $\rightarrow$ Forward
  * **Right Swing**: Left Motor $\rightarrow$ Forward, Right Motor $\rightarrow$ Stopped

---

## 🧭 Angle Calibration in Open-Loop Systems

Without encoders (which count wheel rotations) or a gyroscope (which measures angles), a robot cannot directly measure how many degrees it has turned.

Instead, we calibrate turns using **time** and **speed**:
$$\text{Turning Angle} \propto \text{Motor Speed} \times \text{Duration (ms)}$$

To turn exactly $90^\circ$ or $360^\circ$:
1. We choose a fixed turning speed (e.g., `100` PWM).
2. We experimentally find the exact time duration (in milliseconds) required for the robot to complete the target angle.

---

## 📂 Experiment Subdirectories

This experiment consists of three separate Arduino sketches:

1. **[Left-Turn](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/Left-Turn/Left-Turn.ino)**: Calibrates a precise $90^\circ$ left pivot turn.
2. **[Right-Turn](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/Right-Turn/Right-Turn.ino)**: Calibrates a precise $90^\circ$ right pivot turn.
3. **[Spin-360](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/Spin-360/Spin-360.ino)**: Calibrates a complete $360^\circ$ spin on the spot.

---

## 🧠 Turning Truth Table (Left vs. Right Motors)

| Target Turn | Left Motor Dir | Right Motor Dir | Left Speed | Right Speed | Action |
|:---|:---:|:---:|:---:|:---:|:---|
| **Left Pivot** | `Backward` | `Forward` | `Speed` | `Speed` | Spins counter-clockwise on spot |
| **Right Pivot** | `Forward` | `Backward` | `Speed` | `Speed` | Spins clockwise on spot |
| **Left Swing** | `Stopped` | `Forward` | `0` | `Speed` | Swings CCW around stationary left wheel |
| **Right Swing** | `Forward` | `Stopped` | `Speed` | `0` | Swings CW around stationary right wheel |

---

## 📝 Lab Procedure & Student Tasks

### Task 1: Calibrating the $90^\circ$ Left Pivot
1. Open the [Left-Turn](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/Left-Turn/Left-Turn.ino) sketch.
2. Place the robot on a flat surface. Mark its starting alignment.
3. Upload the sketch and run it. The robot will attempt a $90^\circ$ left turn on the spot.
4. If it turns **less than $90^\circ$**, increase `TURN_DURATION_MS` in the code.
5. If it over-rotates, decrease `TURN_DURATION_MS`.
6. Iterate until the turn is exactly $90^\circ$.

### Task 2: Calibrating the $90^\circ$ Right Pivot
1. Open the [Right-Turn](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/Right-Turn/Right-Turn.ino) sketch.
2. Repeat the calibration process to find the exact `TURN_DURATION_MS` for a $90^\circ$ right pivot.
3. *Note*: Even at the same speed, left and right motors might require slightly different durations due to friction!

### Task 3: The $360^\circ$ Spot Spin Challenge
1. Open the [Spin-360](file:///f:/AlphaBot2/Experiments/02-Turning-Concepts/Spin-360/Spin-360.ino) sketch.
2. Calibrate the delay until the robot does exactly one full rotation and points back to its exact starting line.

---

## ❓ Post-Lab Questions for Students
1. Did your robot require the exact same duration for a $90^\circ$ Left turn versus a $90^\circ$ Right turn? Explain why they might differ.
2. What are the advantages of a **Pivot Turn** over a **Swing Turn** in a narrow maze track?
3. If battery voltage drops from 7.4V to 6.2V, what will happen to your calibrated $360^\circ$ turn? How can a closed-loop system using a gyroscope or wheel encoders resolve this?
