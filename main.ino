/*

*/

#include "EspMQTTClient.h"
#include <Adafruit_NeoPixel.h>

const byte pir1 = 5; //D1
const byte pir2 = 4; //D2
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0
int sensorValue = 0;

int interruptCounter = 0;
int ledstatus = 0;
int alarmPinTop = 5;        // PIR at the top of the stairs
int alarmPinBottom =4;      // PIR at the bottom of the stairs
int alarmValueTop = LOW;    // Variable to hold the PIR status
int alarmValueBottom = LOW; // Variable to hold the PIR status
int downUp = 0;              // variable to rememer the direction of travel up or down the stairs
unsigned long timeOut=60000; // timestamp to remember when the PIR was triggered.

int stairs = 30;  // Total number of stairs
unsigned long imillis;
unsigned long ledmillis;
String topic = "mytopic/stairs";

#define PIN        0  //D3 pin for the NEOPIXEL
#define NUMPIXELS 30  //Total leds aka ledsStair = Stairs
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int ledsStair = NUMPIXELS/stairs;

int outputpin= A0;

EspMQTTClient client(
  "Esperyd",
  "Esperyd4",
  "192.168.10.100",  // MQTT Broker server ip
  "mqtt",   // Can be omitted if not needed
  "mqtt",   // Can be omitted if not needed
  "ESP_Stairs",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

void setup()
{
  Serial.begin(115200);

  // Optionnal functionnalities of EspMQTTClient : 
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

//attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, HIGH);
//pinMode(pir1,INPUT);
//pinMode(pir2,INPUT);
pinMode(alarmPinTop, INPUT_PULLUP);     // for PIR at top of stairs initialise the input pin and use the internal restistor
pinMode(alarmPinBottom, INPUT_PULLUP);  // for PIR at bottom of stairs initialise the input pin and use the internal restistor
imillis = millis();
 strip.begin();
 
 
 strip.setBrightness(80); //adjust brightness here
 strip.clear();
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("mytopic/test", [](const String & payload) {
    Serial.println(payload);
  });

}


   



/*ICACHE_RAM_ATTR void handleInterrupt() {
  interruptCounter++;
  Serial.print(".");
}*/

 void bottomup() {
    Serial.println ("detected bottom");            // Helpful debug message
    colourWipeUp(strip.Color(random(50,255), random(50,255),random(50,255)), 30);  // Warm White

  }

void topdown() {
    Serial.println ("detected top");                  // Helpful debug message
    colourWipeDown(strip.Color(250, 250, 250), 30 );  // Warm White
 }
 
void checkInterupt() {
      // if(interruptCounter > 0 ){
      imillis = millis();
      ledmillis = imillis;
      interruptCounter = 0;
      if (ledstatus == 0) {
        client.publish(topic, "Turn on LED");
        Serial.println("Starting led ");
        Serial.print("Light is: ");
        Serial.println(sensorValue);
        bottomup();
        ledstatus = 1;
      }
  //}
}





   // Fade light each step strip
 void colourWipeUp(uint32_t c, uint16_t wait) {
   for (uint16_t j = stairs; j > 0; j--){
   int start = strip.numPixels()/stairs *j; //number of stairs in total
   //Serial.println(j);
          for (uint16_t i = start; i > start - ledsStair; i--){  //number of leds per stair
            strip.setPixelColor(i-1, c);
         }
          strip.show();
  delay(wait);
  }  
  
 }

  void colourWipeDown(uint32_t c, uint16_t wait) {
  uint8 out[4];
  *(uint32*)&out = c;
  
uint32_t c2 = (strip.Color(out[2]/2,out[1]/2,out[0]/2));
uint32_t c3 = (strip.Color(out[2]/10,out[1]/5,out[0]/5));
uint32_t c4 = (strip.Color(out[2]/40,out[1]/40,out[0]/40));

  for (uint16_t j = 0; j < stairs+4; j++){
  int start = strip.numPixels()/stairs *j;

      strip.fill(c,start-4*ledsStair,ledsStair);
      strip.fill(c2,start-3*ledsStair,ledsStair);
      strip.fill(c3,start-2*ledsStair,ledsStair);
      strip.fill(c4,start,ledsStair);
      Serial.println(start);
      Serial.println(ledsStair);
      strip.show();  
  delay(wait);
  }
  
 
 }
 
void poweroffLed() {
  if (ledmillis + 2000 < millis() && ledstatus == 1) {
    Serial.println("Turning off led");
    client.publish(topic, "Turn off LED");
    colourWipeUp(strip.Color(0, 0, 0), 30);   // Off
    ledstatus = 0;

}
}




// ######################### LOOOOP ############################
void loop()
{

    alarmValueTop = digitalRead(alarmPinTop);    // Constantly poll the PIR at the top of the stairs
    //Serial.println(alarmPinTop);
    alarmValueBottom = digitalRead(alarmPinBottom);  // Constantly poll the PIR at the bottom of the stairs
    //Serial.println(alarmPinBottom);
    int analogValue = analogRead(outputpin);
    
    
    if ((alarmValueTop == HIGH || alarmValueBottom == HIGH) && analogValue < 200) {  //Only work with the system if something is triggered
      timeOut=millis();  // Timestamp when the PIR is triggered.  The LED cycle wil then start.
       
      if (alarmValueTop == HIGH && downUp != 2)  {      // the 2nd term allows timeOut to be contantly reset if one lingers at the top of the stairs before decending but will not allow the bottom PIR to reset timeOut as you decend past it.
        downUp = 1;
        if (ledstatus == 0) {
          topdown();         // lights up the strip from top down
          Serial.println(analogValue);
          ledstatus = 1;
        }
      } 
      if (alarmValueBottom == HIGH && downUp != 1)  {    // the 2nd term allows timeOut to be contantly reset if one lingers at the bottom of the stairs before decending but will not allow the top PIR to reset timeOut as you decend past it.
        downUp = 2;
        if (ledstatus == 0) {
          bottomup();         // lights up the strip from bottom up
          Serial.println(analogValue);
          ledstatus = 1;
        }
      }

    }

    
   if (timeOut+8000 < millis() && downUp > 0) {    //switch off LED's in the direction of travel.
       if (downUp == 1) {
          colourWipeDown(strip.Color(0, 0, 0), 100); // Off
       }
       if (downUp == 2)  {
        colourWipeUp(strip.Color(0, 0, 0), 100);   // Off
       }
      downUp = 0;
      ledstatus = 0;
      Serial.println("Stairs light have been shut off");
   }

   
    

         
 /* 
if (digitalRead(pir1) || digitalRead(pir2)) {
 checkInterupt();
}
if (ledstatus == 0) {
  sensorValue = analogRead(analogInPin);
}
*/

//poweroffLed();
client.loop();
delay(5);
}
