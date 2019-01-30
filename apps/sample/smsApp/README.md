Legato-sms : smsApp Tutorial
========================================

This sample Legato application demonstrates a variety of usecases for sending and receiving text messages as well as managing stored messages using le_sms APIs.

smsApp also allows you to get various information device information by sending requests to the module through SMS. For example, by sending "info firmware" to the device from any phone,
a reply will be sent shortly with information regarding the current firmware installed on the device. A complete list of the available requests is available by typing sms --help

A registered SIM is assumed to be inserted for the send and receive functionalities to work.

This application has been tested on Sierra Wireless WP76xx module hosted on mangOH Green board.

sms
----
Let's send a simple sms. Check whther there are any messages in our inbox and delete them after they've been read. Assuming there are only 2 messages in our inbox:

* sms send 6041234567 "Hi there, how are you?"
* sms inbox
* sms delete
* sms next
* sms delete

Build
-----
This application needs to be installed at the system level in order for the command line options to work. Follow the steps below for properly building and using smsApp:
~~~
mksys -t <TARGET_TYPE> smsApp.sdef
~~~

Install the app on target
-----------------------------------------
~~~
app install smsApp.<TARGET_TYPE>.update <TARGET_IP>
~~~

You are now ready to use smsApp!
--------------------------------------
For more information, type the following on the command line:
~~~
sms --help
~~~