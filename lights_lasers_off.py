############################################################
# Control of CAMDS LIGHTS and LASERS
# Lights and Lasers are supplied with 24V directly from the battery via power bus
# Power is controlled with waveshare RPi Relay Board
# https://www.waveshare.com/wiki/RPi_Relay_Board
# Light intensity is controlled with 0-5V signal on Lamp pin 3
# This is accomplished via pwm control of pin 18 sent through a dc to dc converter
# 
# Relay control is as follows P26 CH1 RELAY, P20 CH2 RELAY, P21 CH3 RELAY
# This program is dependent on the following packages
# bcm 2835 http://www.airspayce.com/mikem/bcm2835/
# wiringpi, pillow, numpy libopenjp2-7, libtiff5 libatlas-base-dev
# RPI.GPIO, smbus, time
# Version 1.00 created 06/11/2021
# By Chris Holm
#  christopher.holm@oregonstate.edu
#################################################################
# Revision History
# 2021-06-11 CEH Initial Version
#
#
##################################################################
#!/usr/bin/python3
# -*- coding:utf-8 -*-

# External Module imports
import time
import RPi.GPIO as GPIO
import sys

# Pin Definitions:
pwmPin = 18    # Broadcom pin 18 (P1 pin 12)
ch1pin = 26  # Broadcom pin 26
ch2pin = 20  # Broadcom pin 20
ch3pin = 21  # Broadcom pin 21

dc = 100                      # duty cycle (0-100) for PWM pin

# Pin Setup:
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)         # Broadcom pin-numbering scheme 
GPIO.setup(pwmPin, GPIO.OUT)   # PWM pin set as output
pwm = GPIO.PWM(pwmPin,100)     # Initialize PWM on pwmPin 100Hz freq
GPIO.setup(ch1pin,GPIO.OUT) 
GPIO.setup(ch2pin,GPIO.OUT)
GPIO.setup(ch3pin,GPIO.OUT)
pwm.start(100-dc)
#initial state for pins:

try:
    pwm.stop()
    GPIO.output(ch1pin,GPIO.HIGH)
    GPIO.output(ch2pin,GPIO.HIGH)
    GPIO.output(ch3pin,GPIO.HIGH)
    GPIO.cleanup()
except:
    pwm.stop()
    GPIO.output(ch1pin,GPIO.HIGH)
    GPIO.output(ch2pin,GPIO.HIGH)
    GPIO.output(ch3pin,GPIO.HIGH)
    GPIO.cleanup()
#  
