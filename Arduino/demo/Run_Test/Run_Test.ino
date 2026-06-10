
#define PWMA   6           //Left Motor Speed pin (ENA)
#define AIN2   A0          //Motor-L forward (IN2).
#define AIN1   A1          //Motor-L backward (IN1)
#define PWMB   5           //Right Motor Speed pin (ENB)
#define BIN1   A2          //Motor-R forward (IN3)
#define BIN2   A3          //Motor-R backward (IN4)
  
void setup() {
  // Configure speed control pins as outputs
  pinMode(PWMA, OUTPUT);                     
  pinMode(PWMB, OUTPUT);       

  // Configure Left Motor direction pins as outputs
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);      
  
  // Configure Right Motor direction pins as outputs (Fixed: previously duplicated AIN1/AIN2 instead of BIN1/BIN2)
  pinMode(BIN1, OUTPUT);     
  pinMode(BIN2, OUTPUT);  

  // Set initial speed (50 out of 255 - slow & steady)
  analogWrite(PWMA, 50);
  analogWrite(PWMB, 50);

  // Set Left Motor to rotate forward
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  // Set Right Motor to rotate forward
  digitalWrite(BIN1, LOW); 
  digitalWrite(BIN2, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:

}
