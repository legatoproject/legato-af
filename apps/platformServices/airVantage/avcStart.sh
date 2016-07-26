#!/bin/sh

configCmd=/legato/systems/current/bin/config
asset0Name=`$configCmd get "/lwm2m/definitions/legato/assets/0/name"`
asset1Name=`$configCmd get "/lwm2m/definitions/legato/assets/1/name"`


if [ -z "$asset0Name" ] || [ -z "$asset1Name" ]
then
    $configCmd import system:/lwm2m/definitions/legato cfg/assets.cfg
fi


exec avcDaemon
