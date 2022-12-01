/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on NodeMCU.

  Note: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right NodeMCU module
  in the Tools -> Board menu!

  For advanced settings please follow ESP examples :
   - ESP8266_Standalone_Manual_IP.ino
   - ESP8266_Standalone_SmartConfig.ino
   - ESP8266_Standalone_SSL.ino

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */

// scotty note: see blynk PIR_detect_count app to control

#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
/*
   1MB flash sizee

   sonoff header
   1 - vcc 3v3
   2 - rx
   3 - tx
   4 - gnd
   5 - gpio 14

   esp8266 connections
   gpio  0 - button
   gpio 12 - relay
   gpio 13 - green led - active low
   gpio 14 - pin 5 on header

*/

#define   SONOFF_BUTTON             0
#define   SONOFF_INPUT              14  // this was used for input on old version of sonoff, pin no longer broken out
#define   SONOFF_LED                13
#define   SONOFF_AVAILABLE_CHANNELS 1
#define   SONOFF_RX                 3  // use this for input on sonoff of all new versions
#define   SONOFF_TX                 1
const int SONOFF_RELAY_PINS[4] =    {12, 12, 12, 12};
#define SONOFF_RELAY  12
//if this is false, led is used to signal startup state, then always on
//if it s true, it is used to signal startup state, then mirrors relay state
//S20 Smart Socket works better with it false
#define SONOFF_LED_RELAY_STATE      false

#define HOSTNAME "sonoff"

#define Brg_reset_count 1
#define Brg_on    2
#define Brg_off   3
#define Brg_daynight_1 4
#define Brg_daynight_0 5

// You should get Auth Token in the Blynk App.
char auth[] = "...F9R";          //sonoff_1 device slave device B ( pbc condo 119 prototype)
char auth_Bridge2[] = "...L-Z";  //sonoff_2 device slave device A (2nd floor rear window)
char auth_Bridge3[] = "...xWB";  //sonoff_3 device slave device C  (loby table light)
char auth_Bridge4[] = "...5Zg";  //sonoff_4 device master device D (2nd floor flood light next to router)

const int motionSensor = SONOFF_RX;
//const int motionSensor = 4;  // D2 on nodemcu didn't get rx to work on nodemcu
int detect_count = 0;
// timer_event is what we set to start timeing from triger time.  we add millis() + a value in time we want to end the event
unsigned long timer_event = millis();
// default trig_time_min the time the light will remain on after a motion detection trigered
int trig_time_min = 15;
int daynight = 0;  // this value should be changes by blyk eventor timer, 1 for day time, 0 night for night time

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Wifi";
char pass[] = "password";

#ifndef STASSID
#define STASSID "Wifi"
#define STAPSK  "password"
#endif

const char* ssidota = STASSID;
const char* password = STAPSK;
BlynkTimer timer;
// led1 is the pwr status of the relay on or off
WidgetLED led1(V8);
// led2 is the daynight status led seen on app for both device A and B
WidgetLED led2(V5);
WidgetBridge bridge1(V22);
WidgetBridge bridge2(V23);
WidgetBridge bridge3(V24);
WidgetRTC rtc;
int ledstate = 1;
// set default power up state on or off here
int state_sonoff = 0;

int hour_day = 6;
int min_day = 0;

int hour_night = 17;
int min_night = 30;
int disable_master = 1;
int false_alarm = 0;
unsigned long last_triger_time = millis();
unsigned long min_triger_time = 1000000000L;

void myTimerEvent(){
  ledstate = !ledstate;
  int val = 0;
  //digitalWrite(D1, ledstate); // togle the LED hooked to D1 on and off
  if (digitalRead(SONOFF_RELAY) == 1){
    led1.on();
     Blynk.virtualWrite(V1, 1);
  }else{
    led1.off();
    Blynk.virtualWrite(V1, 0);
  }
  Blynk.virtualWrite(V4,trig_time_min); 
   
  if (digitalRead(SONOFF_BUTTON)==0){
    if (digitalRead(SONOFF_RELAY) == 0){
      timer_event = (millis()+(trig_time_min * 60000L ));
      setState(1,0);
      // set to daytime so that events don't change state, manual override
      daynight = 1;
    }else{
      setState(0,0);
      daynight = 1;
    }
  }
  // hour() and minute() is updated by rtc no need remote bridge any more
  //if (hour()==hour_night && minute()==min_night){
  if (hour()>=hour_night || hour()<=hour_day){
    if (minute() >= min_night){
      daynight = 0;
      Blynk.virtualWrite(V10,"daynight set 0: ");  
    } else {
      daynight =1;
    }
  } else {
    daynight = 1;
  }
  
  if (daynight == 1){
    led2.on();
  }else{
    led2.off();
  }

  clockDisplay();
  
  // not sure this led_builtin is working on sonoff, need to use the SONOFF_LED
  digitalWrite(LED_BUILTIN, ledstate);
  //Serial.println("digitalread motionSensor");
  //Serial.println(digitalRead(motionSensor)); 
  if ( timer_event <= millis()){
    setState(0,0); // turn off relay if timer_event runs out  
  }
}

BLYNK_WRITE(V44){
  //v44 is setting of hour_night value from app
  hour_night = param.asInt();
  bridge1.virtualWrite(V44, hour_night);
  bridge2.virtualWrite(V44, hour_night);
  bridge3.virtualWrite(V44, hour_night);
}

BLYNK_WRITE(V45){
  //v44 is setting of hour_night value from app
  min_night = param.asInt();
  bridge1.virtualWrite(V45, min_night);
  bridge2.virtualWrite(V45, min_night);
  bridge3.virtualWrite(V45, min_night);
}

BLYNK_WRITE(V46){
  //v44 is setting of hour_night value from app
  hour_day = param.asInt();
  bridge1.virtualWrite(V46, hour_day);
  bridge2.virtualWrite(V46, hour_day);
  bridge3.virtualWrite(V46, hour_day);
}

BLYNK_WRITE(V47){
  //v44 is setting of hour_night value from app
  min_day = param.asInt();
  bridge1.virtualWrite(V47, min_day);
  bridge2.virtualWrite(V47, min_day);
  bridge3.virtualWrite(V47, min_day);
}


void setState(int state, int channel) {
  //relay
  //digitalWrite(SONOFF_RELAY_PINS[channel], state);
  digitalWrite(SONOFF_RELAY, state);
  //Serial.println("setState ");
  //Serial.println(state);
  //led
  digitalWrite(SONOFF_LED, (state + 1) % 2); // led is active low 
  Blynk.virtualWrite(V1, state);  
}

void blink_led_3_times(){
  digitalWrite(SONOFF_LED, 1);
  delay(200);
  digitalWrite(SONOFF_LED, 0);
  delay(200);
  digitalWrite(SONOFF_LED, 1);
  delay(200);
  digitalWrite(SONOFF_LED, 0);
  delay(200);
  digitalWrite(SONOFF_LED, 1);
  delay(500);
  digitalWrite(SONOFF_LED, 0);
}

void clockDisplay()
{
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details

  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  //Serial.print("Current time: ");
  //Serial.print(currentTime);
  //Serial.print(" ");
  //Serial.print(currentDate);
  //Serial.println();

  // Send time to the App
  Blynk.virtualWrite(V11, currentTime);
  // Send date to the App
  Blynk.virtualWrite(V12, currentDate);
}


BLYNK_WRITE(V1){
  // now this botton just manualy starts a timer_event
 //V1 is botton to toggle sonoff relay on off
  //state_sonoff = param.asInt();
  if ((digitalRead(SONOFF_RELAY) == 0)&&(param.asInt() == 1)){
    //timer_event = millis()-2;
    timer_event = (millis()+(trig_time_min * 60000L ));
    setState(1,0);
    daynight = 0;
    // this bridge is only done from device A
    // device B does nothing commented out next line?
    if (disable_master==0){
      //bridge1.virtualWrite(V22, Brg_daynight_0);
      //bridge2.virtualWrite(V22, Brg_daynight_0);
      //bridge3.virtualWrite(V22, Brg_daynight_0);
      bridge1.virtualWrite(V22, Brg_on);
      bridge2.virtualWrite(V22, Brg_on);
      bridge3.virtualWrite(V22, Brg_on);
    } 
  }else{
    //timer_event = (millis()+(trig_time_min * 60000L ));
    //timer_event = millis()-1;
    if (param.asInt() == 1){
      setState(0, 0);
      daynight = 1;
      if (disable_master==0){
        //bridge1.virtualWrite(V22, Brg_daynight_1);
        //bridge2.virtualWrite(V22, Brg_daynight_1);
        //bridge3.virtualWrite(V22, Brg_daynight_1);
        bridge1.virtualWrite(V22, Brg_off);
        bridge2.virtualWrite(V22, Brg_off);
        bridge3.virtualWrite(V22, Brg_off);
      }  
    }
  }
  Serial.println("sonoff switch v1 change detected");
  //Serial.println(state_sonoff);
}



//unsigned long last_triger_time = 0;
//unsigned long min_triger_time = 10000000L;
unsigned long triger_filter_time = 1500L;
unsigned long max_triger_time = 0L;
unsigned long triger_timeout = 9500L;

ICACHE_RAM_ATTR void detectsMovement() {
  bool ledstate = digitalRead(SONOFF_LED);
  bool motionSensor_pin_state = digitalRead(motionSensor);
  //Serial.println("motionSenso pin state");
  //Serial.println(motionSensor_pin_state);
  //max_triger_time = triger_filter_time;
  //Blynk.virtualWrite(V18, false_alarm);
  Blynk.virtualWrite(V21,max_triger_time);
  Blynk.virtualWrite(V19,min_triger_time);
  Blynk.virtualWrite(V25,(millis() - last_triger_time));
 
  //Serial.println("diff last trig time");
  //Serial.println((millis() - last_triger_time));

  if ((millis() - last_triger_time) < triger_filter_time){ 
    false_alarm++;
    //Blynk.virtualWrite(V25,last_triger_time);
    last_triger_time=millis();
    Blynk.virtualWrite(V18, false_alarm);
    return;
  }

  if ((millis() - last_triger_time) > triger_timeout){
    false_alarm++;
    last_triger_time = millis();
    Blynk.virtualWrite(V18, false_alarm);
    return; 
  }
  
  
 
  if (max_triger_time < (millis() - last_triger_time)){
    max_triger_time = (millis() - last_triger_time);
   
  }
  if (min_triger_time > (millis() - last_triger_time)){
    min_triger_time = (millis() - last_triger_time);
    
  }
  Blynk.virtualWrite(V18, false_alarm);
  Blynk.virtualWrite(V21,max_triger_time);
  Blynk.virtualWrite(V19,min_triger_time);
  Blynk.virtualWrite(V25,(millis() - last_triger_time));
  
  last_triger_time=millis();
  digitalWrite(SONOFF_LED, !ledstate);
  //Serial.println("MOTION DETECTED!!!"); 
  detect_count++;
  //Serial.println("sensor detect count");
  //Serial.println(detect_count); 
  Blynk.virtualWrite(V16, detect_count); // added graph to this number works ok
  on_event();
}

BLYNK_WRITE(V20){
  // trig_filter_time entry
  triger_filter_time = param.asInt();
  bridge1.virtualWrite(V20, triger_filter_time);
  bridge2.virtualWrite(V20, triger_filter_time);
  bridge3.virtualWrite(V20, triger_filter_time);
}

BLYNK_WRITE(V26){
  // trig_filter_time entry
  triger_timeout = param.asInt();
  bridge1.virtualWrite(V26, triger_timeout);
  bridge2.virtualWrite(V26, triger_timeout);
  bridge3.virtualWrite(V26, triger_timeout);
}

void on_event(){
  if (daynight == 1){
    // it's daytime so don't need lights on yet
    // do nothing
    //return;
  }else{
    timer_event = (millis()+(trig_time_min * 60000L ));
    setState(1, 1);
    // device A is master and also turns on device B with on_event unless we comment out next line
    if (disable_master == 0){
      bridge1.virtualWrite(V22, Brg_on );
      bridge2.virtualWrite(V22, Brg_on );
      bridge3.virtualWrite(V22, Brg_on );
    }
  }  
}


BLYNK_WRITE(V6){
  // V6 is blynk reset botton to clear detect_count to zero
  int value = param.asInt();
  reset_count();
}

BLYNK_WRITE(V15){
  // disable master mode, turn off ability effect slaves
  disable_master = param.asInt();
}

void reset_count(){
  detect_count = 0;
  Blynk.virtualWrite(V16,0); 
  // doesn't matter what we send at this point if the call is bridge function is called B device will reset
  // maybe later we will have a small number of commands to be recieved at B to cause functions to fire.
  // bridge send bellow is not done on device B
  if (disable_master == 0){
     bridge1.virtualWrite(V22, Brg_reset_count);
     bridge2.virtualWrite(V22, Brg_reset_count);  
     bridge3.virtualWrite(V22, Brg_reset_count); 
  }   
}

BLYNK_WRITE(V3){
  // V3 is blynk trig time in minutes that is changed here from the app
  // this is the time the light or device stays on after a motion detect event
  trig_time_min = param.asInt();
  // we should be able to have any sonoff be master of trig time as long as they have token keys to do it
  //if (disable_master ==1){
  //  return;
  //}
  bridge1.virtualWrite(V3, trig_time_min);
  bridge2.virtualWrite(V3, trig_time_min);
  bridge3.virtualWrite(V3, trig_time_min); 
  //timer_event = (millis()+(trig_time_min * 60000L ));
  Serial.println("trig_time_min set:");
  Serial.println(trig_time_min);
}

BLYNK_CONNECTED() {
  // slaves at this time don't need bridge just here to minimize changes in file and maybe use later
  //bridge1.setAuthToken(auth_Bridge3); // Token of the hardware B note this device can't bridge to itself so commented out here
  bridge2.setAuthToken(auth_Bridge2); // Token of the hardware A note: bridge2 device A is now master
  bridge3.setAuthToken(auth_Bridge3); // Token of the hardware C  
  //bridge4.setAuthToken(auth_Bridge4); // Token of the hardware D  
  rtc.begin();
}


BLYNK_WRITE(V22){
  // not sure I need this method with case over a single V22, just bridge direct to to each devices Vxx function should work
  // bridge events bettween device A and B
  // device A is master device the only one that can reset both
  // device B sending reset does nothing to device A
  // example to use from other board:
  // bridge1.virtualWrite(V22, Brg_daynight_0); 
  // dev A a master so we will disable this part for now on device A
    int pinData = param.asInt(); //pinData variable will store value that came via Bridge
    Serial.println("bridge data");
    Serial.println(pinData);
    Blynk.virtualWrite(V10,"bridge data: ");
    Blynk.virtualWrite(V10,pinData);
    switch (pinData) {
       case 0:
         // not used yet
         break;
       case 1:
         // Brg_reset_count
         reset_count();
        break;
      case 2:
        // Brg_on turn on pwr, start timer
        timer_event = (millis()+(trig_time_min * 60000L ));
        setState(1, 1);
        daynight = 0;
        break;
      case 3:
        // Brg_off turn off pwr
         setState(0, 0);
         daynight = 1;
        break;
      case 4:
        // Brg_daynight_1  set daynight to 1 day time
        daynight = 1;
        break;
      case 5:
        // Brg_daynight_0  set daynight to 0 night time
        daynight = 0;
      case 6:
        // not used yet
      default:
        // this no longer needed we have direct bridge to each setup now
        // anything above 8 will be setting of trig_time_min
        // else trig_time_min will be set to minimum of 1
        //if (pinData < 8){
         // trig_time_min = 1;
        //}else{
         // trig_time_min = pinData;
        //}
        break;
      } 
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  pinMode(SONOFF_LED, OUTPUT); 
  pinMode(SONOFF_RELAY, OUTPUT); //relay
  pinMode(SONOFF_BUTTON, INPUT_PULLUP);// button on top of sonoff
  // didn't play with this yet as I needed a console to see it work
  // but pin 1
  //pinMode(1, INPUT_PULLUP);
  // make rx (GPIO 3) a standard gpio input pin
  //pinMode(3, INPUT_PULLUP);
  pinMode(motionSensor, INPUT_PULLUP);
 // Blynk.begin(auth, ssid, pass);
  Blynk.begin(auth, ssid, pass, "www.funtracker.site", 8080);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidota, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    //ESP.restart();
  }
  Blynk.virtualWrite(V16,0); // reset app detection count
  Blynk.virtualWrite(V3,trig_time_min); // set default trig time to 30 minutes 
  Blynk.virtualWrite(V44,hour_night);
  Blynk.virtualWrite(V45,min_night);
  Blynk.virtualWrite(V46,hour_day);
  Blynk.virtualWrite(V47,min_day);
  Blynk.virtualWrite(V15,disable_master);
  Blynk.virtualWrite(V20,triger_filter_time);
  Blynk.virtualWrite(V26,triger_timeout);
   
  // I noted to get ArduinoOTA to work I had to first upload the BasicOTA first.  after that installed ok then this one also installed
  // see File>examples>ArduinoOTA>BasicOTA, open it compile it and install it, then try this one again otherwise I would get error exit 1
  // I found the real problem was the motion sensor interupt that broke OTA.
  // this new version disables interupt when OTA is active.  work around to load this new version 
  // is to remove motion sensor from device until after loading new version.
  ArduinoOTA.setHostname("SONOFF");
  ArduinoOTA.setPassword("password");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  timer.setInterval(1000L, myTimerEvent);
  setSyncInterval(10 * 60); // Sync rtc interval in seconds (10 minutes)
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  blink_led_3_times();
  
}

void loop()
{  
  Blynk.run();  
  timer.run(); // Initiates BlynkTimer
  detachInterrupt(digitalPinToInterrupt(motionSensor));
  ArduinoOTA.handle();
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
}
