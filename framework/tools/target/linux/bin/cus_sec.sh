#!/bin/sh

# Customer need to fill this script. This script is used to perform customer security check.
# Data is received via parameter $1 $2: $1 is the path of CUSG image, $2 is the SHA256 value
# for the package.
# Exit with any non-zero exit code will be considered as failure.

# Here do nothing but print $1 and $2 for test then return 0 for default ok.
echo $1
echo $2
exit 0