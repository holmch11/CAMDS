# Welcome

This wiki covers the software set up of a new OOI EA CAMDS underwater camera.  If creating a new camera unit it is possible to deploy from a brand new SD card however starting from a cloned SD card is **highly recommended**.  This is for two important reasons; 1) it is a lot less work, 2) the DCLs and your computer will recognize all the CAMDS units that are cloned as a single CAMDS when using ssh to connect.  If you have a new card in a unit it will give man in the middle attack warnings/errors if you already have connected to a different CAMDS unit.  This will require removal of any saved data from known hosts for 192.168.0.147 (CAMDS IP) before connecting, this can be avoided when all the units share a cloned set of cards.    

[How to Start with Cloned Card](https://bitbucket.org/ooicgsn/ea_camds/wiki/Starting%20With%20Cloned%20SD%20card)

[How to Start with a New Card](https://bitbucket.org/ooicgsn/ea_camds/wiki/Creating%20CAMDS%20from%20New%20SD%20card)

## Relevant Information and File Names
Relevant controlled engineering drawings are on [OOI Alfresco](https://alfresco.oceanobservatories.org/alfresco/faces/jsp/browse/browse.jsp)

3305-20013 CAMDS INTERNAL WIRING                        

3705-30170 CAMDS INTERFACE PCB BOARD                    

3705-20170 CAMDS INTERFACE PCB SCH                      

3705-10170 CAMDS INTERFACE PCB BOM                      

3705-00170 CAMDS INTERFACE PCB                           

3705-00087 CAMDS POWER LIGHTS AND LASERS CABLE ASSEMBLY  

3705-00115 CAMDS DCL CAMERA WAKE CABLE ASSEMBLY       

## Primary COTS Parts

[Raspberry Pi 4GB](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)

[Sleepy Pi 2 USB C with low profile header](https://spellfoundry.com/product/sleepy-pi-2-usb-c/)

[Raspberry Pi HQ camera](https://www.raspberrypi.com/products/raspberry-pi-high-quality-camera/)

[AIDA fixed 2.8f Wide Angle Lens](https://aidaimaging.com/cs-2-8f/)

[Waveshare 3 port Relay Board](https://www.waveshare.com/rpi-relay-board.htm)

Have Fun!