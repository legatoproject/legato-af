#!/bin/bash

LoadTestLib
#. ../test.shlib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "IMA SMACK Test Failed!"
}

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# List of apps
appsList="imaFileClient imaFileServer"

echo "******** IMA SMACK Test Starting (on $targetType @ $targetAddr) ***********"

ENABLE_IMA=`printenv LE_CONFIG_ENABLE_IMA`

if  [ -z $ENABLE_IMA ] || [ $ENABLE_IMA -ne y ]; then
    echo "IMA build environment (LE_CONFIG_ENABLE_IMA = y) isn't set properly. IMA test won't run"
    exit 0
fi

echo "Checking whether target is running IMA enabled linux."
cmd="(zcat /proc/config.gz | grep CONFIG_IMA=y) && (cat /proc/cmdline | grep \"ima_appraise=enforce\")"
ssh root@$targetAddr "$cmd"
if [ $? -ne 0 ]; then
    echo "Target $targetAddr isn't running IMA-enabled linux. Test won't run"
    exit 0
fi

cd $scriptDir

echo "Build all the apps."
for app in $appsList
do
    mkapp ${app}.adef -t $targetType
    CheckRet
done

echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

echo "Removing old stale appp if exists."
ssh root@$targetAddr "$BIN_PATH/app remove imaFileClient"
sleep 1

ssh root@$targetAddr "$BIN_PATH/app remove imaFileServer"
sleep 1

ClearLogs

echo "Install the imaFileClient app."
InstallApp imaFileClient.$targetType.signed.update

echo "Run the imaFileClient app."
ssh root@$targetAddr  "$BIN_PATH/app start imaFileClient"
CheckRet

echo "Install the imaFileServer app."
InstallApp imaFileServer.$targetType.signed.update

echo "Run the imaFileServer app."
ssh root@$targetAddr  "$BIN_PATH/app start imaFileServer"
CheckRet

# Wait for all the apps to finish running.
sleep 1

echo "Grepping the logs to check the results."
CheckLogStr "==" 1 "File descriptor was passed correctly."
CheckLogStr "==" 3 "Success: Could not access installed file"

echo "IMA SMACK Test Passed!"
exit 0
