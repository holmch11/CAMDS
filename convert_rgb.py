############################################################
# Handling of Pi HQ camera files
# This takes numpy array files created by capture_rgb.py and converts them to
# a set of png images (full resolution) and jpg images (compressed) 
# the compressed files are rsync'ed to the DCL
# The program is run with 3 arguments: where to find npy and where to save png and jpg 
# Syntax is: python3 home/camds/bin/convert_rgb.py /media/camds/drivename/npy/ /media/camds/drivename/png/ /media/camds/drivename/jpg/
# Version 0.0.0 created 07/08/2021
# By Ian Black
# Added to camera control by Chris Holm
#  christopher.holm@oregonstate.edu
#################################################################
# Revision History
# 2021-07-08 IB Initial Version
# 2021-07-08 CEH Added comments and removed v2 from file name, added arguments 2 and 3
# 2021-07-10 CEH Initial working version
#
##################################################################
#!/usr/bin/python3
# -*- coding:utf-8 -*-

# External Module imports
import numpy as np
import os
from PIL import Image
import re
import sys

raw_location = sys.argv[1]  
png_location = sys.argv[2]
jpg_location = sys.argv[3]

recent_file = os.path.join(raw_location,"filepaths.txt")
with open(recent_file,'r') as f:  #Read in filepaths.txt.
    lines = f.readlines()

#Create full size PNGs.
png_filepaths = []
for filepath in lines:
    path = filepath.rstrip()
    rgb = np.load(path)
    img = Image.fromarray(rgb)
    pattern = '(.*?Z).npy'
    file = os.path.basename(path)
    dt = re.findall(pattern,file)[0]
    png_filepath = os.path.join(png_location,dt+'.png')
    img.save(png_filepath)
    png_filepaths.append(png_filepath)

#Create 1/4 size jpgs.
jpg_filepaths=[]
for filepath in png_filepaths:
    with Image.open(filepath) as img:
        dt = re.findall('(.*?Z).png',os.path.basename(filepath))[0]
        resized = img.resize((1008,760))
        resized_filepath = os.path.join(jpg_location,dt+'.jpg')
        resized.save(resized_filepath)
    jpg_filepaths.append(resized_filepath)            