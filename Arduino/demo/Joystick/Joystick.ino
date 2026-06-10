#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PWMA   6           //Left Motor Speed pin (ENA)
#define AIN2   A0          //Motor-L forward (IN2).
#define AIN1   A1          //Motor-L backward (IN1)
#define PWMB   5           //Right Motor Speed pin (ENB)
#define BIN1   A2          //Motor-R forward (IN3)
#define BIN2   A3          //Motor-R backward (IN4)

#define Addr  0x20

#define OLED_RESET 9
#define OLED_SA0   8
Adafruit_SSD1306 display(OLED_RESET, OLED_SA0);

#define beep_on  PCF8574Write(0xDF & PCF8574Read())
#define beep_off PCF8574Write(0x20 | PCF8574Read())

byte value;
int Speed = 150;

// Speed offsets to calibrate wheel imbalance (Carried over -3 right wheel offset)
#define LEFT_SPEED_OFFSET   0  
#define RIGHT_SPEED_OFFSET -3  
void PCF8574Write(byte data);
byte PCF8574Read();
void forward();
void backward();
void right();
void left();
void stop();
void updateOLED(const char* text);

void setup() {
  Serial.begin(115200);
  Serial.println("Joystick example!!");
  Wire.begin();

  // Initialize OLED Screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("AlphaBot2");
  display.setCursor(10, 35);
  display.println("Joystick!");
  display.display();
  delay(1500);
  
  updateOLED("READY");
  
  // Configure speed control pins as outputs
  pinMode(PWMA, OUTPUT);                     
  pinMode(PWMB, OUTPUT);       

  // Configure Left Motor direction pins as outputs
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);      
  
  // Configure Right Motor direction pins as outputs (Fixed: previously duplicated AIN1/AIN2 instead of BIN1/BIN2)
  pinMode(BIN1, OUTPUT);     
  pinMode(BIN2, OUTPUT);  

  analogWrite(PWMA, 150);
  analogWrite(PWMB, 150);
  stop(); 
}

void loop() {
  PCF8574Write(0x1F | PCF8574Read());
  value = PCF8574Read() | 0xE0;
  if(value != 0xFF)
  {
    beep_on;
    switch(value)
    { 
      case 0xFE:
        forward();
        updateOLED("FORWARD");
        Serial.println("up");
        break; 
      case 0xFD:
        right();
        updateOLED("RIGHT");
        Serial.println("right");
        break;
      case 0xFB:
        left();
        updateOLED("LEFT");
        Serial.println("left");
        break; 
      case 0xF7:
        backward();
        updateOLED("BACKWARD");
        Serial.println("down");
        break;
      case 0xEF:
        forward();
        updateOLED("CENTER");
        Serial.println("center");
        break;
      default :
        updateOLED("UNKNOWN");
        Serial.println("unknow\n");
    }
    while(value != 0xFF)
    {
      PCF8574Write(0x1F | PCF8574Read());
      value = PCF8574Read() | 0xE0;
      delay(10);
    }
    stop();  
    updateOLED("STOP");
    beep_off;
  }
}

void PCF8574Write(byte data)
{
  Wire.beginTransmission(Addr);
  Wire.write(data);
  Wire.endTransmission(); 
}

byte PCF8574Read()
{
  int data = -1;
  Wire.requestFrom(Addr, 1);
  if(Wire.available()) {
    data = Wire.read();
  }
  return data;
}

void forward()
{
  analogWrite(PWMA, constrain(Speed + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(Speed + RIGHT_SPEED_OFFSET, 0, 255));
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);  
  digitalWrite(BIN2, HIGH); 
}

void backward()
{
  analogWrite(PWMA, constrain(Speed + LEFT_SPEED_OFFSET, 0, 255));
  analogWrite(PWMB, constrain(Speed + RIGHT_SPEED_OFFSET, 0, 255));
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); 
  digitalWrite(BIN2, LOW);  
}

void right()
{
  analogWrite(PWMA,50);
  analogWrite(PWMB,50);
  digitalWrite(AIN1,LOW);
  digitalWrite(AIN2,HIGH);
  digitalWrite(BIN1,HIGH); 
  digitalWrite(BIN2,LOW);  
}

void left()
{
  analogWrite(PWMA,50);
  analogWrite(PWMB,50);
  digitalWrite(AIN1,HIGH);
  digitalWrite(AIN2,LOW);
  digitalWrite(BIN1,LOW); 
  digitalWrite(BIN2,HIGH);  
}

void stop()
{
  analogWrite(PWMA,0);
  analogWrite(PWMB,0);
  digitalWrite(AIN1,LOW);
  digitalWrite(AIN2,LOW);
  digitalWrite(BIN1,LOW); 
  digitalWrite(BIN2,LOW);  
}

void updateOLED(const char* text)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(15, 20);
  display.println(text);
  display.display();
}
