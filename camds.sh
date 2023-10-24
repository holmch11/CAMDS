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
# 1.0.0 Initial Version 2021-07-06	Chris Holm 	holmch@oregonstate.edu	
# 1.0.1	Removed shutdown from camds and put into start.sh 2021-07-12
# 1.1.0 Removed all Sleepy set-up to simplify code 2021-07-19 CEH
# 1.1.1 Added Rsync of jpg data to photo routine to make dcl schedule unnecessary 2021-08-20 CEH
#**********************************************************************#

# Constants
version="1.1.1"					# Software Version 
serial_number="0122"			# Camera Serial number
creation_date="2021/08/20"		# Date Software version created
dclpin=19						# Enable wake pin of DCL
sleepyserial="/dev/ttyS0"		# Serial port for sleepy pi

# Variables
pinstate=0						# DCL pin state intially set to off 0
sampletime=`date -u +%H:%M`		# Place holder for sampletime from config default is today
startdate=`date -u +%Y-%m-%d`	# Place holder for startdate from config default is today
filedate=`date -u +%Y%m%d`		# get todays date for appending to file names
filetime=`date -u +%H%M$S`		# get time of scipt running for file names
sampleTimeMin=0					# sample time converted to int min
startDateEpoch=0				# startdate converted to seconds since epoch
sampleinterval=240				# 4 hour sample interval default
lightpower=60					# lights set to 50%
cutoffvoltage=13				# voltage to stop sampling
lightdelay=180					# seconds to keep lights on
dclOnDelay=3600					# seconds to keep pi on with DCL
piOnDelay=600					# seconds to keep pi on without DCL
burst=3						# number of photos to take at sampling
interval=2					# time between photos in burst
shutter=0					# shutter speed 0 is auto
framerate=0					# framerate 
check=0						# check for grep stder 0 found, 1 not found

# File Paths
config="/home/camds/config/camds.cfg"								# location of config file
raw="/media/camds/EACAM$serial_number/npy/"							# location of raw numpy array of image	
png="/media/camds/EACAM$serial_number/png/"							# where to store pngs photos
jpg="/media/camds/EACAM$serial_number/jpg/"							# where to store jpg photos (reduced size)
configdir="/home/camds/config/"										# Config directory
RemoteDataDir="root@192.168.0.137:/data/cg_data/camds/" 		# where to put jpgs
RemoteConfigDir="root@192.168.0.137:/root/current/Cfg/InstCfg/dcl37/camds.cfg" 			# where to look for camds.cfg
temporary="/home/camds/logs/temp.txt" 								# location of minicom expect scripts
conlogfile="/media/camds/EACAM$serial_number/logs/sleepy_logs/config_$filedate$filetime.txt" 	# sleepyPi configuraton logs
camlogdir="/media/camds/EACAM$serial_number/logs/cam_logs/"			# Overall script logs
logdir="/home/camds/logs/"

# This is already done in start.sh so commented out here now
# Set the dcl pin for input
#sudo echo $dclpin	> 	/sys/class/gpio/export
#sudo echo "in"		> 	/sys/class/gpio/gpio$dclpin/direction

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
			cutoffvoltage)
				IFS=" " read c d <<< "$b"
				cutoffvoltage="${c//[[:blank:]]/}"
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
			shutter)
				IFS=" " read c d <<< "$b"
				shutter="${c//[[:blank:]]/}"
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
		sudo rsync -rtP $jpg -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
		config
		sleep 1
		#sleepyConfig 
		#check=$(grep -qw Photos! $conlogfile; echo $?)
		#if (($check == 0)); then 
		#	python3 /home/camds/bin/lights_lasers_on.py $lightpower $lightdelay &
		#	PID=$!
		#	python3 /home/camds/bin/capture_rgb.py $raw $burst $interval $shutter
		#	sleep 1
		#	sudo kill $PID
		#	python3 /home/camds/bin/lights_lasers_off.py
		#	python3 /home/camds/bin/convert_rgb.py $raw $png $jpg
		#	sleep 2
		#else
		#configCheck
		#sleep 1
		#fi
	else
		#minicom -D $sleepyserial -C $logfile
		python3 /home/camds/bin/lights_lasers_on.py $lightpower $lightdelay &
		PID=$!
		python3 /home/camds/bin/capture_rgb.py $raw $burst $interval $shutter
		sleep 1
		sudo kill $PID
		python3 /home/camds/bin/lights_lasers_off.py
		python3 /home/camds/bin/convert_rgb.py $raw $png $jpg
		sleep 1 
		#pkill minicom
		sleep 1
		sudo rsync -rtP $jpg -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
	fi
}

#	Function that creates minicom expect for sleepy pi config
sleepyConfig()	{
	cat >> $temporary << EOL
	
set a 0
timeout 180 goto error
goto iteration
loop:
	inc a
	expect {
		"sampletime!"  		goto sampletime
		"startdate!"   		goto startdate
		"interval!"			goto interval	
		"lightpower!"		goto lightpower		
		"cutvolts!"			goto cutvolts
		"dclOnDelay!"		goto dclOnDelay
		"piOnDelay!"		goto piOnDelay		
		"Photos!"  			goto photo
		"Alarm!"			goto alarm
		timeout 5 			goto iteration
}

iteration:
	send "Status!"	
	if a > 3 	goto error
	if a < 3 	goto loop
	timeout 2	goto error

sampletime:		
	send "$sampleTimeMin"	
	expect {
		"received:$sampleTimeMin"	goto confirm
		timeout 3					goto iteration
}

startdate:
	send "$startDateEpoch"
	expect {
		"received:$startDateEpoch"	goto confirm
		timeout 3					goto iteration
}

interval:
	send "$sampleinterval"
	expect {
		"received:$sampleinterval"	goto confirm
		timeout 3					goto iteration
}

lightpower:
	send "$lightpower"
	expect {
		"received:$lightpower"	goto confirm
		timeout 3				goto iteration	
}

cutvolts:
	send "$cutoffvoltage"
	expect {
		"received:$cutoffvoltage"	goto confirm
		timeout 3					goto iteration
}

dclOnDelay:
	send "$dclOnDelay"
	expect {
		"received:$dclOnDelay"	goto confirm
		timeout 3				goto iteration
}

piOnDelay:
	send "$piOnDelay"			
	expect {
		"received:$piOnDelay"	goto confirm
		timeout 3				goto iteration
}

confirm:
	send "1"
	set a 0
	expect {
		"DONE!"		goto quit
		timeout 3	goto loop
}
alarm:
	print "Alarm!"
	sleep 2
	! pkill minicom

photo:
	print "Photos!"
	sleep 2
	! pkill minicom

quit:
	print "success!"
	sleep 2
	! pkill minicom

error:
	print "FAIL!"
	sleep 2
	! pkill minicom
		
EOL

minicom -D $sleepyserial -C $conlogfile -S $temporary
rm -r $temporary
cat >> $temporary << EOL
expect {
	"s1"		goto quit
	timeout 5 	goto quit
}

quit:
	! pkill minicom
	
EOL

minicom -D $sleepyserial -C $conlogfile -S $temporary
rm -r $temporary
}

#	Function that adds on printed info to config when in verbose mode
verboseConfig() {
	echo "Sample Time set to $sampletime!"
	echo "Sample Time set to $sampleTimeMin!"
	echo "start date set to $startdate!"
	echo "start date set to $startDateEpoch!"
	echo "Light Power set to $lightpower!"
	echo "Sample Interval set to $sampleinterval!"
	echo "Cut off Voltage set to $cutoffvoltage!"
	echo "Light Delay set to $lightdelay!"
	echo "dcl on delay set to $dclOnDelay!"
	echo "pi on delay set to $piOnDelay!"
	echo "burst set to take $burst photos"
	echo "interval set to $interval between photos"
	echo "shutter speed set to $shutter!"
	sleep 1
}

#	Function that replaces the state interpreter when in verbose mode
verboseStateInterpreter()	{
	echo "begin State Interpreter"
	pinstate=$(sudo	cat /sys/class/gpio/gpio$dclpin/value)
	echo "The DCL GPIO pin is $pinstate"
	if	(($pinstate == 1)); 	then
		echo "DCL on starting Rsysncs"
		rsync -rtP -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteConfigDir $configdir
		sudo rsync -rtP $jpg -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
		echo "rsync done getting new configuration"
		config
		verboseConfig
		echo "Starting minicom expect script and configuring sleepy pi"
		sleep 2
		#sleepyConfig 
		#echo "check $conlogfile for sleepy pi configuration"
		#check=$(grep -qw Photos! $conlogfile; echo $?)
		#if (($check == 0)); then 
		#	echo "DCL is OFF Taking photos"
		#	echo "Turning on Lights at $lightpower%"
		#	echo "Turning on Lasers"
		#	echo "Set light delay time to $lightdelay seconds"
		#	python3 /home/camds/bin/lights_lasers_on.py $lightpower $lightdelay &
		#	PID=$!
		#	echo "Taking photos now"
		#	python3 /home/camds/bin/capture_rgb.py $raw $burst $interval $shutter
		#	sleep 2
		#	sudo kill $PID
		#	python3 /home/camds/bin/lights_lasers_off.py
		#	python3 /home/camds/bin/convert_rgb.py $raw $png $jpg
		#	echo "done"
		#	sleep 1
		#	echo "shutting down now...."
		#	sleep 1
		#else
		#configCheck
		#echo "configuration confirmed ok to shut down now"
		#sleep 2
		#fi
	else
		echo "DCL is OFF Taking photos"
		#minicom -D $sleepyserial -C $logfile &
		timedatectl status
		echo "Turning on Lights at $lightpower%"
		echo "Turning on Lasers"
		echo "Set light delay time to $lightdelay seconds"
		python3 /home/camds/bin/lights_lasers_on.py $lightpower $lightdelay &
		PID=$!
		echo "Taking photos now"
		python3 /home/camds/bin/capture_rgb.py $raw $burst $interval $shutter
		echo "done"
		sleep 1
		sudo kill $PID
		python3 /home/camds/bin/lights_lasers_off.py  
		python3 /home/camds/bin/convert_rgb.py $raw $png $jpg
		echo "shutting down now...."
		sleep 1
		sudo rsync -rtP $jpg -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
		#pkill minicom
		sleep 1
	fi
}

# 	Function to check config transfer status
configCheck() {
	check=()
		check+=$(grep -qx sampletime!received:$sampleTimeMin $conlogfile; echo $?)
		check+=$(grep -qx startdate!received:$startDateEpoch $conlogfile; echo $?)
		check+=$(grep -qx interval!received:$sampleinterval $conlogfile; echo $?)
		check+=$(grep -qx lightpower!received:$lightpower $conlogfile; echo $?)
		check+=$(grep -qx dclOnDelay!received:$dclOnDelay $conlogfile; echo $?)
		check+=$(grep -qx piOnDelay!received:$piOnDelay $conlogfile; echo $?)
		check+=$(grep -qx cutvolts!received:$cutoffvoltage $conlogfile; echo $?)
	echo "$check"
	if [ $check == "0000000" ]; then
		echo "config is correct"
		sleep 1
	else
	sleep 1
	echo " sleepy pi config failed on one or more parameters trying again"
	sleep 2
	sleepyConfig
	fi
}


#	Help Function only run with -h options
Help() {
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

Version() {
	echo " camds Version $version created $creation_date"
}
Serial() {
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
			V)	Version
				;;
			h)	Help
				;;
			S)	Serial
				;;
		esac
	done
fi
##
##
