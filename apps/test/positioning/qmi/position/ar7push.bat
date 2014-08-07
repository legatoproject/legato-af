adb shell mkdir -p /tmp/testPosition

adb push positionTest.sh /tmp/testPosition

adb push ..\..\..\..\..\build\ar7\bin\tests\testPaQmiPos /tmp/testPosition

adb push ..\..\..\..\..\build\ar7\bin\lib\liblegato.so /tmp/testPosition
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_pa.so /tmp/testPosition
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_mdm_services.so /tmp/testPosition
adb push ..\..\..\..\..\build\ar7\bin\lib\lible_pa_gnss.so /tmp/testPosition

adb shell chmod u+x /tmp/testPosition/testPaQmiPos
adb shell chmod u+x /tmp/testPosition/positionTest.sh
