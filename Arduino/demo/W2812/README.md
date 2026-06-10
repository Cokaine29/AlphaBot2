# RGB LED (`W2812`)

This program demonstrates how to control the 4 serial-controlled **WS2812B RGB LEDs** located at the bottom of the AlphaBot2 chassis.

---

## 🔌 Hardware Setup

*   **Quantity**: 4 Addressable RGB LEDs (Indexed `0` to `3`).
*   **Data Pin**: Connected to **Digital Pin 7** on the Arduino.
*   **Library**: Uses the `Adafruit_NeoPixel` library (included in `Arduino/libraries`).

---

## 🎨 Color Mapping (Default Program)

When the code runs, it sets a distinct color for each of the 4 LEDs:

| LED Index | Position on Chassis | Color | RGB Value |
| :--- | :--- | :--- | :--- |
| **`0`** | Bottom Left | **Red** | `(255, 0, 0)` |
| **`1`** | Bottom Center-Left | **Green** | `(0, 255, 0)` |
| **`2`** | Bottom Center-Right | **Blue** | `(0, 0, 255)` |
| **`3`** | Bottom Right | **Yellow** | `(255, 255, 0)` |

---

## 📊 Flowchart

```mermaid
graph TD
    Start([Start Program]) --> InitPixel[Initialize Adafruit_NeoPixel object:<br>Set Pin 7, count = 4, GRB mode]
    InitPixel --> BeginPixel[Run RGB.begin:<br>Configure Pin 7 as output data line]
    BeginPixel --> Color0[Set LED 0: Red]
    Color0 --> Color1[Set LED 1: Green]
    Color1 --> Color2[Set LED 2: Blue]
    Color2 --> Color3[Set LED 3: Yellow]
    Color3 --> Show[Run RGB.show:<br>Transmit pixel colors to physical LEDs]
    Show --> IdleLoop[Idle Loop:<br>Empty loop leaves LEDs statically lit]
    IdleLoop --> IdleLoop
```
