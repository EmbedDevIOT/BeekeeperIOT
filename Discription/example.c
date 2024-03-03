char phone_no[]="+123456789012"; // Your phone number that receive SMS with counry code 
// NeverSleep
#include <SoftwareSerial.h> // Sofrware serial library
#include "HX711.h" // HX711 lib. https://github.com/bogde/HX711
#include <EEPROM.h> // EEPROM lib.

HX711 scale0(10, 14);
HX711 scale1(11, 14);
HX711 scale2(12, 14);
#define SENSORCNT 3
HX711 *scale[SENSORCNT];

SoftwareSerial mySerial(5, 4); // Set I/O-port TXD, RXD of GSM-shield  

float delta00; // delta weight from start
float delta10;
float delta20;
float delta01; // delta weight from yesterday
float delta11;
float delta21;

float raw00; //raw data from sensors on first start
float raw10;
float raw20;
float raw01; //raw data from sensors on yesterday
float raw11;
float raw21;
float raw02; //actual raw data from sensors
float raw12;
float raw22;

word calibrate0=20880; //calibration factor for each sensor
word calibrate1=20880;
word calibrate2=20880;

word daynum=0; //numbers of day after start

int notsunset=0;

boolean setZero=false;
boolean forceSend=false;

char ch = 0;
char ch1 = 0;
char ch2 = 0;
char ch3 = 0;
char ch4 = 0;

void readVcc() // read battery capacity 
{
  ch = mySerial.read();
   while (mySerial.available() > 0) {  ch = mySerial.read(); } // empty input buffer from modem

 mySerial.println("AT+CBC?"); //ask gprs for battery status (for sim800 and neoway command must be "AT+CBC" )
 delay(200);
 while (mySerial.available() > 0) { //read input string between coma and CR
 ch = mySerial.read();
     if (ch ==','){ 
       ch1 = mySerial.read();
       ch2 = mySerial.read();
       ch3 = mySerial.read();
       ch4 = mySerial.read();       
     }
   }
}

// **********************************************************************
void SendStat() 
{
  detachInterrupt(digitalPinToInterrupt(0)); // turn off external interrupt
   digitalWrite(13, HIGH);  

  if (!forceSend){
  notsunset=0;
 for (int i=0; i <= 250; i++){
      if ( !digitalRead(2) ){ notsunset++; } //is a really sunset now? you shure?
      delay(360);
   }
  }
  if ( notsunset==0 || forceSend )
  { 

  raw01=raw02;
  raw11=raw12;
  raw21=raw22;
  
  raw02=scale0.get_units(16); //read data from scales
  raw12=scale1.get_units(16);
  raw22=scale2.get_units(16);

  daynum++; 
  delta00=(raw02-raw00)/calibrate0; // calculate weight changes 
  delta01=(raw02-raw01)/calibrate0;
  delta10=(raw12-raw10)/calibrate1;
  delta11=(raw12-raw11)/calibrate1; 
  delta20=(raw22-raw20)/calibrate2;
  delta21=(raw22-raw21)/calibrate2;

  readVcc(); 
  delay(200);
  mySerial.println("AT+CMGF=1");    //  Part of SMS sending 
  delay(2000);
  mySerial.print("AT+CMGS=\"");
  mySerial.print(phone_no); 
  mySerial.write(0x22);
  mySerial.write(0x0D);  // hex equivalent of Carraige return    
  mySerial.write(0x0A);  // hex equivalent of newline
  delay(2000);
  mySerial.print("Turn ");
  mySerial.println(daynum);
  mySerial.print("Hive1  ");
  mySerial.print(delta01);
  mySerial.print("   ");
  mySerial.println(delta00);
  mySerial.print("Hive2  ");
  mySerial.print(delta11);
  mySerial.print("   ");
  mySerial.println(delta10);
  mySerial.print("Hive3 ");
  mySerial.print(delta21);
  mySerial.print("   ");
  mySerial.println(delta20);

  mySerial.print("Battery capacity is ");
  mySerial.print(ch1);
  mySerial.print(ch2);
  mySerial.print(ch3);
  mySerial.print(ch4);
  mySerial.println(" %");
  if (forceSend) {mySerial.print("Forced SMS");}
  mySerial.println (char(26));//the ASCII code of the ctrl+z is 26
  delay(3000);

  }
  forceSend=false;
digitalWrite(13, LOW);
attachInterrupt(0, SendStat , RISING); // Interrupt by HIGH level 
}
// *************************************************************************************************

void switchto9600()
{
mySerial.begin(115200); // Open software serial port
delay(16000); // wait for boot
mySerial.println("AT");
delay(200);
mySerial.println("AT");
delay(200);
mySerial.println("AT+IPR=9600");    //  Change Serial Speed 
delay(200);
mySerial.begin(9600);
mySerial.println("AT&W0");
delay(200);
mySerial.println("AT&W");
}

void setup() { // Setup part run once, at start

  pinMode(13, OUTPUT);  // Led pin init
  pinMode(2, INPUT_PULLUP); // Set pullup voltage
  Serial.begin(9600);

// -------------------------------------------------------------------------------

switchto9600(); // switch module port speed

// -------------------------------------------------------------------------------

mySerial.begin(9600);

delay(200);
scale[0] = &scale0; //init scale
scale[1] = &scale1;
scale[2] = &scale2;

scale0.set_scale();
scale1.set_scale();
scale2.set_scale();

delay(200);

setZero=digitalRead(2);

//if (EEPROM.read(500)==EEPROM.read(501) || setZero) // first boot/reset with hiding photoresistor
if (setZero)
{
raw00=scale0.get_units(16); //read data from scales
raw10=scale1.get_units(16);
raw20=scale2.get_units(16);
EEPROM.put(500, raw00); //write data to eeprom
EEPROM.put(504, raw10);
EEPROM.put(508, raw20);
for (int i = 0; i <= 24; i++) { //blinking LED13 on reset/first boot
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
  }
}
else {
EEPROM.get(500, raw00); // read data from eeprom after battery change
EEPROM.get(504, raw10);
EEPROM.get(508, raw20);
digitalWrite(13, HIGH); // turn on LED 13 on 12sec. 
    delay(12000);
digitalWrite(13, LOW);
}

delay(200); // Test SMS at initial boot

readVcc();
delay(200);
  mySerial.println("AT+CMGF=1");    
  delay(2000);
  mySerial.print("AT+CMGS=\"");
  mySerial.print(phone_no); 
  mySerial.write(0x22);
  mySerial.write(0x0D);  // hex equivalent of Carraige return    
  mySerial.write(0x0A);  // hex equivalent of newline
  delay(2000);
  mySerial.println("INITIAL BOOT OK");

  mySerial.print("Battery capacity is ");
  mySerial.print(ch1);
  mySerial.print(ch2);
  mySerial.print(ch3);
  mySerial.print(ch4);
  mySerial.println(" %");
  mySerial.println (char(26));//the ASCII code of the ctrl+z is 26
  delay(3000);

raw02=raw00;
raw12=raw10;
raw22=raw20;

attachInterrupt(0, SendStat , RISING); // Interrupt by HIGH level

}

void loop() {

digitalWrite(13, LOW);
ch=mySerial.read();
if ( ch=='R' ) { //wait first lerrer from "RING" string
forceSend=true;
mySerial.println("ATH");
SendStat();
ch=' ';
}

}