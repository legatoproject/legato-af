#!/bin/bash

rm -rf ./build/; mkexe -o ct configTest -i ../../../interfaces/config/ -v -g " -I ../../../interfaces/config/ " -w ./build/

./build/ct

for VAR in {1..2000}
do

    ./build/ct > /dev/null
#    echo "Test: " $VAR

done

