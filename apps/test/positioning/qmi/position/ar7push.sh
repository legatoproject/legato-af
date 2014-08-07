#!/bin/bash

targetName=$1

sourceDir="../../../../../build/ar7/bin"
targetDir="/tmp/testPos"

ssh $targetName mkdir -p $targetDir

scp positionTest.sh $targetName:$targetDir/positionTest.sh
scp $sourceDir/tests/testPaQmiPos $targetName:$targetDir/testPaQmiPos

scp $sourceDir/serviceDirectory $targetName:$targetDir/serviceDirectory
scp $sourceDir/logCtrlDaemon $targetName:$targetDir/logCtrlDaemon

scp $sourceDir/lib/liblegato.so $targetName:$targetDir/liblegato.so
scp $sourceDir/lib/lible_pa.so $targetName:$targetDir/lible_pa.so
scp $sourceDir/lib/lible_mdm_services.so $targetName:$targetDir/lible_mdm_services.so
scp $sourceDir/lib/lible_pa_gnss.so $targetName:$targetDir/lible_pa_gnss.so

ssh $targetName chmod u+x $targetDir/testPaQmiPos
ssh $targetName chmod u+x $targetDir/positionTest.sh
ssh $targetName chmod u+x $targetDir/serviceDirectory
ssh $targetName chmod u+x $targetDir/logCtrlDaemon
