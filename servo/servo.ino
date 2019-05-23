
#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
 
void setup() 
{ 
  myservo.attach(A2);  // attaches the servo on pin 9 to the servo object 
} 
 
 
void loop() 
{ 
   myservo.write(90); 
   delay(3000);
   myservo.write(180);
   delay(3000);
} 
