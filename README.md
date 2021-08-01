# sonoff_PIR_detect
This is replacement firmware for  sonoff basic devices to add PIR surveillance recording using blynk lib
This software adds an inexpensive PIR (Passive Infrared) motion sensors to an inexpensive sonoff basic power control device.
This adds a few features including intrusion time data colection and auto light on and off during night hours.
in this version I only tested it with 3 sonoff devices at the same time.  We use blynks bridge feature to allow control and monitoring of multiple devices from a single android Iphone blynk app project screen.  You can in theory add as many devices to your bridged network as you want but will require manual modification of the ino file.

Features:
* Remotely turn on and off one of two devices individualy or all devices in the network at one time with the blynk app from anyplace on the planet.
* With PIR detection at night hours it will turn on the light or device conected on the detecting sonoff device 
* If master mode is set on the master unit, all lights in the brided network will come on for detection on that sonoff unit.
* Sensitivity of the PIR devices is setable at the app with triger filter time and triger timeout to prevent false alarms.
* Alarm trigers are counted and graphed on a time chart to view time line of intrusion
* False alarms are also counted and displayed on the app
* Blynks RTC feature is used to control the time window that lights will be triggered to turn on when PIR detects motion
* The time window of night and day is setable in the app to only turn on lights at dark
* The trigger time is settable that controls how long the lights will stay on after a PIR is detected.
* OTA (Over The Air) updates with arduino ide support is implemented after first serial programing the device

I used the arduino IDE to program the devices. To program you need to set board type to "Generic ESP8266 module". See other online sources (google it) for information that details the process of reprograming a sonoff basic device including youtube videos.

For the PIR device I used the Mini IR Pyroelectric Infrared PIR Motion Human Sensor Automatic Detector Module AM312 Sensor DC 2.7 to 12V HC-SR312 that are availble here for less that $1 USD or 27 thai baht at the time of this publishing.

For the sonoff I used the Sonoff basic V2 Smart Home WiFi Wireless Switch Module For Apple Android APP Control that at the time of publication cost about $4 here or 120 thai baht.  Note this should work on the newer version V3 and above of Sonoff as I found the new ones no longer have the added break out pin gpio 14 so I use the available serial input pin as the sensor input after serial programing is done.

 older version sonoff header
   1 - vcc 3v3   to AM312 +vcc
   2 - rx        to AM312 center detect signal pin
   3 - tx
   4 - gnd       to AM312 gnd 
   5 - gpio 14
   
   Newer vesions of sonoff are labled on the board so just use the above as a reference of signal names.
   
   Note: You must modify the Blynk tokens and your wifi AP name and password in the ino file to match your personal settings.
   
   Todo: At the time of publication I was unable to get the android Notification feature to work to notify when the alarm mode was activated with audio notification on your android or Iphone with an intrusion event.  Maybe I will leave that to someone else to fix some day.
   Also some devices are more sensitive to activation than others.  This may be due to placement of the device or less noise immunity on some PIR devices.  try placeing the device in locations with little change in light and motion as posible like hallways and such.  Maybe someday we will come up with better more inteligent false alarm filters to fix this.

