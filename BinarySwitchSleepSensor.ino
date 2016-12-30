/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Interrupt driven binary switch example with dual interrupts
 * Author: Patrick 'Anticimex' Fallberg
 * Connect one button or door/window reed switch between 
 * digitial I/O pin 3 (BUTTON_PIN below) and GND and the other
 * one in similar fashion on digital I/O pin 2.
 * This example is designed to fit Arduino Nano/Pro Mini
 * 
 */


// Enable debug prints to serial monitor
#define MY_DEBUG 

//send with ACK set?
#define NEED_ACK false

#define SLEEP_IN_MS 86400000 // 1 day
#define BATTERY_SENSE_PIN  A0  // select the input pin for the battery sense point

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>

#define SKETCH_NAME "BinarySwitchSleepSensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "2"

#define PRIMARY_CHILD_ID 3
#define SECONDARY_CHILD_ID 4

#define PRIMARY_BUTTON_PIN 2   // Arduino Digital I/O pin for button/reed switch
#define SECONDARY_BUTTON_PIN 3 // Arduino Digital I/O pin for button/reed switch

#if (PRIMARY_BUTTON_PIN < 2 || PRIMARY_BUTTON_PIN > 3)
#error PRIMARY_BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif
#if (SECONDARY_BUTTON_PIN < 2 || SECONDARY_BUTTON_PIN > 3)
#error SECONDARY_BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif
#if (PRIMARY_BUTTON_PIN == SECONDARY_BUTTON_PIN)
#error PRIMARY_BUTTON_PIN and BUTTON_PIN2 cannot be the same
#endif
#if (PRIMARY_CHILD_ID == SECONDARY_CHILD_ID)
#error PRIMARY_CHILD_ID and SECONDARY_CHILD_ID cannot be the same
#endif
 

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(PRIMARY_CHILD_ID, V_TRIPPED);
MyMessage msg2(SECONDARY_CHILD_ID, V_TRIPPED);

void setup()  
{  
  // Setup the buttons
  pinMode(PRIMARY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SECONDARY_BUTTON_PIN, INPUT_PULLUP);

  // Activate internal pull-ups
  digitalWrite(PRIMARY_BUTTON_PIN, HIGH);
  digitalWrite(SECONDARY_BUTTON_PIN, HIGH);
  
  // use the 1.1 V internal reference
#if defined(__AVR_ATmega2560__)
   analogReference(INTERNAL1V1);
#else
   analogReference(INTERNAL);
#endif
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  // void present(uint8_t sensorId, uint8_t sensorType, const char* description="", bool ack=false)
  present(PRIMARY_CHILD_ID, S_DOOR, "pin D2", NEED_ACK);  
  present(SECONDARY_CHILD_ID, S_DOOR, "pin D3", NEED_ACK);  
}

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  uint8_t value;
  static uint8_t sentValue=2;
  static uint8_t sentValue2=2;

  // Short delay to allow buttons to properly settle
  delay(5);
  
  value = digitalRead(PRIMARY_BUTTON_PIN);
  
  if (true /*value != sentValue*/) {
     // Value has changed from last transmission, send the updated value
     // bool send(MyMessage &msg, bool ack=false)
     send(msg.set(value==HIGH ? 1 : 0), true);
     sentValue = value;
  }

  value = digitalRead(SECONDARY_BUTTON_PIN);
  
  if (true /*value != sentValue2*/) {
     // Value has changed from last transmission, send the updated value
     send(msg2.set(value==HIGH ? 1 : 0), NEED_ACK);
     sentValue2 = value;
  }

  sendBatteryLevel(getBatteryLevel(), NEED_ACK);

  //define sleeptime as unsigned long and use UL specifier! or you get weird numbers :-((
  unsigned long sleeptime = 300000UL; // 10 minutes 1000*60*10 = 24h / 60minuten * 10
  #ifdef MY_DEBUG
  Serial.print("######## sleeptime: ");Serial.println(sleeptime);
  #endif
  // Sleep until something happens with the sensor
  //int8_t mysleep=sleep(PRIMARY_BUTTON_PIN-2, CHANGE, SECONDARY_BUTTON_PIN-2, CHANGE, /*0*/ sleeptime);
  int8_t mysleep=sleep(SECONDARY_BUTTON_PIN-2, CHANGE, sleeptime);
  //sleep returns -1 for timeout, or the PIN INT (first or second INT argument) that fired the interrupt!
  switch (mysleep){
    case -1:    
      send(msg2.set(value==HIGH ? 1 : 0), NEED_ACK);
      sendBatteryLevel(getBatteryLevel(), NEED_ACK);
      #ifdef MY_DEBUG
      Serial.println("######## wake up by time out");
      #endif
      break;
    case PRIMARY_BUTTON_PIN-2:
      //msg will be send above due to change
      #ifdef MY_DEBUG
      Serial.println("######## wake up PRIMARY_BUTTON_PIN change");
      #endif
      break;
    case SECONDARY_BUTTON_PIN-2:
      //msg will be send above due to change
      #ifdef MY_DEBUG
      Serial.println("######## wake up SECONDARY_BUTTON_PIN change");
      #endif
      break;
    default:
      #ifdef MY_DEBUG
      Serial.print("######## sleep returned: ");Serial.println(mysleep);
      #endif
      break;
  }
  //delay(sleeptime);
} 

// Battery measure
int getBatteryLevel () 
{
   // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
   #ifdef MY_DEBUG
   Serial.println(sensorValue);
   #endif
   
   // 1M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
   // 3.44/1023 = Volts per bit = 0.003363075
   
   int batteryPcnt = sensorValue / 10;

   #ifdef MY_DEBUG
   float batteryV  = sensorValue * 0.003363075;
   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
   #endif
   if(batteryPcnt>100)
    batteryPcnt=100;
   return batteryPcnt;
}
