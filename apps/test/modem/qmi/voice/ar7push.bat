adb shell mkdir -p /tmp/testModem

adb push voiceTest.sh /tmp/testModem

adb push ..\..\..\..\..\build\ar7\bin\tests\testPaQmiVoice /tmp/testModem

adb push ..\..\..\..\..\build\ar7\bin\lib\liblegato.so /tmp/testModem
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_pa.so /tmp/testModem
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_mdm_services.so /tmp/testModem

adb shell chmod u+x /tmp/testModem/testPaQmiVoice
adb shell chmod u+x /tmp/testModem/voiceTest.sh
