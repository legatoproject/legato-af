#!/bin/sh


configCmd=/usr/local/bin/config
basePath=/lwm2m/definitions/legato/assets/0

assetName=`config get "$basePath/name"`


if [ -z "$assetName" ]
then
    $configCmd set $basePath/name legato

    $configCmd set $basePath/fields/0/name Version
    $configCmd set $basePath/fields/0/access w
    $configCmd set $basePath/fields/0/type string

    $configCmd set $basePath/fields/1/name Restart
    $configCmd set $basePath/fields/1/access x
fi


exec avcDaemon
