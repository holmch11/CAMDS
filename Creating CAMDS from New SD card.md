# README for setting up CAMDS from brand new SD card #

Install Raspian Raspbery Pi OS 32 Bit

***sudo su root***

*** adduser camds***

logout of pi and into camds

***killall -u pi***

***deluser pi***

remove all folders from home directory

***sudo usermod -aG sudo camds***

***sudo usermod -aG gpio camds***

***sudo usermod -aG netdev camds***

add directories: *apps bin config logs and sketchbook*

***nano .bashrc***   and add ll and la alias

***sudo visudo*** and add at bottom "camds  ALL=(ALL:ALL) NOPASSWD:ALL"
 
***sudo apt update***

***sudo apt upgrade***

***sudo apt-get install arduino***

***sudo apt-get install wiringpi****

***cd***

***wget https://project-downloads.drogon.net/wiringpi-latest.deb***

***sudo dpkg -i wiringpi-latest.deb***

***gpio -v*** should come back with 2.52 as version

***cd***

***wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.60.tar.gz***

***tar zxvf bcm2835-1.60.tar.gz***

***mv bcm2835-1.60.tar.gz ~/apps/***

***mv wiringpi-latest.deb ~/apps/***

***cd bcm2835-1.60/***

***sudo ./configure***

***sudo make && sudo make check && sudo make install***

***sudo pip3 install pillow***

***sudo pip3 install numpy***

***sudo apt-get install libopenjp2-7***

***sudo apt install libtiff***

***sudo apt install libtiff5***

***sudo apt-get install libatlas-base-dev***

***cd***

***sudo apt-get update***

***sudo apt-get install python-pip***

***sudo pip install RPi.GPIO***

***sudo pip install smbus***

***cd***

***sudo apt-get update***

***sudo apt-get install python3-pip***

***sudo pip3 install RPi.GPIO***

***sudo pip3 install smbus***

***sudo systemctl stop serial-getty@ttyS0.service***
 
***sudo systemctl disable serial-getty@ttyS0.service***

***nano /boot/cmdline.txt*** and delete console=serial0,115200

***cd apps*** 

***wget https://github.com/SpellFoundry/avrdude-rpi/archive/master.zip***

***sudo unzip master.zip***

***cd avrdude-rpi-master***

***sudo cp autoreset /usr/bin***

***sudo cp avrdude-autoreset /usr/bin***

***sudo mv /usr/bin/avrdude /usr/bin/avrdude-original***

***sudo ln -s /usr/bin/avrdude-autoreset /usr/bin/avrdude***

***sudo raspi-config nonint do_camera 0***

***sudo raspi-config nonint do_overscan 1***

***sudo raspi-config nonint do_ssh 0***

***sudo raspi-config nonint do_vnc 0***

***sudo raspi-config nonint do_spi 1***

***sudo raspi-config nonint do_i2c 0***

***sudo raspi-config nonint do_serial 1***

***sudo raspi-config nonint do_leds 1***

***sudo raspi-config nonint do_fan 0***

***sudo raspi-config nonint do_rgpio 1***

***sudo raspi-config nonint do_onewire 1***

***sudo raspi-config nonint do_audio 1***

***sudo raspi-config nonint set_config_var gpu_mem 512 /boot/config.txt***

***sudo apt-get install minicom***

***sudo usermod -a -G video camds***

make sure to add the file shutdowncheck.py to home/camds/bin

add sleepy.service, camds.service, camdslast.target to /etc/systemd/system

***sudo systemctl start sleepy.service***

***sudo systemctl enable sleepy.service***

***sudo systemctl daemon-reload***

***sudo nano /boot/config.txt*** 
verify the line dtparam=i2c_arm=on is NOT commented out and add line at the very bottom "dtoverlay=i2c-rtc,pcf8523"

***sudo apt-get install i2c-tools***

***sudo reboot***

***sudo i2cdetect -y 1*** 
should come up with grid that has 24 and 68  populated possible position 68 will have UU that is ok

***sudo nano /lib/udev/hwclock-set***

comment out 

if [ -e /run/systemd/system ] ; then		

		exit 0

			  fi

also comment out the /sbin/hwclock --rtc=$dev --systz --badyear


and /sbin/hwclock -rtc=$dev --systz --bad year from else
 
#read clock# 

***sudo hwclock -r***

#set clock#

***sudo hwclock -w***


#set Rpi to RTC#

***sudo hwclock -s***

#set RTC directly# 

***sudo hwclock --set --date="2021-08-14 16:45:05"***

***sudo systemctl stop ntp.service***

***sudo systemctl disable ntp.service***

#if there are issues verify RTC connected#
***dmesg | grep rtc***

# All Clock Status #

*** sudo timedatectl status***

# find uuid of drive # 

***sudo lsblk -o UUID,NAME,FSTYPE,SIZE,MOUNTPOINT,LABEL,MODEL***

***sudo apt update***

***sudo apt install exfat-fuse***

***sudo blkid***

***sudo mkdir /media/camds/***EACAM$cameraSerialNumber/

***sudo mount drive location /media/camds/***EACAM$cameraSerialNumber/

***sudo nano /etc/fstab*** and add
UUID="camera UID" /mnt location exfat defaults,auto,users,rw,nofail 0 0

***sudo enable camds.service***

***sudo shutdown now***