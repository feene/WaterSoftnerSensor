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
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 *
 * DESCRIPTION
 * This sketch provides an example how to implement a distance sensor using HC-SR04
 * http://www.mysensors.org/build/distance
 */

// Enable debug prints
  #define MY_DEBUG

//Includes some additional RFM69 Definces that are needed for Mysensors 2.2 and 2.3
#define MY_RADIO_RFM69
#define MY_RFM69_FREQUENCY RFM69_433MHZ
#define MY_IS_RFM69HW

#include <MySensors.h>

//#include <SPI.h>
#include <NewPing.h>
#include <SoftwareSerial.h>

//SoftwareSerial Pins
const int US100_TX = 5;
const int US100_RX = 6;

//Analog Pin to detect Battery Voltage
int BATTERY_SENSE_PIN = A0;
int oldBatteryPcnt = 0;

// Using a 2N7000 Mosfet.  Brings Idle current from 2.5ma to .07ma instead of vcc direct
int MOSFET_PIN = 4;


SoftwareSerial US100(US100_RX, US100_TX);

unsigned int MSByteDist = 0;
unsigned int LSByteDist = 0;
unsigned int mmDist = 0;
int temp = 0;


#define SKETCH_NAME "Water Softner Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

//Child ID for this sensor
#define CHILD_ID 4

unsigned long SLEEP_TIME = 3600000; // Sleep time between reads (in milliseconds)

MyMessage msg(CHILD_ID, V_DISTANCE);
int lastDist;
bool metric = true;

void setup()
{
 pinMode(MOSFET_PIN, OUTPUT);
 digitalWrite(MOSFET_PIN, HIGH);

  #if defined(__AVR_ATmega2560__)
    analogReference(INTERNAL1V1);
  #else
    analogReference(INTERNAL);
  #endif

  metric = getControllerConfig().isMetric;
  US100.begin(9600);
  //Serial.begin(9600);
  Serial.println("Setup Complete");
}


void presentation() {
  //Send the sketch version information to the gateway and Controller
  sendSketchInfo("Distance Sensor", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID, S_DISTANCE);
}

void loop()
{
  //Power Up US-100 Sensor.  Required after awake from sleep
  digitalWrite(MOSFET_PIN, HIGH);
  delay(500);

  //Measure Battery PCT
  int sensorValue = analogRead(BATTERY_SENSE_PIN);
  Serial.println(sensorValue);

  int batteryPcnt = sensorValue / 10;

  if (batteryPcnt >= 100) {
    batteryPcnt = 99;
  }
  float batteryV  = sensorValue * 0.003363075;
    Serial.print("Battery Voltage: ");
    Serial.print(batteryV);
    Serial.println(" V");

    Serial.print("Battery percent: ");
    Serial.print(batteryPcnt);
    Serial.println(" %");

    if (oldBatteryPcnt != batteryPcnt) {
      // Power up radio after sleep
      sendBatteryLevel(batteryPcnt);
      wait(500);
      oldBatteryPcnt = batteryPcnt;
  }

  //US-100 is in UART Mode.  Works well and is temp adjusted
  US100.flush();
  US100.write(0x55);
  delay(500);

  if(US100.available() >= 2)
    {
        MSByteDist = US100.read();
        LSByteDist  = US100.read();
        mmDist  = MSByteDist * 256 + LSByteDist;

        if((mmDist > 1) && (mmDist < 10000))
        {
            Serial.print("Distance ");
            Serial.print(mmDist, DEC);
            Serial.println(" mm");
        }
    }


  if (mmDist != lastDist) {
      send(msg.set(mmDist));
      lastDist = mmDist;
  }

  // Shutdown US-100 sensor
  digitalWrite(MOSFET_PIN, LOW);
  
  sleep(SLEEP_TIME);
}
