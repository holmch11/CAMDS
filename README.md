# README #

To change this README, you must have permission to change the master branch. Alternatively, you can create a branch, edit the README in the branch, and create a Pull Request to get it merged back into the master branch.

This captures an overview of the EA CAMDS Control Software as well as the steps needed to prep for deployment of the EA CAMDS Control Software.

### Contribution guidelines ###
    * Create a new branch for any new development
    * Ensure any updates are properly reviewed/tested prior to deployment by creating pull requests in BitBucket
* Writing tests - None
* Other guidelines - None

Who do I talk to?
Contact Chris Holm christopher.holm@oregonstate.edu


* Version
This project uses [Semantic Versioning](https://semver.org/) with Major:Minor:Patch levels designtated in the VERSION
file. Be sure to include/update the version level as appropriate based on the guidlines for Semantic Versioning and
provide that information in the pull request so the project admin can appropriately tag the code for automated builds
and testing.
# ARCHITECTURE
The Basic setup of this system is a Raspberry PI HQ camera that takes a burst of 3 photos in RAW format, converts them to reduced size JPG and Rsyncs those files up to a connnected datalogging computer on the OOI mooring.  The RAW photos are stored with log files for removal upon recovery.  The Pi executes this upon start up and then shutsdown. 
The timing of pictures is then controlled by Sleepy Pi turning on and off the Raspberry Pi (and camera).  In this version it is hard coded to 4 hours, Pi is turned on and then sleepy pi waits for the pi to shutdown before removing power. One vunerability in this system is if for some reason Sleepy Pi doesn't remove power from the Pi, it will not boot again and the battery will be drained.  Multiple layers of checks were created to prevent this including a timeout.  Sleepy pi then sets a new wake signal before going to sleep itself.  The sleepy pi can be woken up asyncronously with a wake pin going high, in this case that is wired to the datalogger (DCL) power pin for the CAMDS port.   
# SETUP

This wiki covers the software set up of a new OOI EA CAMDS underwater camera.  If creating a new camera unit it is possible to deploy from a brand new SD card however starting from a cloned SD card is **highly recommended**.  This is for two important reasons; 1) it is a lot less work, 2) the DCLs and your computer will recognize all the CAMDS units that are cloned as a single CAMDS when using ssh to connect.  If you have a new card in a unit it will give man in the middle attack warnings/errors if you already have connected to a different CAMDS unit.  This will require removal of any saved data from known hosts for 192.168.0.147 (CAMDS IP) before connecting, this can be avoided when all the units share a cloned set of cards.    

[How to Start with Cloned Card](https://github.com/holmch11/CAMDS/blob/Production/Starting%20With%20Cloned%20SD%20card.md)


[How to Start with a New Card](https://github.com/holmch11/CAMDS/blob/Production/Creating%20CAMDS%20from%20New%20SD%20card.md)


## Relevant Information and File Names
Relevant controlled engineering drawings are on [OOI Alfresco](https://alfresco.oceanobservatories.org/alfresco/faces/jsp/browse/browse.jsp)

3305-20013 CAMDS INTERNAL WIRING                        

3705-30170 CAMDS INTERFACE PCB BOARD                    

3705-20170 CAMDS INTERFACE PCB SCH                      

3705-10170 CAMDS INTERFACE PCB BOM                      

3705-00170 CAMDS INTERFACE PCB                           

3705-00087 CAMDS POWER LIGHTS AND LASERS CABLE ASSEMBLY  

3705-00115 CAMDS DCL CAMERA WAKE CABLE ASSEMBLY     

THESE CAN BE MADE AVAILABLE OUTSIDE OF OOI FROM AUTHOR ABOVE, HOWEVER UTILITY IS VERY OOI SPECIFIC

## Primary COTS Parts

[Raspberry Pi 4GB](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)

[Sleepy Pi 2 USB C with low profile header](https://spellfoundry.com/product/sleepy-pi-2-usb-c/)

[Raspberry Pi HQ camera](https://www.raspberrypi.com/products/raspberry-pi-high-quality-camera/)

[AIDA fixed 2.8f Wide Angle Lens](https://aidaimaging.com/cs-2-8f/)

[Waveshare 3 port Relay Board](https://www.waveshare.com/rpi-relay-board.htm)

Have Fun!
