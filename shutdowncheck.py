###################################################################################
# Executes graceful shutdown when signaled by Sleepy Pi
# https://spellfoundry.com/docs/getting-the-sleepy-pi-to-shutdown-the-raspberry-pi
# Hardware required is https://spellfoundry.com/product/sleepy-pi-2-usb-c
# Version 1.00
# Created by Christopher Holm
#   christopher.holm@oregonstate.edu
#
# Revision History
#####################################################################################
# 2021/16/07 CEH Initial Version
#
####################################################################################
#!/usr/bin/python

import RPi.GPIO as GPIO
import os, time

GPIO.setmode(GPIO.BCM)
GPIO.setup(24, GPIO.IN)
GPIO.setup(25, GPIO.OUT)
GPIO.output(25, GPIO.HIGH)
print ("[info] Telling Sleepy Pi we are running pin 25")

while True:
    if (GPIO.input(24)):
        print ("Sleepy Pi requesting shutdown on pin 24")
        os.system("sudo shutdown -h now")
        break
    time.sleep(0.5)
    
