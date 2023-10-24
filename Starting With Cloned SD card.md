# README for setting up CAMDS from Cloned SD card #
## This could become a bash script but for now just follow along..  

### Check function of RTC ###
***i2cdetect -y 1***

it should come up with grid that has 24 and 68  populated possible position 68 will have UU that is ok

### Then Check RTC Clock
  
**sudo hwclock -r**
  
If Clock Errors out try Setting both RPI and RTC time 

**sudo hwclock --utc --systohc**

Set RTC with RPi clock using 

**sudo hwclock -w**

set Rpi to RTC with 

**sudo hwclock -s**

Set Rpi time with 

**sudo timedatectl set-time 'YYYY-MM-DD HH:MM:SS'**

Set RTC directly with 

**sudo hwclock --set --date="2021-08-14 16:45:05"**

If there are issues verify RTC connected with 

**dmesg | grep rtc**

Final Check on all clocks

**sudo timedatectl status** 

########

## Next set Hard drive up ##

find uuid of drive with 

**sudo lsblk -o UUID,NAME,FSTYPE,SIZE,MOUNTPOINT,LABEL,MODEL**

or 

**sudo blkid**

make drive directory:

**sudo mkdir /media/camds/EACAM$cameraSerialNumber/**

Not strictly necessary but one could test by mounting drive

**sudo mnt (drive location) /media/camds/EACAM$cameraSerialNumber/**

add to drive permanently to: 

**sudo nano /etc/fstab**

by adding this line to fstab:
**UUID="camera UID" /mnt location exfat defaults,auto,users,rw,nofail 0 0**
 

Edit camds and start scripts for associated SN of camera

If they don't already exist add folders raw,npy,jpg,png,logs to mounted hard drive

## If trying to program sleepy not through power bypass but direct use ## 

**sudo i2cset -y 1 0x24 0xFF**  

Never leave it this way! This sets the software jumper on Sleepy to always on.

## Before running make sure ## 

**sudo i2cset -y 1 0x24 0xFD**

This sets software jumper to software control

## Make sure kill all clock handlers ##

**sudo systemctl stop fake-hwclock.service**

**sudo systemctl disable fake-hwclock.service**

**sudo timedatectl set-ntp false**

**sudo systemctl stop systemd-timesycnd.service**

**sudo systemctl disabel systemd-timesyncd.service**

Shut unit down and power up on a power supply and test connected to bench DCL for at least 48hrs before deploying