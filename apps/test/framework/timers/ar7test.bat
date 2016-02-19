adb shell mkdir -p /mnt/userrw/timerTest
adb push bin\ar7\testTimer /mnt/userrw/timerTest
adb push testTimer.sh /mnt/userrw/timerTest
adb push ..\..\..\..\framework\c\lib\ar7\liblegato.so /mnt/userrw/timerTest
sleep 1
adb shell chmod u+x /mnt/userrw/timerTest/testTimer
adb shell chmod u+x /mnt/userrw/timerTest/testTimer.sh
