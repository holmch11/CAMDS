#!/bin/bash
# This shell program is the primary autonomous underwater camera 
# 	controll program and is run automatically at start up.
# There are two primary states in this program DCL on and autonomous 
#	camera mode.  The DCL port power is brought in at 12v and reduced
# 	to 3.3V applied at pin 19.  This is the primary signal used for 
#	determining state.
#	This program can also be run with -v verbose mode for testing,  
#	-h for help, and -V to get program version.
#  
# Version History
#**********************************************************************#
# 2021-07-06 CEH Initial Version Chris Holm 	holmch@oregonstate.edu	
# 2021/07/12 CEH Removed shutdown from camds and put into start.sh
# 2021/07/19 CEH Removed all Sleepy set-up to simplify code
# 2021/08/20 CEH Added Rsync of jpg data to photo routine to make dcl schedule unnecessary
# 2021/08/31 CEH Forked Simplified version Always Rsync, Always take photos no sleepy config
# 2021/09/03 CEH Changed rsync directory structure and added python serial logging of sleepy pi
# 2021/09/04 CEH Added config check on non-dcl photo routine to fix default parameter bug
# 2021/11/17 CEH Cleaned up remaining holdovers from non-simplified version 
# 2022/06/10 CEH Changed Photo Aquistition to raspicam
#
#**********************************************************************#

# Constants
version="2.0.0"			# Software Version 
serial_number="0112"		# Camera Serial number
creation_date="2022/06/15"	# Date Software version created
dclpin=19					# Enable wake pin of DCL
sleepyserial="/dev/ttyS0"	# Serial port for sleepy pi

# Variables
pinstate=0			    # DCL pin state intially set to off 0
sampletime=`date -u +%H:%M`	# Place holder for sampletime from config default is today
startdate=`date -u +%Y-%m-%d`	# Place holder for startdate from config default is today
filedate=`date -u +%Y%m%d`	# get todays date for appending to file names
filetime=`date -u +%H%M%S`	# get time of scipt running for file names
sampleTimeMin=0			# sample time converted to int min
startDateEpoch=0		# startdate converted to seconds since epoch
sampleinterval=240		# 4 hour sample interval default
lightpower=60			# lights set to 50%
lightdelay=180			# seconds to keep lights on
dclOnDelay=3600			# seconds to keep pi on with DCL
piOnDelay=600			# seconds to keep pi on without DCL
burst=3				# number of photos to take at sampling
interval=2			# time between photos in burst (sec)
shutter=0			# shutter speed 0 is auto
timeout=3		    # photo timout 3 seconds (sec)
t=3000			    # converted timeout value to msec
s=0					# shutterspeed in microseconds 
check=0				# check for grep stder 0 found, 1 not found
imgquality=60		# sets percentage of resized image quality
imgresize=50		# sets percentage of resized image size
gain=0				# 0 auto,sets sensor gain (analog first then digital) 
shutter=0			# sets shutter speed in milliseconds 0 auto max 200s

# File Paths
config="/home/camds/config/camds.cfg"						# location of config file
raw="/media/camds/EACAM$serial_number/raw/"					# location of raw image	
jpg="/media/camds/EACAM$serial_number/jpg/"					# where to store compressed photos (jpg)
configdir="/home/camds/config/"								# Config directory
RemoteDataDir="root@192.168.0.137:/data/cg_data/camds/" 	# where to put jpgs
RemoteConfigDir="root@192.168.0.137:/root/current/Cfg/InstCfg/camds.cfg" 	# where to look for camds.cfg
tempfile=$(mktemp) 											# location of minicom expect scripts
conlogfile="/media/camds/EACAM$serial_number/logs/config_$filedate$filetime.txt" 	# sleepyPi configuraton logs
camlogdir="/media/camds/EACAM$serial_number/logs/"				# Overall script logs
logdir="/home/camds/logs/"


if [ ! -d "/sys/class/gpio/gpio$dclpin"  ]
then
	sudo echo "$dclpin" > /sys/class/gpio/export
	sleep 1
	echo "in" > /sys/class/gpio/gpio$dclpin/direction
else
	echo "in" > /sys/class/gpio/gpio$dclpin/direction
fi

# Function to read variables from config file at path config
config() {
	while IFS="=" read a b; do
		case $a in
			sampletime)
				IFS=" " read c d <<< "$b"
				sampletime="${c//[[:blank:]]/}"
				;;
			startdate)
				IFS=" " read c d <<< "$b"
				startdate="${c//[[:blank:]]/}"
				;;
			lightpower)
				IFS=" " read c d <<< "$b"
				lightpower="${c//[[:blank:]]/}"
				;;
			sampleinterval)
				IFS=" " read c d <<< "$b"
				sampleinterval="${c//[[:blank:]]/}"
		 		;;
			lightdelay)
				IFS=" " read c d  <<< "$b"
				lightdelay="${c//[[:blank:]]/}"
				;;
			dclOnDelay)
				IFS=" " read c d <<< "$b"
				dclOnDelay="${c//[[:blank:]]/}"
				;;
			piOnDelay)
				IFS=" " read c d <<< "$b"
				piOnDelay="${c//[[:blank:]]/}"
				;;
			burst)
				IFS=" " read c d <<< "$b"
				burst="${c//[[:blank:]]/}"
				;;
			interval)
				IFS=" " read c d <<< "$b"
				interval="${c//[[:blank:]]/}"
				;;
			gain)
				IFS=" " read c d <<< "$b"
				gain="${c//[[:blank:]]/}"
				;;
			shutter)
				IFS=" " read c d <<< "$b"
				shutter="${c//[[:blank:]]/}"
				;;
			imageresize)
				IFS=" " read c d <<< "$b"
				imgresize="${c//[[:blank:]]/}"
				;;
			imagequality)
				IFS=" " read c d <<< "$b"
				imgquality="${c//[[:blank:]]/}"
				;;
		esac
	done <"$config"
	IFS=":" read c d <<< "$sampletime"
	sampleTimeMin=$(($c * 60 + $d))
	startDateEpoch=$(date -u -d "$startdate" +"%s")
}

#	Function to determine wake state asycnronous (dcl) or sycnronous 
stateInterpreter()	{
	pinstate=$(sudo	cat	/sys/class/gpio/gpio$dclpin/value)
	if	(($pinstate == 1)); 	then
		rsync -rtP -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteConfigDir $configdir
	fi
	config
	sleep 1
	timedatectl status
	python3 /home/camds/bin/sleepycom.py $sleepyserial
	sleep 1
	python3 /home/camds/bin/lights_lasers_on.py $lightpower $lightdelay &
	PID=$!
	photos
	sleep 1
	sudo kill $PID
	python3 /home/camds/bin/lights_lasers_off.py
	compress
	sleep 1
	sudo rsync -rtP $jpg -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
	sleep 1
}

#	Function that adds on printed info to config when in verbose mode
verboseConfig() {
	echo "Sample Time set to $sampletime!"
	echo "Sample Time set to $sampleTimeMin!"
	echo "start date set to $startdate!"
	echo "start date set to $startDateEpoch!"
	echo "Light Power set to $lightpower!"
	echo "Sample Interval set to $sampleinterval!"
	echo "Light Delay set to $lightdelay!"
	echo "dcl on delay set to $dclOnDelay!"
	echo "pi on delay set to $piOnDelay!"
	echo "burst set to take $burst photos"
	echo "interval set to $interval between photos"
	echo "gain set to $gain!"
	echo "shutter speed set to $shutter!"
	echo "image resize set to $imgresize!"
	echo "image quality set to $imgquality!"
	sleep 1
}

#	Function that replaces the state interpreter when in verbose mode
verboseStateInterpreter()	{
	echo "begin State Interpreter"
	pinstate=$(sudo	cat /sys/class/gpio/gpio$dclpin/value)
	echo "The DCL GPIO pin is $pinstate"
	if	(($pinstate == 1)); 	then
		echo "DCL on starting Config Rsysnc"
		rsync -rtP -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteConfigDir $configdir
		echo "rsync done getting new configuration"
	else
		echo "DCL is OFF Taking photos"
	fi
	config
	verboseConfig
	timedatectl status
	echo "Starting pyserial read of sleepy pi"
	sleep 2
	python3  /home/camds/bin/sleepycom.py $sleepyserial
	sleep 1
	echo "Turning on Lights at $lightpower%"
	echo "Turning on Lasers"
	echo "Set light delay time to $lightdelay seconds"
	python3 /home/camds/bin/lights_lasers_on.py $lightpower $lightdelay &
	PID=$!
	echo "Taking photos now"
	photos
	sleep 2
	sudo kill $PID
	python3 /home/camds/bin/lights_lasers_off.py
	echo "Photos Done"
	echo "Resizing raw images for DCL Rsync"
	compress	
	sleep 1
	echo "image compression done, starting image rsync"
	sudo rsync -rtP $jpg -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
	echo "Image Rsync Done"
	sleep 1
	echo "shutting down now...."	
}

# Photo Taking Routine with Raspicam
photos() {
	let timeout="($interval*$burst)-$interval+1"
	let t="$timeout" 
	let tl="$interval"
	if (($shutter == 0)); then
		if (($gain == 0 )); then
		echo "using mode full auto"
		raspistill -q 100 -t $t -tl $tl -v -r -n -o  $raw`date +%Y%m%dT%H%M%SZ`%04d.jpg --exif IFD0.Artist=OOIEA-$serial_number --exif IFDO.Software=$version,$creation_date --exif EXIF.Fnumber=1.8 --exif EXIF.FocalLength=2.8 --exif EXIF.MaxApertureValue=1.8
		else
		echo "Floating Shutter fixed gain set to $gain"
		raspistill -q 100 -t $t -tl $tl -v -r -n -ag $gain -o $raw`date +%Y%m%dT%H%M%SZ`%04d.jpg --exif IFD0.Artist=OOIEA-$serial_number --exif IFD0.Software=$version,$creation_date --exif EXIF.Fnumber=1.8 --exif EXIF.FocalLength=2.8 --exif EXIF.MaxApertureValue=1.8
		fi
	else
	let s="$shutter*1000"
	echo "auto mode off using manual shutter set to $s microseconds"
		if (($gain == 0)); then
		echo "Gain is set to 0 and will be set by the camera"
		raspistill -q 100 -t $t -tl $tl -v -r -n -ss $s -o  $raw`date +%Y%m%dT%H%M%SZ`%04d.jpg --exif IFD0.Artist=OOIEA-$serial_number --exif IFD0.Software=$version,$creation_date --exif EXIF.Fnumber=1.8 --exif EXIF.FocalLength=2.8 --exif EXIF.MaxApertureValue=1.8
		else
		echo "fixed shutter set to $shuttter and fixed gain set to $gain"
		raspistill -q 100 -t $t -tl $tl -v -r -n -ss $s -ag $gain -o $raw`date +%Y%m%dT%H%M%SZ`%04d.jpg --exif IFD0.Artist=OOIEA-$serial_number --exif IFD0.Software=$version,$creation_date --exif EXIF.Fnumber=1.8 --exif EXIF.FocalLength=2.8 --exif EXIF.MaxApertureValue=1.8
		fi
	fi
}

# Photo transformation with ImageMagick
compress() {
	diff -r $raw $jpg | grep $raw | awk '{print$4}' > $tempfile
	trap "rm -f $tempfile" 0 2 3 15
	sleep 1
	sudo sed -i '/and/d' $tempfile
	while IFS= read -r line; do 
	convert -interlace Plane -gaussian-blur 0.05 -quality $imgquality% -adaptive-resize $imgresize% $raw$line $jpg$line
	echo "$raw$line has been compressed to $imgquality% quality and $imgresize% size and saved as $jpg$line"
	done < $tempfile	
}

#	Help Function only run with -h options
help() {
	cat << EOL
	This script runs at start up and controls the overall camds system.
		Most common error would be with filepaths open code in a text 
		editor and edit file paths to match the system.
	
	Syntax: camds.sh [-v|h|V]
	Options:
	-v		Verbose mode
	-h		Print this Help Menu
	-V		Print software version and exit
	-S		Print Camera Serial number and Software version and exit
EOL
}

version() {
	echo " camds Version $version created $creation_date"
}
serial() {
		echo "camds SN $serial_number running camds Version $version created $creation_date"
}

# Main Program altered by options
if (! getopts "vVhS" arg); then
	stateInterpreter
else
	while getopts "vVhS" arg; do
		case $arg in
			v)	verboseStateInterpreter
				echo "ran verboseStateInterpreter"
				echo "All done!"
				;;
			V)	version
				;;
			h)	help
				;;
			S)	serial
				;;
		esac
	done
fi
##
##
