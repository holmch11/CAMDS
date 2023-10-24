#!/bin/bash
# This starts camds.sh with a daily logfile
# The logfile for this can be seen in the journal 
# Version History
#**********************************************************************#
# 2021/07/09 CEH Initial Version Chris Holm 	holmch@oregonstate.edu	
# 2021/07/12 CEH Added shutdown control to this script with dcl pin query
# 2021/07/19 CEH Deleted space between # and ! in /bin/bash	
# 2021/08/31 CEH Forked Version simplified version
# 2021/09/01 CEH Added DCL off early shutdown
# 2022/06/23 CEH Added camera SN variable
# 2022/06/24 CEH Using tee to direct output to logfile
# 2022/07/07 CEH Added -a to tee so log is appended to logfile
#**********************************************************************#

serial_number=0120
dclpin=19
filedate=`date -u +%Y%m%d`	# get todays date for appending to file names
logfile="/home/camds/logs/$filedate.txt"
RemoteDataDir="root@192.168.0.137:/data/cg_data/camds/" 
interrupt_delay=240
time=0
time_elapsed=0
time_left=0

sleep 2
# Set the dcl pin for input
if [ ! -d "/sys/class/gpio/gpio/gpio$dclpin" ] 
then
	sudo echo "$dclpin"	> 	/sys/class/gpio/export
	sleep 1
	sudo echo "in"		> 	/sys/class/gpio/gpio$dclpin/direction
else
	sudo echo in > /sys/class/gpio/gpio$dclpin/direction
fi

sleep 6
/home/camds/bin/camds.sh -v 2>&1 | tee -a $logfile
sleep 2
pinstate=$(sudo	cat /sys/class/gpio/gpio$dclpin/value)
echo "DCL pin is $pinstate"
sleep 1
sudo rsync -aP $logfile /media/camds/EACAM$serial_number/logs/
sudo rsync -rtP $logfile -e "ssh -i /home/camds/.ssh/id_rsa" $RemoteDataDir
sleep 1
if (($pinstate == 1)); then
	time=$SECONDS
	time_elapsed=$((SECONDS - time))
	while [ "$time_elapsed" -lt "$interrupt_delay" ] && [ "$pinstate" -eq "1" ]; do
		time_elapsed=$((SECONDS - time))
		time_left=$((interrupt_delay - time_elapsed))
		sudo wall "You have $time_left seconds to kill $BASHPID before shutdown"
		echo "You have $time_left seconds to kill $BASHPID before shutdown"
		sleep 10
		pinstate=$(sudo cat /sys/class/gpio/gpio$dclpin/value)
		sudo wall "DCL pin is $pinstate"
	done
fi
sleep 1
echo "shutting down now bye"
sudo wall "Shutting down now bye"
sleep 2 
sudo shutdown -h now
