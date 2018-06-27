#!/bin/sh

KO_PATH="$1"
insmod "$KO_PATH"

while [ -z "$(find -name "/dev/spisvc*")" ]
do
  sleep 1
done
