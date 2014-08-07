#!/bin/bash

targetName=$1

sourceDir="../../../../../build/ar7/bin"
targetDir="/tmp/testSim"

ssh $targetName mkdir -p $targetDir

scp simTest.sh $targetName:$targetDir

scp $sourceDir/tests/testPaQmiSim $targetName:$targetDir

scp $sourceDir/lib/liblegato.so $targetName:$targetDir
scp $sourceDir/lib/lible_pa.so $targetName:$targetDir
scp $sourceDir/lib/lible_mdm_services.so $targetName:$targetDir

ssh $targetName chmod u+x $targetDir/testPaQmiSim
ssh $targetName chmod u+x $targetDir/simTest.sh
