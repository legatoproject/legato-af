Legato-AirVantage : Asset Data Tutorial
======================================

This sample Legato application demonstrates data exchange feature between an embedded Legato application and AirVantage server, over the LWM2M protocol. This latter feature requires Legato 17.06. Simulated room temperatures are pushed to AirVantage every 10 seconds.

This application has been tested on Sierra Wireless AirPrime WP8548 module hosted on mangOH board.

Asset Data
----------
Let’s design a simple home automation scenario to monitor and to control temperature of a Room, remotely using AirVantage.
A room has :
* __3 variables__ : *ROOM_NAME_VAR_RES*, *IS_AC_ON_VAR_RES*, *ROOM_TEMP_READING_VAR_RES*. AirVantage only has read-access to these variables
* __1 setting__ : *THERMOSTAT_TEMP_SET_RES* that can be changed by AirVantage. User can therefore set the temperature of the room remotely
* __1 command__ : *AC_CMD_TURN_OFF_RES* that can be triggerred by AirVantage upon user request.

This collection of data are declared variable of Linux path.<br>


Application Scenario
--------------------
AirVantage can do the following actions, for each instance:
* Retrieve the name of the room, the current room temperature and the current status of the AC (On or Off)
* Set the target temperature of the room, the AC is automatically switched On
* Trigger a command to turn off the AC


Build
-----
To build executable for WP8548:
~~~
make wp85
~~~

Install the app on target (mangOH/WP8548)
-----------------------------------------
~~~
instapp assetData.wp85.update <IP address of target>
~~~


Create an update package & Generate package for upload to AirVantage
--------------------------------------------------------------------
Do the following only Once:
~~~
av-pack -u assetData.wp85.update -b _build_assetData/wp85/ -t <TypeName>, where <TypeName> shall be globally unique (e.g. AssetDataTutorialApp). AirVantage uses it to uniquely identify multiple versions of this application
~~~


Start the app on target (mangOH/WP8548)
-----------------------------------------
ssh to target then launch the app as follow:
~~~
app start assetData
~~~
