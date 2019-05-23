#include <Servo.h>
#include <SoftwareSerial.h>
SoftwareSerial GSM(P2_6,P2_7); // RX, TX

float pingTime;  //time for ping to travel from sensor to target and return
float targetDistance; //Distance to Target in inches
float speedOfSound=776.5; //Speed of sound in miles per hour when temp is 77 degrees.


Servo ser1;  //wet waste chamber gate ---calibrated at at 0 deg
Servo ser2;  //dry waste chamber gate ---calibrated at at 180 deg


int stoptime;         //intervals after which gsm must send data to server
int update_time=60;   //duration afer which data is sent to server via GSM module in sec 

//int trigPin1=2; 
//int echoPin1=5;
//int trigPin2=8; 
//int echoPin2=9;
//int trigPin3=10; 
//int echoPin3=11;

//int soil1=6;  //soil moisture should work on 3.3V supply
//int soil2=7;

float ultrasonic1_status=40.0;    //setting initital waste level status of empty bin for bothe ultrasonic sensors of both the chambers of wet and dry waste 
float ultrasonic2_status=40.0;


int wet_status;  //storing level of wet waste in wet waste chamber
int dry_status;   //storing level of dry waste in dry waste chamber
int rotten_status=0; //  ? rotten status(not being used in current version of project due to no interfacing of dht11)


enum _parseState {         //creating buffer for gsm
  PS_DETECT_MSG_TYPE,       

  PS_IGNORING_COMMAND_ECHO,

  PS_HTTPACTION_TYPE,     //setting http action i.e (GET or POST)
  PS_HTTPACTION_RESULT,   //receiving result
  PS_HTTPACTION_LENGTH,   //length of result

  PS_HTTPREAD_LENGTH,     //for reading length
  PS_HTTPREAD_CONTENT     //reading content of received response after getting length
};

byte parseState = PS_DETECT_MSG_TYPE;
char buffer[80];
byte pos = 0;

int contentLength = 0;

void resetBuffer() {      //resetting buffer after processing current response
  memset(buffer, 0, sizeof(buffer));
  pos = 0;
}

void sendGSM(const char* msg, int waitMs = 500) {  //to send data through GSM
  GSM.println(msg);
  delay(waitMs);
  while(GSM.available()) {
    parseATText(GSM.read());
  }
}

void setup()    
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  GSM.begin(9600);
  pinMode(2, OUTPUT);  //for seeting trigger and echo pins for the three ultrasonic sensors
  pinMode(5, INPUT);
  pinMode(8, OUTPUT);
  pinMode(9, INPUT);
  pinMode(10, OUTPUT);
  pinMode(11, INPUT);
}




void loop() 
{
  // put your main code here, to run repeatedly: 
  Serial.println(ultrasonic(10,11));
  if(ultrasonic(10,11)<17.0)    //detecting waste received at input mouth of container
  {
    delay(2000);
    if(soil_m1()>2.0||soil_m2()>2.0)  //checking moisture content in waste and segregating accordingly by opening gates of respected chamber by servo motors
    {
      servo1();
    }
    else
    {
      servo2();
    }
  }

  stoptime=millis();
  if((stoptime%(update_time*1000))==0)  //deciding status of bin at regular intervals
  {
    if(ultrasonic1_status>30.0)
    wet_status=1;
    if(ultrasonic1_status>20.0 && ultrasonic1_status<30.0)
    wet_status=2;
    if(ultrasonic1_status>5.0 && ultrasonic1_status<20.0)
    wet_status=3;
    if(ultrasonic2_status>30.0)
    dry_status=1;
    if(ultrasonic2_status>20.0 && ultrasonic2_status<30.0)
    dry_status=2;
    if(ultrasonic2_status>5.0 && ultrasonic2_status<20.0)
    dry_status=3;
    sendreq();    //sending data through gsm after deciding status of bin at regular intervals
  } 
  delay(500); 
}


float ultrasonic(int tpin,int epin)     //generalised code for usage of ultrasonic sensors to be used for detecting waste level(2 sensors) and entry of waste(1 sensor)
{
  digitalWrite(tpin, LOW); //Set trigger pin low
  delayMicroseconds(2000); //Let signal settle
  digitalWrite(tpin, HIGH); //Set trigPin high
  delayMicroseconds(15); //Delay in high state
  digitalWrite(tpin, LOW); //ping has now been sent
  delayMicroseconds(10); //Delay in low state
  
  pingTime = pulseIn(epin, HIGH);  //pingTime is presented in microceconds
  pingTime=pingTime/1000000; //convert pingTime to seconds by dividing by 1000000 (microseconds in a second)
  pingTime=pingTime/3600; //convert pingtime to hourse by dividing by 3600 (seconds in an hour)
  targetDistance= speedOfSound * pingTime;  //This will be in miles, since speed of sound was miles per hour
  targetDistance=targetDistance/2; //Remember ping travels to target and back from target, so you must divide by 2 for actual target distance.
  targetDistance= targetDistance*63360*2.54;    //Convert miles to inches by multipling by 63360 (inches per mile)  
  return targetDistance;
}


float soil_m1()     //reading soil moisture sensor 1 values
{
  float moisture_percentage;
  int sensor_analog;
  sensor_analog = analogRead(6);
  moisture_percentage = ( 100 - ( (sensor_analog/1023.00) * 100 ) );
  return moisture_percentage;
}

float soil_m2()  //reading soil moisture sensor 2 values
{
  float moisture_percentage2;
  int sensor_analog;
  sensor_analog = analogRead(7);
  moisture_percentage2 = ( 100 - ( (sensor_analog/1023.00) * 100 ) );
  return moisture_percentage2;
}

void servo1()   //executing servo motor 1 in case of wet waste detected
{
  ser1.attach(12);
  ser1.write(5);                  // sets the servo position according to the scaled value
  delay(1500); 
  ser1.write(50);                  // sets the servo position according to the scaled value
  delay(4500);                           // waits for the servo to get there
  ultrasonic1_status=ultrasonic(2,5);
  ser1.write(55);                  // sets the servo position according to the scaled value
  delay(1500);
  ser1.detach();
}

void servo2()               //executing servo motor 2 in case of wet waste detected
{
  ser2.attach(13);
  ser2.write(170);                  // sets the servo position according to the scaled value
  delay(1500); 
  ser2.write(120);                  // sets the servo position according to the scaled value
  delay(4500);                           // waits for the servo to get there
  ultrasonic2_status=ultrasonic(8,9);
  ser2.write(170);                  // sets the servo position according to the scaled value
  delay(1500);
  ser2.detach();
}

void sendreq()        //for sending data to server through GSM by AT commands
{
  sendGSM("AT+SAPBR=3,1,\"APN\",\"vodafone\"");  
  sendGSM("AT+SAPBR=1,1",3000);
  sendGSM("AT+HTTPINIT");  
  sendGSM("AT+HTTPPARA=\"CID\",1");
  if(dry_status==1 && wet_status==1 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=1&wet=1&rotten=0&bin_id=bin_01\"");
  if(dry_status==1 && wet_status==1 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=1&wet=1&rotten=1&bin_id=bin_01\"");
  if(dry_status==1 && wet_status==2 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=1&wet=2&rotten=0&bin_id=bin_01\"");
  if(dry_status==1 && wet_status==2 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=1&wet=2&rotten=1&bin_id=bin_01\"");
  if(dry_status==1 && wet_status==3 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=1&wet=3&rotten=0&bin_id=bin_01\"");
  if(dry_status==1 && wet_status==3 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=1&wet=3&rotten=1&bin_id=bin_01\"");

  if(dry_status==2 && wet_status==1 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=2&wet=1&rotten=0&bin_id=bin_01\"");
  if(dry_status==2 && wet_status==1 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=2&wet=1&rotten=1&bin_id=bin_01\"");
  if(dry_status==2 && wet_status==2 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=2&wet=2&rotten=0&bin_id=bin_01\"");
  if(dry_status==2 && wet_status==2 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=2&wet=2&rotten=1&bin_id=bin_01\"");
  if(dry_status==2 && wet_status==3 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=2&wet=3&rotten=0&bin_id=bin_01\"");
  if(dry_status==2 && wet_status==3 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=2&wet=3&rotten=1&bin_id=bin_01\"");

  if(dry_status==3 && wet_status==1 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=3&wet=1&rotten=0&bin_id=bin_01\"");
  if(dry_status==3 && wet_status==1 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=3&wet=1&rotten=1&bin_id=bin_01\"");
  if(dry_status==3 && wet_status==2 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=3&wet=2&rotten=0&bin_id=bin_01\"");
  if(dry_status==3 && wet_status==2 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=3&wet=2&rotten=1&bin_id=bin_01\"");
  if(dry_status==3 && wet_status==3 && rotten_status==0)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=3&wet=3&rotten=0&bin_id=bin_01\"");
  if(dry_status==3 && wet_status==3 && rotten_status==1)
  sendGSM("AT+HTTPPARA=\"URL\",\"http://www.smartjunk.tk/sjunk/?dry=3&wet=3&rotten=1&bin_id=bin_01\"");
  
  sendGSM("AT+HTTPACTION=0");
}

void parseATText(byte b) {  //for parsing received information in GSM during execution of AT commands

  buffer[pos++] = b;

  if ( pos >= sizeof(buffer) )
    resetBuffer(); // just to be safe

  switch (parseState) {
  case PS_DETECT_MSG_TYPE: 
    {
      if ( b == '\n' )
        resetBuffer();
      else {        
        if ( pos == 3 && strcmp(buffer, "AT+") == 0 ) {
          parseState = PS_IGNORING_COMMAND_ECHO;
        }
        else if ( b == ':' ) {
          //Serial.print("Checking message type: ");
          //Serial.println(buffer);

          if ( strcmp(buffer, "+HTTPACTION:") == 0 ) {
            Serial.println("Received HTTPACTION");
            parseState = PS_HTTPACTION_TYPE;
          }
          else if ( strcmp(buffer, "+HTTPREAD:") == 0 ) {
            Serial.println("Received HTTPREAD");            
            parseState = PS_HTTPREAD_LENGTH;
          }
          resetBuffer();
        }
      }
    }
    break;

  case PS_IGNORING_COMMAND_ECHO:
    {
      if ( b == '\n' ) {
        Serial.print("Ignoring echo: ");
        Serial.println(buffer);
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_TYPE:
    {
      if ( b == ',' ) {
        Serial.print("HTTPACTION type is ");
        Serial.println(buffer);
        parseState = PS_HTTPACTION_RESULT;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_RESULT:
    {
      if ( b == ',' ) {
        Serial.print("HTTPACTION result is ");
        Serial.println(buffer);
        parseState = PS_HTTPACTION_LENGTH;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_LENGTH:
    {
      if ( b == '\n' ) {
        Serial.print("HTTPACTION length is ");
        Serial.println(buffer);
        
        // now request content
        GSM.print("AT+HTTPREAD=0,");
        GSM.println(buffer);
        
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPREAD_LENGTH:
    {
      if ( b == '\n' ) {
        contentLength = atoi(buffer);
        Serial.print("HTTPREAD length is ");
        Serial.println(contentLength);
        
        Serial.print("HTTPREAD content: ");
        
        parseState = PS_HTTPREAD_CONTENT;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPREAD_CONTENT:
    {
      // for this demo I'm just showing the content bytes in the serial monitor
      Serial.write(b);
      
      contentLength--;
      
      if ( contentLength <= 0 ) {

        // all content bytes have now been read

        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;
  }
}
