#!/bin/sh

KO_PATH=$1
insmod $KO_PATH

while [ ! -e /dev/spidev* ]
do
  sleep 1
done
