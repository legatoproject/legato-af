adb shell mkdir -p /mnt/userrw/testModem


adb push simTest.sh /mnt/userrw/testModem

adb push ..\..\..\..\..\build\ar7\bin\tests\testPaQmiSim /mnt/userrw/testModem

adb push ..\..\..\..\..\build\ar7\bin\lib\liblegato.so /mnt/userrw/testModem
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_pa.so /mnt/userrw/testModem
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_mdm_services.so /mnt/userrw/testModem

adb shell chmod u+x /mnt/userrw/testModem/testPaQmiSim
adb shell chmod u+x /mnt/userrw/testModem/simTest.sh
