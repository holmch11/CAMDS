    /*
   This controls the arduino based sleepy pi 2 usb-c from spell foundry                                                                                                              
 http://sleepypi.com/
 
 The sleepy pi controls power on and shutdown-poweroff of the Raspberry Pi
 as well as puts itself into a low power sleep mode between wake 
 cycles.  This is accomplished via synchronous (timed) wake cycles programed
 via a config file on the Raspberry pi, as well as an asynchronous 3.3V pin signal 
 on pin 8 powered by the buoy data logger controller (DCL) via the 
 12V port power. 
 
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
 2021-07-02 CEH Initial version created by Christopher Holm holmch@oregonstate.edu
 2021-08-28 CEH Forked Version for simplifiled control code
 2021-08-31 CEH Further improved DCL signal processing
 2022-07-06 CEH Added Power Off as backup before Sleepy Pi Sleep
 *************************************************************************************/

//****INCLUDED LIBRARIES*********
#include <LowPower.h>
#include <PCF8523.h>
#include <PinChangeInt.h>
#include <TimeLib.h>
#include <Wire.h>
#include "SleepyPi2.h"
#include <avr/wdt.h>

//****** DEFINE CONSTANTS *******
#define NO_PORTC_PINCHANGES
#define DISABLE_PCINT_MULTI_SERVICE
#define NO_PIN_STATE
#define NO_PIN_NUMBER
#define KPI_CURRENT_THRESHOLD_MA 95    // Shutdown current threshold in mA

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
const int sampleInterval = 240;                          // 4 hours 
const int sampleTime = 31;                               // minutes from midnight
eTIMER_TIMEBASE  PeriodicTimer_Timebase    = eTB_MINUTE; // Timebase set to minutes could be eTB_SECOND, eTB_HOUR

//****** DEFINE VARIABLES ********
int dclState = 0;                      // dcl off = 0; dcl on = 1
int i = 0;
uint8_t wakeMin = 0;
uint8_t wakeTime = 0;
unsigned long wakeTimeMin = 27073440ul;     // sample time in minutes since 1970-01-01
unsigned long startDate = 1624406400ul;     // 2021-06-23 00:00 in seconds since 1970-01-01 00:00
unsigned long wakeTimeSec = 0ul;            // calculated wake time in seconds after now()
unsigned long wakeTimeEpoch = 1624406400ul; // startdate and sample time in seconds since 1970-01-01 00:00
unsigned long DCLtimeAwakeSec = 3600;       // time sec to allow Rpi to stay awake if it doesn't shut down itself
unsigned long timeAwakeSec = 360;           // time sec to allow RPi on before shutdown with no DCL
unsigned long timeOutStart;                 // timer start in milliseconds
unsigned long elapsedTimeMS;                // Elapsted time in milliseconds
unsigned long timeOutEnd;                   // calculated time out end in milliseconds
unsigned long timeNow = 1624406400ul;       // place holder for today in seconds since 1970-01-01
boolean alarmFired = false;                 // RTC Alarm has triggerd?
boolean pi_running = false;                 // Pin 25 High indicates Pi is running
boolean dcl_trigger = false;                // Did the DCL trigger this interrupt
boolean pi_powered = false;                 // Did the pi get powered up?
ePISTATE  pi_state = ePI_OFF;               // Not 100% sure this is needed, might be used by sleepyPi library
float supplyVoltage;                        // Sleepy measurement of battery voltage
float wakeDayFloat;
float wakeHourFloat;
float timerMin = 0;
float timerSec = 0;
//******** DEFINE FUNCTIONS ********

// Function to handle alarm interrupt isr's need to be limited so not much here
void alarm_isr() {
  Serial.println("Awake from Alarm interrupt");  
  alarmFired = true;
  dcl_trigger = false;
}

void dcl_isr() {
  Serial.println("Awake from DCL interrupt");
  dcl_trigger = true;
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
  wakeTimeEpoch = (startDate + (sampleTime * 60)); // calculate total wake time in seconds since 1970 
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
    Serial.print("Next sample is less than 5 minutes from now adding ");
    Serial.print(sampleInterval);
    Serial.println(" minutes to the next wake time");
  }
  else { wakeTimeMin = wakeTimeSec / 60;}
  
  Serial.print("Camera will wake in ");
  Serial.print(wakeTimeMin);
  Serial.println(" minutes");
  
  if (wakeTimeMin > 255) {
    Serial.println("Error wake time exceeds 255 setting timer1 to 240");
    wakeTime = 240; 
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
   
  // Allow wake up alarm to trigger interrupt on falling edge
  attachInterrupt(0, alarm_isr, FALLING);  //RTC Alarm pin
  SleepyPi.enableWakeupAlarm(true);
  
  SleepyPi.ackAlarm();  // At this point we are waking
  
  // determine state of wake dcl interrupt or timer alarm or is it nonsense?
  pi_running = SleepyPi.checkPiStatus(false);
  if (pi_running == false) {
    delay(100);
    if ( dcl_trigger == true ) {
      delay(2000);
      if (digitalRead(DCL_PIN == 1)) {
        delay(3000);
        if (digitalRead(DCL_PIN == 1)) {
          SleepyPi.enablePiPower(true);
          ePISTATE  pi_state = ePI_BOOTING;
          pi_powered = true;
          Serial.println("DCL on Turning on Pi Power");
        }
      }
    }
    else {
      if ( dcl_trigger == false ) {
        if ( wakeTimeEpoch <= timeNow ) {
          SleepyPi.enablePiPower(true);
          ePISTATE  pi_state = ePI_BOOTING;
          pi_powered = true;
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
  
  // wait for pi to boot...
  if (pi_powered == true) {
    delay(25000);
    pi_running = SleepyPi.checkPiStatus(false);
    if (pi_running == true) {
     ePISTATE  pi_state = ePI_ON;
     Serial.println("pi state is on");
    }
    else {
      timeOutEnd = timeOutStart + 45000;
      while ((elapsedTimeMS < timeOutEnd) && (pi_running == false)) {
        elapsedTimeMS = millis();
        pi_running = SleepyPi.checkPiStatus(false);
        Serial.println("Waiting for Pi to boot, taking longer than normal?");
        delay(100);
      }
    }
  } 
  
   // set delay timers
  delay(1000);
  timeOutStart = millis();
  elapsedTimeMS = timeOutStart;
  if (digitalRead(DCL_PIN) == 1) {
    timeOutEnd = timeOutStart + (DCLtimeAwakeSec * 1000);
  }
  else {
    timeOutEnd = timeOutStart + (timeAwakeSec * 1000);
  }
 
  pi_running = SleepyPi.checkPiStatus(false); // check pin 25 (pi) for 3.3V

  // If pi is able to boot in time check pin 25 on pi for 3.3V (pi on) 
  while ((pi_running == true) && (elapsedTimeMS < timeOutEnd)) {
    elapsedTimeMS = millis();
    delay(800);
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
    digitalWrite(LED_PIN,HIGH);   // Switch on LED
    delay(100);
    digitalWrite(LED_PIN,LOW);    // Switch off LED 
    wakeTime = CalcNextWakeTime();
      
    Serial.print("Setting timer1 to wake in ");
    Serial.print(wakeTime);
    Serial.println(" minutes");

    
    // Check if DCL is on or off
    int dclState = digitalRead(DCL_PIN);
    Serial.println(dclState);
    Serial.println();
    Serial.println();
    
    pi_running = SleepyPi.checkPiStatus(false); 
    delay(100);
  } 
     
  SleepyPi.setTimer1(PeriodicTimer_Timebase, wakeTime);
  // Allow DCL to initiate wake
  PCintPort::attachInterrupt(DCL_PIN,dcl_isr,RISING);    // DCL pin 

  pi_running = SleepyPi.checkPiStatus(false);
  delay(200);
 
  // Start a shutdown
  if(pi_running == true) { // check GPIO 25 for pi status
      SleepyPi.piShutdown(); // send GPIO 24 to 3.3V to signal shutdown
      delay(2000);
      SleepyPi.enableExtPower(false); // pull power
  }
  else {
      SleepyPi.enablePiPower(false); // pull power
      SleepyPi.enableExtPower(false); // pull power
  }
  delay(200);
  pi_running = false;
  pi_powered = false;
  alarmFired = false;
  dcl_trigger = false;
  delay(50);
  SleepyPi.enablePiPower(false); //make sure power is pulled
  delay(50);
  SleepyPi.enableExtPower(false); //make sure power is pulled
  delay(100);
  // Enter Power down state with ADC off and BOD off 
  SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  // Disable external pin interrupt on WAKEUP_PIN
  PCintPort::detachInterrupt(DCL_PIN);
  detachInterrupt(0);
}
// ************************ END OF EXECUTE LOOP**************************
