#!/bin/bash

targetName=$1

sourceDir="../../../../../build/ar7/bin"
targetDir="/tmp/testVoice"

ssh $targetName mkdir -p $targetDir

scp voiceTest.sh $targetName:$targetDir/voiceTest.sh

scp $sourceDir/tests/testPaQmiVoice $targetName:$targetDir/testPaQmiVoice

scp $sourceDir/lib/liblegato.so $targetName:$targetDir/liblegato.so
scp $sourceDir/lib/lible_pa.so $targetName:$targetDir/lible_pa.so
scp $sourceDir/lib/lible_mdm_services.so $targetName:$targetDir/lible_mdm_services.so

ssh $targetName chmod u+x $targetDir/testPaQmiVoice
ssh $targetName chmod u+x $targetDir/voiceTest.sh
