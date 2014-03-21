adb shell mkdir -p /mnt/userrw/testModem

adb push bin\ar7\mdmSrvcsTest /mnt/userrw/testModem
adb push testModem.sh /mnt/userrw/testModem

adb push ..\..\..\framework\c\lib\ar7\liblegato.so /mnt/userrw/testModem
adb push ..\..\..\components\modem\qmi\lib\ar7\libqmipa.so /mnt/userrw/testModem
adb push ..\..\..\apps\modemServices\lib\ar7\libmdmsrvcs.so /mnt/userrw/testModem
adb push ..\..\..\3rdParty\CUnit\src\extract\ar7\CUnit-2.1-2\CUnit\Sources\.libs\libcunit.so.1.0.1 /mnt/userrw/testModem/libcunit.so.1

sleep 1
adb shell chmod u+x /mnt/userrw/testModem/mdmSrvcsTest
adb shell chmod u+x /mnt/userrw/testModem/testModem.sh
