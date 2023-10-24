############################################################
# Control of Pi HQ camera
# This captures images from Pi HQ camera and converts them to numpy arrays for storage
# It allows a burst of fixed images set apart by interval
# At run it should include arguments save file location, burst, interval, and shutter speed
# Syntax is: python3 home/camds/bin/capture_rgb.py /media/camds/drivename/npy/ 3 2 0
# Version 0.0.0 created 07/08/2021
# By Ian Black
# Added to camera control by Chris Holm
#  christopher.holm@oregonstate.edu
#################################################################
# Revision History
# 2021-07-08 IB Initial Version
# 2021-07-08 CEH Added comments and removed v2 from file name
# 2021-07-08 CEH removed str() on arg 1 changed shutterspeed to shutter_speed filename to filpath ln53
# 2021-07-08 CEH moved rgb_filepaths.append(filepath) from ln 60 to line 55
# 2021-07-10 CEH Initial working Version 
##################################################################
#!/usr/bin/python3
# -*- coding:utf-8 -*-

# External Module imports
from datetime import datetime,timezone
import numpy as np
import os
import picamera
import time
import sys

save_location = sys.argv[1]   #Save location of data.
burst = int(sys.argv[2])       #Number of images to capture.
interval = float(sys.argv[3]) #Number of seconds between images.
shutter = int(sys.argv[4])    #Value of 0 is auto adjust. Otherwise value in ms.

#Setup the camera.
camera = picamera.PiCamera()
width = int(np.floor(4056/32)*32) #Compute ND compatible width.
height = int(np.floor(3040/16)*16) #Compute ND compatible height.
camera.resolution = (width,height)  #Set camera resolution.
camera.shutter_speed = shutter 
time.sleep(3) #Wait for the camera to adjust.

#Capture images.
rgb_filepaths = []
for image in range(burst):  #For each capture in the burst.
    start = time.monotonic()  #Get the loop start time.
    img = np.empty((width*height*3,),dtype=np.uint8)  #Create a holder array.
    camera.capture(img,'rgb')  #Capture the image as RGB values to the holder.
    dt = datetime.now(timezone.utc) #Get the datetime.
    rgb = img.reshape((height,width,3))
    filename = dt.strftime('%Y%m%dT%H%M%S_%f')[:-3] + 'Z' + '.npy' 
    filepath = os.path.join(save_location,filename) #Absolute path location.
    np.save(filepath,rgb,allow_pickle = False)  #Save as .npy.
    end = time.monotonic()
    rgb_filepaths.append(filepath)
    if interval - (end-start <= 0): #Only really matters <2 sec intervals.
        continue
    else:
        time.sleep(interval - (end - start)) #Fine tune loop time.
camera.close()  #Exit the camera.

print(*rgb_filepaths, sep = "\n")

#Save filepath names to a text file.
recent_file = os.path.join(save_location,"filepaths.txt")
with open(recent_file,'w') as f:
    for filepath in rgb_filepaths:
        f.write(filepath)
        f.write("\n")