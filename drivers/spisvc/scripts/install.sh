#!/bin/sh

KO_PATH=$1

modprobe -q spidev
insmod "$KO_PATH"

# Make 10 attempts to check whether dev file exists
# Sleep 1s in between

for i in $(seq 1 10)
do
  if [ ! "$(find /dev/spidev* 2> /dev/null | wc -l)" -eq "0" ]
  then
    exit 0
  fi
  sleep 1
done

# return error if device file hasn't been created after timeout
if [ "$i" -eq "10" ]
then
  exit 1
fi

exit 0
