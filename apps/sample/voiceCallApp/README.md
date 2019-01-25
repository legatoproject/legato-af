Legato-voiceCall : voiceCallApp Tutorial
========================================

This sample Legato application demonstrates a variety of usecases for making and receiving voice calls using le_voicecall as well as le_audio APIs.

This application is dependant on the voiceCallService and requires it to be running. None of the functionalities will work if the service is not running. A registered SIM is assumed to be inserted.

This application has been tested on Sierra Wireless WP76xx module hosted on mangOH Green board.

voice call
----------
Let's make a simple voice call. We will first choose our custom <ourWavFile.wav> for our hold music, initiate the call, place the call on hold when the other side picks up, take the out of hold, hang up, and finally redial the same number with a few simple commands through the command line:

* voice wav /path/to/our/wave/file/ourWavFile.wav
* voice call 6041234567
* voice hold
* voice unhold
* voice hangup
* voice redial

Audio
----------
The audio APIs allow us to transmit and receive voice through different means throughout the voice call. It all happens automatically in this app. By simply placing a call on hold, a default sound file will be played for the other side. By unholding a call, the mic and speakers will work as expected and transmit and receive voice for having a conversation.

Build
-----
This application needs to be installed at the system level in order for the command line options to work. Follow the steps below for properly building and using voiceCallApp:
~~~
mksys -t <TARGET_TYPE> voiceCallApp.sdef
~~~

Install the app on target
-----------------------------------------
~~~
app install voiceCallApp.<TARGET_TYPE>.update <TARGET_IP>
~~~

When the package is successfully installed, voiceCallService needs to be manually started:
------------------------------------------------------------------------------------------
Do the following on the target:
~~~
app start voiceCallService
~~~

You are now ready to use voiceCallApp!
--------------------------------------
For more information, type the following on the command line:
~~~
voice --help
~~~