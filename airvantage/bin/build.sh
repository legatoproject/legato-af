#!/bin/sh

#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Usage:
# build.sh [-d] [-n] [-m ] [-t <build target>] [-C target_dir]


EXEC_DIR=$(pwd)
BIN_DIR=$(dirname $(readlink -f $0))
ROOT_DIR=$(dirname $BIN_DIR)
CMAKE_DIR=$ROOT_DIR/cmake
SOURCECODE_DIR=$ROOT_DIR

PROFILE=
TARGET=default
NO_CLEAN=0

while getopts 'dnmt:C:' OPTION
do
    case $OPTION in
    d)    PROFILE="Debug"
          echo '>>> Set DEBUG option to TRUE'
                ;;
    n)    NO_CLEAN=1
          echo ">>> No clean option activated"
                ;;
    m)    PROFILE="MinSizeRel"
          echo '>>> Set Minimum Size release option to TRUE'
                ;;
    t)    TARGET="$OPTARG"
          echo ">>> Set TARGET type to $OPTARG"
                ;;
    C)    BUILD_DIR="$OPTARG"
          echo ">>> Set BUILD DIRECTORY to $OPTARG"
                ;;
    *)    printf "Usage: %s: [-d] [-n] [-m] [-t target] [-C target_dir]\n" $(basename $0) >&2
            exit 2
                ;;
    esac
done

if [ ! $BUILD_DIR ]
then
  BUILD_DIR=$(pwd)/build.$TARGET
  echo ">>> Set BUILD DIRECTORY to default value: $BUILD_DIR"
fi

# go into the root directory
cd $ROOT_DIR
ret=$?
if [ $ret -ne 0 ]
then
    echo "Command \"cd $ROOT_DIR\" failed"
    exit $ret
fi

# check if the target exist
if [ ! -e $CMAKE_DIR/toolchain.$TARGET.cmake ]
then
    echo "No toolchain file found for target \"$TARGET\"" >&2
    exit 2
fi

#look for source directory
SOURCECODE_DIR=$ROOT_DIR

#clean build directory
#clean build directory
if [  -d $BUILD_DIR ]
then
    if [ $NO_CLEAN -ne 1 ]
    then
      rm -fr $BUILD_DIR
      ret=$?
      if [ $ret -ne 0 ]
      then
          echo "Command \"rm -fr $BUILD_DIR\" failed"
          exit $ret
      fi
    fi
fi

mkdir -p $BUILD_DIR
ret=$?
if [ $ret -ne 0 ]
then
    echo "Command \"mkdir -p $BUILD_DIR\" failed"
    exit $ret
fi

cd $BUILD_DIR
ret=$?
if [ $ret -ne 0 ]
then
    echo "Command \"cd $BUILD_DIR\" failed"
    exit $ret
fi

#launch cmake
CMAKE_OPT="-DCMAKE_TOOLCHAIN_FILE=$CMAKE_DIR/toolchain.$TARGET.cmake -DPLATFORM=$TARGET"
if [ $PROFILE ]
then
    CMAKE_OPT="$CMAKE_OPT -DCMAKE_BUILD_TYPE=${PROFILE}"
else
    CMAKE_OPT="$CMAKE_OPT -DCMAKE_BUILD_TYPE=Release"
fi

cmake $CMAKE_OPT $SOURCECODE_DIR
ret=$?
if [ $ret -ne 0 ]
then
    echo "Command \"cmake $CMAKE_OPT $SOURCECODE_DIR\" failed"
    exit $ret
fi


# Do the compilation
cores=1
cores=$(getconf _NPROCESSORS_ONLN)
make -j${cores} mihini
#make -j${cores} all
ret=$?
if [ $ret -ne 0 ]
then
    echo "Command \"make\" failed"
    exit $ret
fi
