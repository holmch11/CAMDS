############################################################
# Serial Communications with Sleepy Pi 
# This handles all serial communications between the raspberry pi and the sleepy pi
# 
# At run it should include the serial port and logfile path as arguments 
# Syntax is: python3 /home/camds/bin/sleepycom.py /dev/ttyS0 /home/camds/logs/sleepy.txt
# Version 1.00 created 09/2/2021
# By Christopher Holm
#  christopher.holm@oregonstate.edu
#################################################################
# Revision History
# 2021-09-02 CEH Initial Version
# 2022-06-15 CEH Better Return if no data available
#
##################################################################
#!/usr/bin/python3
# -*- coding:utf-8 -*-

# External Module imports
import serial
import sys

# Define Variables
ser_port = sys.argv[1] 	# sleepy pi serial address
baud = 115200		# baudrate is set by sleepy pi at 115200 
t=5            # Serial timeout for each line


with serial.Serial(ser_port, baud, timeout=t) as ser:
    log1 = ser.readline()
    dlog1 = (log1[0:len(log1)-2].decode("utf-8"))
    print(dlog1)
    log2 = ser.readline()
    dlog2 = (log2[0:len(log2)-2].decode("utf-8"))
    print(dlog2)
    log3 = ser.readline()
    dlog3 = (log3[0:len(log3)-2].decode("utf-8"))
    print(dlog3)
    log4 = ser.readline()
    dlog4 = (log4[0:len(log4)-2].decode("utf-8"))
    print(dlog4)
    log5 = ser.readline()
    dlog5 = (log5[0:len(log5)-2].decode("utf-8"))
    print(dlog5)
    log6 = ser.readline()
    dlog6 = (log6[0:len(log6)-2].decode("utf-8"))
    log7 = ser.readline()
    dlog7 = (log7[0:len(log7)-2].decode("utf-8"))
    print(dlog7)
    log8 = ser.readline()
    dlog8 = (log8[0:len(log8)-2].decode("utf-8"))
    print(dlog8)
    log9 = ser.readline()
    dlog9 = (log9[0:len(log9)-2].decode("utf-8"))
    print(dlog9)
    ser.flushInput()


