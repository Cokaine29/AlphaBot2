# 🧪 Experiment 06: Ultrasonic Ranging & Collision Avoidance

## 🎯 Objectives
1. Understand the physics of ultrasonic waves and time-of-flight (ToF) distance measurement.
2. Interface the standard **HC-SR04 Ultrasonic Sensor** with the Arduino UNO R4 WiFi.
3. Apply physical equations to convert pulse duration (microseconds) into physical distance (centimeters).
4. Implement active closed-loop motor control to brake the robot when an obstacle is detected.

---

## 🛠️ Theoretical Background

### 1. How Ultrasonic Ranging Works
The ultrasonic sensor operates like a miniature sonar system (similar to bats or dolphins). It has two main cylinders on the front:
* **Transmitter (TRIG pin)**: Emits a high-frequency sound wave burst at **40 kHz** (which is above human hearing).
* **Receiver (ECHO pin)**: Listens for the returning echo reflecting off an obstacle.

```
       AlphaBot2                            Obstacle
      +----------+      Ultrasonic Wave     +--------+
      |   TRIG   | )))))))))))))))))))))))> |        |
      |          |                          |        |
      |   ECHO   | <((((((((((((((((((((((( |        |
      +----------+        Reflected Echo    +--------+
```

### 2. The Trigger and Echo Pulse Sequence
To initiate a distance measurement:
1. The Arduino sends a short **10 microsecond HIGH pulse** to the **TRIG** pin.
2. The sensor automatically emits **8 cycles of 40 kHz** ultrasonic sound pulses.
3. Immediately after sending, the sensor pulls the **ECHO** pin **HIGH**.
4. The **ECHO** pin remains **HIGH** until the sound wave returns to the sensor. The duration that the ECHO pin stays HIGH is the exact travel time (round-trip) of the sound wave.

```text
TRIG  ___| 10us |______________________________________
         
ECHO  ___________|======== Travel Time (us) ========|___
```

### 3. Converting Time to Distance (The Physics Equation)
The speed of sound in air at room temperature ($20^\circ\text{C}$) is approximately **343 meters per second** ($0.0343\text{ cm/}\mu\text{s}$).

Since the sound wave travels from the sensor to the obstacle and then back to the sensor, the physical distance to the object is half of the total distance traveled:
$$\text{Distance} = \frac{\text{Travel Time} \times \text{Speed of Sound}}{2}$$

Substituting the speed of sound:
$$\text{Distance (cm)} = \frac{\text{Travel Time }(\mu\text{s}) \times 0.0343\text{ cm/}\mu\text{s}}{2}$$
$$\text{Distance (cm)} = \frac{\text{Travel Time }(\mu\text{s})}{\frac{2}{0.0343}} \approx \frac{\text{Travel Time }(\mu\text{s})}{58.3}$$

In Arduino programming, we simplify this to:
$$\text{Distance (cm)} = \frac{\text{Travel Time }(\mu\text{s})}{58.0}$$

---

## 📂 Experiment Files

* **[06-Ultrasonic-Ranging.ino](file:///f:/AlphaBot2/R4Experiments/06-Ultrasonic-Ranging/06-Ultrasonic-Ranging.ino)**: Reads the ultrasonic sensor, calculates distance using pulse timing, and prints real-time telemetry to the Serial Monitor.

---

## 📝 Lab Procedure & Student Tasks

> [!IMPORTANT]
> To ensure the robot runs immediately on boot without networking delay or connection timeouts, Wi-Fi, OTA, and Telnet Stream have been disabled in this sketch. The robot will read and react to IR signals immediately upon boot/reset. Status monitoring is done entirely over the **USB Serial Monitor** (115200 baud).

### Task 1: Calibrate and Verify Distance Accuracy
1. Open [06-Ultrasonic-Ranging.ino](file:///f:/AlphaBot2/R4Experiments/06-Ultrasonic-Ranging/06-Ultrasonic-Ranging.ino).
2. Upload the code to the robot using PowerShell:
   ```powershell
   .\manage.ps1 upload R4Experiments\06-Ultrasonic-Ranging
   ```
3. Open the **Serial Monitor** at **115200 baud**.
4. Set up a ruler or tape measure on a flat table. Align the front face of the ultrasonic sensor with the $0\text{ cm}$ mark.
5. Place a flat, solid object (like a book or notebook) at the distances listed in the table below. Record the distance printed on the Serial Monitor.

| Ruler Target Distance | Serial Monitor Reading | Calculated Error (%) |
| :---: | :---: | :---: |
| **5.0 cm** | *Measure* | |
| **10.0 cm** | *Measure* | |
| **15.0 cm** | *Measure* | |
| **20.0 cm** | *Measure* | |
| **30.0 cm** | *Measure* | |
| **50.0 cm** | *Measure* | |

* **Error Formula**:
  $$\text{Error } \% = \frac{|\text{Ruler Distance} - \text{Serial Reading}|}{\text{Ruler Distance}} \times 100$$

---

### Task 2: Implement Autonomous Collision Avoidance
Modify the sketch so that the AlphaBot2 drives forward but stops automatically before hitting an obstacle:
1. In `loop()`, command the motors to drive forward at speed **80** (re-use the H-bridge command logic from Experiment 01/03).
2. Constantly check the distance.
3. If the distance drops **below 15 cm**, immediately stop the motors and turn on the buzzer (use PCF8574 pin `P5` to make a beep!).
4. If the distance goes back **above 20 cm** (obstacle cleared), turn off the buzzer and resume driving forward.

---

## ❓ Post-Lab Questions for Students
1. Why is there a minimum detection distance (approx. 2 cm) for the HC-SR04 sensor? (Hint: Think about how fast the microcontroller can toggle pins and detect a pulse compared to the speed of sound).
2. How would a significant change in room temperature (e.g. from $15^\circ\text{C}$ in winter to $35^\circ\text{C}$ in summer) affect the distance measurements? 
3. If the sensor is pointing at a soft velvet curtain or acoustic foam instead of a solid plastic wall, how does this affect the echo detection?
4. How does the sensor handle obstacles that are placed at a $45^\circ$ angle relative to the sensor face?
