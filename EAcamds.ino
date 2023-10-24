  /*
   This controls the arduino based sleepy pi 2 usb-c from spell foundry                                                                                                              
 http://sleepypi.com/
 
 The sleepy pi controls power on and shutdown-poweroff of the Raspberry Pi
 as well as puts itself into a low power sleep mode between wake 
 cycles.  This is accomplished via synchronous (timed) wake cycles programed
 via a config file on the Raspberry pi, as well as an asynchronous 3.3V pin signal 
 on pin 8 powered by the buoy data logger controller (DCL) via the 
 12V port power. The sleepy pi also monitors battery 
 voltage and will shutdown the Raspberry pi based on low battery voltage.
 
 Power from the DCL is stepped down from 12V to 3.3v using
 XP power TR2024S3V3 from Digikey:
 https://www.digikey.com/en/products/detail/xp-power/TR2024S3V3/11494617?s=N4IgTCBcDaICoCUwAYwBYDKBmAalkAugL5A
 and this is connected to both GPIO pin 8 on sleepy py as well as 
 GPIO pin P35 or G19 on the raspberry pi.
 
 Pi is determined to be on using PI GPIO pin 25 and a shutdown is triggered via pin 24
 Pi GPIO pins 14 RX and 15 TX are used for serial communication.
 
 GPIO 22 is used for arduino reset during programming
 
 ************************************************************************************
 Verision History
 1.0.0 2021-07-02 Initial version created by Christopher Holm holmch@oregonstate.edu
 1.0.1 2021-07-19 Edited DCL interrupt and brought better timing into while loop
 1.0.2 2021-07-22 Added ackTimer1() to include timer functionality, moved waketime around, changed calculation
 1.0.3 2021-08-13 Added additional serial req, added timer check, added enablePiPower CEH
 2.0.0 2021-08-25 Major rewrite of timer setting and movement of interrupts, attempt to repair unstable behavior CEH
 
 ************************************************************************************
 
 */

//****INCLUDED LIBRARIES*********
#include <LowPower.h>
#include <PCF8523.h>
#include <PinChangeInt.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include "SleepyPi2.h"
#include <avr/wdt.h>

//****** DEFINE CONSTANTS *******
#define NO_PORTC_PINCHANGES
#define DISABLE_PCINT_MULTI_SERVICE
#define NO_PIN_STATE
#define NO_PIN_NUMBER
#define KPI_CURRENT_THRESHOLD_MA 110    // Shutdown current threshold in mA
#define LOW_VOLTAGE_TIME_MS 30000ul     // 30 seconds
#define OVERRIDE_TIME_MS    300000ul    // 5 minutes
typedef enum {
  ePI_OFF = 0,
  ePI_BOOTING,
  ePI_ON,
  ePI_SHUTTING_DOWN
}ePISTATE;
const int LED_PIN = 13;
const int DCL_PIN = 8;
const int LIGHT_PIN = 5;
const int iterations = 3;                                // number of attempts 
eTIMER_TIMEBASE  PeriodicTimer_Timebase    = eTB_MINUTE; // Timebase set to minutes could be eTB_SECOND, eTB_HOUR

//****** DEFINE VARIABLES ********
int sampleInterval = 240;              // sampling interval in minutes
int dclState = 0;                      // dcl off = 0; dcl on = 1
int sampleTime = 30;                    // in minutes from midnight
int lightPower = 100;
int cutOffVoltage = 17;                  // this could be removed battery will do this anyway
int i = 0;
int check = 0;
int light = 255;
int t =0;
uint8_t wakeDay = 1;
uint8_t wakeHour = 0;
uint8_t wakeMin = 0;
uint8_t wakeTime = 0;
unsigned long wakeTimeMin = 27073440ul;     // sample time in minutes since 1970-01-01
unsigned long startDate = 1624406400ul;     // 2021-06-23 00:00 in seconds since 1970-01-01 00:00
unsigned long wakeTimeSec = 0ul;            // calculated wake time in seconds after now()
unsigned long wakeTimeEpoch = 1624406400ul; // startdate and sample time in seconds since 1970-01-01 00:00
unsigned long DCLtimeAwakeSec = 480;        // time sec to allow Rpi to stay awake if it doesn't shut down itself
unsigned long timeAwakeSec = 360;           // time sec to allow RPi on before shutdown with no DCL
unsigned long timeOutStart;                 // timer start in milliseconds
unsigned long elapsedTimeMS;                // Elapsted time in milliseconds
unsigned long timeOutEnd;                   // calculated time out end in milliseconds
unsigned long timeVoltageLow = 0;           // milliseconds that voltage is low
unsigned long timeNow = 1624406400ul;       // place holder for today in seconds since 1970-01-01
boolean alarmFired = false;                 // RTC Alarm has triggerd?
boolean pi_running = false;                 // Current indicates Pi is running?
boolean dcl_trigger = false;
ePISTATE  pi_state = ePI_OFF;
float supplyVoltage;                        // Sleepy measurement of battery voltage
float wakeDayFloat;
float wakeHourFloat;
float timeLeft = 5;
float timerMin = 0;
float timerSec = 0;
//******** DEFINE FUNCTIONS ********

// Function that handles serial Configuration            
void incomingConfigRead() {
  if (Serial.available() > 5) {
    Serial.read();
    i=0;
    while (i < iterations)  {
      Serial.read();
      Serial.println("sampletime!");
      delay(1400);
      if (Serial.available() > 2) {
          sampleTime = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(sampleTime);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            } 
            else  {
            i ++;
            sampleTime = 30;
            }
          }
          else {i++;}
      }
      else {i++;}
    }
    i = 0;
    delay(1000);
    while (i < iterations)  {
      Serial.read();
      Serial.println("startdate!");
      delay(1400);
      if (Serial.available() > 2) {
          startDate = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(startDate);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            }
            else  {
              startDate = 0;
              i ++;
            }
          }
          else {i++;}
      }
      else {i++;}
    }
    i = 0;
    delay(1000);
    while (i < iterations)  {
      Serial.read();
      Serial.println("interval!");
      delay(1400);
      if (Serial.available() > 2) {
          sampleInterval = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(sampleInterval);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            }
            else  {
              i ++;
              sampleInterval = 240;
            }
          }
          else {i++;}
      }
      else {i++;}
    }
    i = 0;
    delay(1000);
    while (i < iterations)  {
      Serial.read();
      Serial.println("lightpower!");
      delay(1400);
      if (Serial.available() > 2) {
          lightPower = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(lightPower);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            }
            else  {
              i ++;
              lightPower = 100;
            }
          }
          else {i++;}
      }
      else {i++;}
    }
    i = 0;
    delay(1000);
    while (i < iterations)  {
      Serial.read();
      Serial.println("dclOnDelay!");
      delay(1400);
      if (Serial.available() > 2) {
          DCLtimeAwakeSec = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(DCLtimeAwakeSec);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            }
            else  {
              i ++;
              DCLtimeAwakeSec = 480;
            }
          }
          else {i++;}
      }
      else {i++;}
    }
    i = 0;
    delay(1000);
    while (i < iterations)  {
      Serial.read();
      Serial.println("piOnDelay!");
      delay(1400);
      if (Serial.available() > 2) {
          timeAwakeSec = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(timeAwakeSec);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            }
            else  {
              i ++;
              timeAwakeSec = 360;
            }
          }
          else {i++;}
      }
      else {i++;}
    }
    i = 0;
    delay(1000);
    while (i < iterations)  {
      Serial.read();
      Serial.println("cutvolts!");
      delay(1400);
      if (Serial.available() > 2) {
          cutOffVoltage = Serial.parseInt();  
          Serial.read();
          Serial.print("received:");
          Serial.println(cutOffVoltage);
          delay(1600);
          if (Serial.available() > 1) {
            check = Serial.parseInt();
            if (check == 1) {
              i = iterations;
              check = 0;
              Serial.println();
            }
            else  {
              i ++;
              cutOffVoltage = 17;
            }
          }  
          else {i++;}
      }
      else {i++;}
    }
  Serial.println("DONE!");
  }
  else {delay(100);}
}

// Function that gets run at DCL Interrupt
void dcl_isr() {
  Serial.println("DCL ON from dcl_isr");
  digitalWrite(LIGHT_PIN, LOW);
  dcl_trigger = true;
  delay(100);
}

// Function to handle alarm interrupt isr's need to be limited so don't do much here
void alarm_isr() {
  Serial.println("Awake from Alarm interrupt");  
  alarmFired = true;
  dcl_trigger = false;
}

// Function that prints the current time
void printTimeNow() {
  // Read the time
  DateTime now = SleepyPi.readTime();

  // Print out the time
  Serial.print("Time = ");
  print2digits(now.hour());
  Serial.write(':');
  print2digits(now.minute());
  Serial.write(':');
  print2digits(now.second());
  Serial.print(", Date (Y/M/D) = ");
  Serial.print(now.year(),DEC);
  Serial.write('/');
  Serial.print(now.month()); 
  Serial.write('/');
  Serial.print(now.day());
  Serial.println();
  return;
}

// Function that adds leading Zeros
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

// Function that calculates next wake up time
uint8_t  CalcNextWakeTime(void)  {
  
  DateTime now = SleepyPi.readTime();              // set time now to RTC value
  wakeTimeEpoch = (startDate + (sampleTime * 60));   // calculate total wake time in seconds since 1970 
  timeNow = now.unixtime();                        // get now in seconds since 1970
  if (wakeTimeEpoch <= timeNow) { 
      i =0;
      while (wakeTimeEpoch + (i* (sampleInterval *60)) < timeNow) {
        i ++;
      }
      wakeTimeSec =  wakeTimeEpoch + i * (sampleInterval * 60) - timeNow;
    }
  else {
    wakeTimeSec =  wakeTimeEpoch - timeNow;
    }
  if (wakeTimeSec <  300) {
    wakeTimeMin = (wakeTimeSec / 60) + sampleInterval;
    i = 0;
    Serial.print("Next sample is less than 10 minutes from now adding ");
    Serial.print(sampleInterval);
    Serial.println(" minutes to the next wake time");
  }
  else { wakeTimeMin = wakeTimeSec / 60;}
  
  Serial.print("Camera will wake in ");
  Serial.print(wakeTimeMin);
  Serial.println(" minutes");
  
  if (wakeTimeMin > 255) {
    float wakeHourFloat = wakeTimeMin / 60;
    float wakeDayFloat = wakeHourFloat / 24;
    uint8_t wakeDay = wakeDayFloat;
    wakeHourFloat = (wakeDayFloat - wakeDay) * 24;
    uint8_t wakeHour = wakeHourFloat;
    uint8_t wakeMin = (wakeHourFloat - wakeHour) * 60; 
    Serial.print("Camera will wake in: ");
    Serial.print(wakeDay);
    Serial.print("days, ");
    Serial.print(wakeHour);
    Serial.print("hours, and, ");
    Serial.print(wakeMin);
    Serial.print("minutes");
    Serial.println(); 
  }
  else {
    wakeTime = wakeTimeMin;
    return wakeTime;
  }
}

//******** END OF DEFINITIONS*******

//******** SETUP FUNCTION *********
void setup()  {
  
  // Configure pins
  pinMode(LED_PIN, OUTPUT);     // Board LED    
  digitalWrite(LED_PIN,LOW);    // Switch off LED
  pinMode(DCL_PIN, INPUT);      // DCL Port Power
  pinMode(LIGHT_PIN,OUTPUT);    // Light Control Pin

  // initialize serial communication: In Arduino IDE use "Serial Monitor"
  Serial.begin(115200);
  Serial.println("Starting Serial Communication");
  delay(250);  
  Serial.setTimeout(3000);
  
  //set the initial power to be off
  SleepyPi.enablePiPower(false);
  SleepyPi.enableExtPower(false);
  SleepyPi.rtcInit(true);
}
//******** EXECUTE LOOP **********

void loop() {
  
  i=0;
  delay(150);
  
  // determine state of wake dcl interrupt or timer alarm or is it nonsense?
  
  if (pi_running == false) {
    delay(1000);
    if (digitalRead(DCL_PIN) == 1) {
      SleepyPi.enablePiPower(true);
      //SleepyPi.enableExtPower(true);  // this seems to pull down the voltage?
      delay(22000);
      ePISTATE  pi_state = ePI_BOOTING;
      Serial.println("DCL on Turning on Pi Power");
      if (pi_state == ePI_BOOTING) {
        Serial.println("RPi is booting");
      }
    }
    else {
      if ( dcl_trigger == false) {
        if ( wakeTimeEpoch <= timeNow ) {
          SleepyPi.enablePiPower(true);
          //SleepyPi.enableExtPower(true);
          delay(22000);
          ePISTATE  pi_state = ePI_BOOTING;
          Serial.println("Turning on Pi Power");
        }
      }
      else {
      Serial.println("dcl interrupt triggered, dcl not on, doing nothing");
      delay(100);
      }
    }
  }
  else { Serial.println("RPi is on");}
  
  // Blink the LED 
  digitalWrite(LED_PIN,HIGH);   // Switch on LED
  // Print the time
  printTimeNow();   
  delay(50);
  digitalWrite(LED_PIN,LOW);    // Switch off LED 
  timeOutStart = millis();
  
  // set delay timers
  delay(2000);
  elapsedTimeMS = timeOutStart;
  if (digitalRead(DCL_PIN) == 1) {
    timeOutEnd = timeOutStart + (DCLtimeAwakeSec * 1000);
  }
  else {
    timeOutEnd = timeOutStart + (timeAwakeSec * 1000);
  }
  wakeTime = CalcNextWakeTime();
  
  // not sure if this works ignoring for now pi_state doesn't seem to register
  if (pi_state == ePI_BOOTING) {
    delay(15000);
    pi_running = SleepyPi.checkPiStatus(false);
    if (pi_running == true) {
     ePISTATE  pi_state = ePI_ON;
    }
  } 
  delay(2000);
  pi_running = SleepyPi.checkPiStatus(false); // check pin 25 (pi) for 3.3V

  // If pi is able to boot in time check pin 25 on pi for 3.3V (pi on) 
  while ((pi_running == true) && (elapsedTimeMS < timeOutEnd)) {
    elapsedTimeMS = millis();
    delay(1000);
    Serial.print("Pi currrent is:");
    Serial.println(SleepyPi.rpiCurrent()); 
    supplyVoltage = SleepyPi.supplyVoltage();
    Serial.print("Battery Supply Voltage is :");
    Serial.println(supplyVoltage);
    printTimeNow();
    timerSec = (timeOutEnd - elapsedTimeMS) / 1000;
    timerMin = timerSec / 60;
    Serial.print(" You have ");
    Serial.print(timerMin);
    Serial.print(" Minutes to shutdown");
    Serial.println();
    Serial.println();
    digitalWrite(LED_PIN,HIGH);   // Switch on LED
    delay(100);
    digitalWrite(LED_PIN,LOW);    // Switch off LED 
    if(supplyVoltage > cutOffVoltage) {
          timeVoltageLow = 0;
    }
    else {
      timeVoltageLow = elapsedTimeMS-timeOutStart;
      Serial.print("voltage low for ");
      Serial.print(timeVoltageLow);
      Serial.println(" MS");
    }
  
    // Check if DCL is on or off
    int dclState = digitalRead(DCL_PIN);
    Serial.println(dclState);
    switch (dclState) {
        case 1:
            digitalWrite(LIGHT_PIN, LOW);
            incomingConfigRead();
            wakeTime = CalcNextWakeTime();
            if (timeVoltageLow > LOW_VOLTAGE_TIME_MS) {
              timerSec = timeVoltageLow/1000;
              timerMin = timerSec / 60;
              timeLeft = 5.00 - timerMin ;
              Serial.print("Voltage is low at:");
              Serial.println(supplyVoltage);
              Serial.print("DCL power overide you have ");
              Serial.print(timeLeft);
              Serial.println(" minutes to shutdown...maybe");
            }
            else if (timeVoltageLow > OVERRIDE_TIME_MS) {
              Serial.println("Override time expired shutting down now!");
              delay(8000);
              SleepyPi.piShutdown();
              delay(200);
              SleepyPi.enablePiPower(false);
              SleepyPi.enableExtPower(false);
            }
           break;
        case 0:
            if (timeVoltageLow > LOW_VOLTAGE_TIME_MS) {
              SleepyPi.piShutdown();
              delay(200);
              SleepyPi.enablePiPower(false);
              SleepyPi.enableExtPower(false);
            }
            light = (lightPower/100)*255;
            analogWrite(LIGHT_PIN, light);
            wakeTime = CalcNextWakeTime();
            if (Serial.available() > 4)  {
              Serial.println("Photos!");
              Serial.read();
              while (i < iterations) {
                delay(3000);
                if (Serial.available() > 2) {
                  Serial.read();
                  i=iterations;
                }
                else  {
                  Serial.println("Photos!");
                  delay(3000);
                  i++;
                }
              }
            }
            break;
        }    

   if (wakeTimeMin > 255)  {// timer1 is limited to 255 max
      if (wakeDay > 255) {
        wakeDay = 255;
        Serial.println("Start time is more than 255 days into the future, will start 255 days from now and reset alarm");      
      }
      else {
      Serial.println("Start time is more than 255 hours from now will use RTC alarm to wake");
      }
    }
    else {
      Serial.print("Setting timer1 to wake in ");
      Serial.print(wakeTime);
      Serial.println(" minutes"); 
   }
    
   pi_running = SleepyPi.checkPiStatus(false);
  
 }
  

 
 // Start a shutdown
 if(pi_running == true) { // check GPIO 25 for pi status
      SleepyPi.piShutdown(); // send GPIO 24 to 3.3V to signal shutdown
      delay(500);
      SleepyPi.enablePiPower(false);  // pull pi power
      SleepyPi.enableExtPower(false); // pull all power
      
  }
    
 delay(100);
 // Enter Power down state with ADC off and BOD off 
 SleepyPi.enablePiPower(false);
 SleepyPi.enableExtPower(false);
 delay(200);
 
 // Allow DCL to initiate wake
 PCintPort::attachInterrupt(DCL_PIN,dcl_isr,CHANGE);    // DCL pin 
   
 // Allow wake up alarm to trigger interrupt on falling edge
 attachInterrupt(0, alarm_isr, FALLING);  //RTC Alarm pin
 SleepyPi.enableWakeupAlarm(true);
 SleepyPi.ackTimer1(); // wake on timer1 
 SleepyPi.ackAlarm();  // wake on RTC alarm
 if (wakeTimeMin > 255)  {// timer1 is limited to 255 max
    if (wakeDay > 255) {
      wakeDay = 255;
      SleepyPi.setAlarm(wakeDay, wakeHour, wakeMin);
      Serial.println("Start time is more than 255 days into the future, will start 255 days from now and reset alarm");      
    }
    else {
      SleepyPi.setAlarm(wakeDay, wakeHour, wakeMin);
      Serial.println("Start time is more than 255 hours from now will use RTC alarm to wake");
    }
 }
 else {
   SleepyPi.setTimer1(PeriodicTimer_Timebase, wakeTime);
    Serial.print("Setting timer1 to wake in ");
    Serial.print(wakeTime);
    Serial.println(" minutes");
 }
 SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);


// Disable external pin interrupt on WAKEUP_PIN
 PCintPort::detachInterrupt(DCL_PIN);
 detachInterrupt(0);

}

// ************************ END OF EXECUTE LOOP**************************
