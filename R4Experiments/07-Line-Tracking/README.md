# 🧪 Experiment 07: Infrared Line Tracking & PD Control

## 🎯 Objectives
1. Interface the **5-channel TRSensors Infrared Reflectance Array** with the Arduino UNO R4 WiFi.
2. Implement an automated calibration routine to map white floor vs. black tape reflectivity thresholds.
3. Understand the math behind weighted-average sensor array positioning.
4. Implement a closed-loop **Proportional-Derivative (PD) control loop** to steering-correct and follow a black line.

---

## 🛠️ Theoretical Background

### 1. The 5-Channel Reflection Sensor Array
The bottom of the AlphaBot2 chassis hosts 5 infrared reflection sensors (numbered 0 to 4 from left to right). Each sensor emits infrared light and reads the intensity of the reflection:
* **White Surface**: Highly reflective. High amounts of IR bounce back, resulting in a low reading (after calibration, close to `0`).
* **Black Surface**: Highly absorbent. Very little IR bounces back, resulting in a high reading (after calibration, close to `1000`).

### 2. Weighted-Average Position Calculation
Instead of simply checking which sensor is over the line, the `TRSensors` library calculates the line's position using a weighted average. The returned position is a value between **`0` and `4000`**:
* **`0`**: Line is directly under Sensor 0 (far left).
* **`1000`**: Line is directly under Sensor 1.
* **`2000`**: Line is directly under Sensor 2 (center).
* **`3000`**: Line is directly under Sensor 3.
* **`4000`**: Line is directly under Sensor 4 (far right).

---

### 3. Closed-Loop Control: The PD Loop
To keep the robot centered on the line (target position = `2000`), we calculate the steering correction using a **Proportional-Derivative (PD)** control loop:

#### A. Proportional Term ($P$)
The Proportional term represents the current error (how far the robot is from the center of the line):
$$\text{Error } (e) = \text{position} - 2000$$
If $e > 0$, the line is to the right; if $e < 0$, the line is to the left. The magnitude represents how far off-center the robot is.

#### B. Derivative Term ($D$)
The Derivative term represents the speed of change in error, helping to predict and dampen overshoot:
$$\text{Derivative } (d) = e(t) - e(t-1)$$
This dampens the steering when the robot is quickly returning to the center line, preventing wild oscillations.

#### C. Control Equation
The steering speed adjustment ($\text{Power Difference}$) is calculated as:
$$\text{Power Difference} = (K_p \times e) + (K_d \times d)$$
Based on tuned parameters in the reference firmware:
* $K_p = \frac{1}{20}$ (converts position error to motor speed adjustment).
* $K_d = 10$ (dampening gain).

We apply this difference by adjusting the speed of the left and right motors:
* **Turning Left**: Slow down Left Motor, keep Right Motor at base speed.
* **Turning Right**: Keep Left Motor at base speed, slow down Right Motor.

---

## 📂 Experiment Files

* **[07-Line-Tracking.ino](file:///f:/AlphaBot2/R4Experiments/07-Line-Tracking/07-Line-Tracking.ino)**: Automatic calibration routine, PD control loop calculation, lost-line protection, and differential motor speed steering.

---

## 📝 Lab Procedure & Student Tasks

> [!IMPORTANT]
> To ensure the robot runs immediately on boot without networking delay or connection timeouts, Wi-Fi, OTA, and Telnet Stream have been disabled in this sketch. The robot will read and react to joystick inputs immediately upon boot/reset. Status monitoring is done entirely over the **USB Serial Monitor** (115200 baud).

### Task 1: Upload and Calibrate
1. Open [07-Line-Tracking.ino](file:///f:/AlphaBot2/R4Experiments/07-Line-Tracking/07-Line-Tracking.ino).
2. Upload the code using PowerShell:
   ```powershell
   .\manage.ps1 upload R4Experiments\07-Line-Tracking
   ```
3. Open the **Serial Monitor** at **115200 baud**.
4. Place the robot's middle sensor over the black tape line on a white floor.
5. Press the joystick **CENTER** key to start calibration.
6. The robot will automatically pivot back and forth for about 1 second (100 sweeps) to map the black vs. white surface.
7. Once the double-beep sounds, place the robot at the start of your line tracking course.
8. Press the joystick **CENTER** key again to start tracking. Observe its movement.

---

### Task 2: Tune the PD Loop Coefficients
Students will observe how different $K_p$ and $K_d$ values impact tracking stability.
1. Modify the constants in the control calculation:
   ```cpp
   int powerDifference = proportional / Kp_divisor + derivative * Kd_multiplier;
   ```
2. Test the following configurations and describe the physical behavior (e.g., smooth tracking, wide oscillations, losing the track at curves):

| Test | Proportional Gain ($Kp$) | Derivative Gain ($Kd$) | Observations / Behavior |
| :---: | :---: | :---: | :---: |
| **A** | `proportional / 5` (High) | `derivative * 0` (No D) | |
| **B** | `proportional / 50` (Low) | `derivative * 0` (No D) | |
| **C** | `proportional / 20` (Good) | `derivative * 0` (No D) | |
| **D** | `proportional / 20` (Good) | `derivative * 30` (High) | |
| **E** | `proportional / 20` (Good) | `derivative * 10` (Tuned)| |

---

## ❓ Post-Lab Questions for Students
1. Why does setting the Derivative Gain ($K_d$) to zero make the robot oscillate back and forth (s-curve wobble) on straight lines?
2. What happens to the sensor outputs if the robot drives off the track entirely? How does the code handle this scenario to prevent runaway?
3. How does ambient sunlight shining onto the track affect the infrared transceiver sensors? How does calibration help mitigate this?
4. If you wanted to increase the base tracking speed (`MAX_SPEED`) to `150`, how would you need to adjust $K_p$ and $K_d$ to maintain stability?
