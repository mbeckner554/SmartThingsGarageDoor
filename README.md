# SmartThingsGarageDoor
Project to create a SmartThings compatible garage door switch using a ESP8266 chip that will control two garage doors in Samsung SmartThings and from Amazon Echo.
The switch has pin outs for 2 LEDs, one indicator LED (notificationPin1) that blinks every 90 seconds when the switch is on and operational,
and another warning light (alarmPin1) for when the switch is counting down to open/close.  Additionally you can connect an audible alarm to alarmPin1
for additional safety. 

## WifiGarageSwitch.ino - ESP8266 Switch Programming
Use the WifiGarageSwitch.ino file to program the ESP8266 switch.  I used a dual 5 volt relay to connect to both my garage doors.
You can adjust the pin outs for the relayPin1, relayPin2, alarmPin1 and notificationPin1.

 * This was derived from the JZ-SmartThings project for a Generic HTTP Device
 * https://github.com/JZ-SmartThings/SmartThings/blob/master/Devices/Generic%20HTTP%20Device
 
## ESP8266Switch.groovy - SmartThings DeviceHandler
Device handler for Samsung SmartThings.  Also derived from the original code in the Jz_SmartThings project, but updated to fit my installation.  Removed Temp 
and humidity tiles as they were not needed.

## VirtualFarageDoor.groovy - SmartThings Smartapp 
This smartapp will use two different devices (tilt sensor and garage door switch)to sync with the virtual garage door switch in SmartThings.
The app was derived from LGK Virtual Garage Door, but with several modifications that fit my installation:

 * Added an input in the preferences for 'alexavirtualswitch' to allow a on/off switch control the up/down operations
 * Added an input in the preferences for 'customTrigger' that will use the .off() command on the WifiGarageSwitch above to control a second garage door
 * The original program would retry the garage commands when the door tilt sensor and virtual door were out of sync, which resulted in unattended 
	opening/closing of the garage.  Instead of having the commands re-trigger I configured the app to turn off the subscriptions for a short period of 
	time while the virtual garage buttons were switched.  I also added additional push notifications when this would happen to alert users when garage 
	was opened manually via the wall switch, through the virtual switch in the app, or using the Alexa Switch.
	

I plan on adding additional functionality in the future for presence control to open a specific garage door based on which user has just arrived.

