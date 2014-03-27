#!/bin/bash

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


usage()
{
cat << EOF
usage: $0 [options] [destination]

This script create a new available site file for apache2 and reload apache2 configuration.

OPTIONS:
   -h      Show this message
   -n      target name to use (default is native)
   -c      lua target configuration file (default is defaulttargetconfig.lua)
   -p      policy to use (default is OnCommit)
   -t      lua test definition file (default is defaulttestconfig.lua)

DESTINATION:
   Destination path for the tester.
   If not defined, create a directory name based on the date/hour in the current location

EOF
}

POLICY="OnCommit"
NAME="native"
TARCONF="defaulttargetconfig.lua"
USETARCONF=0
TESTCONF="defaulttestconfig.lua"
USETESTCONF=0

INITIAL_WD=$(pwd)
cd $(dirname $0)

#store the SVN directory of Mihini
RA_SVNDIRTEST=$(pwd)
RA_SVNDIR=$RA_SVNDIRTEST/..

OPTIND=1
while getopts 'hp:n:c:t:' OPTION
do
    case $OPTION in
    h)    usage
          exit 0
          ;;
    p)    POLICY="$OPTARG"
          echo ">>> Using test policy $OPTARG"
          ;;
    n)    NAME="$OPTARG"
          echo ">>> Using target name: $OPTARG"
          ;;
    c)    TARCONF="$OPTARG"
          if [ -f $TARCONF ]; then
            USETARCONF=1
            cd $(dirname $TARCONF)
            TARCONFDIR=$(pwd)
            cd $INITIAL_WD
            echo ">>> Using target configuration: $OPTARG"
          else
            echo "$TARCONF is not a file"
            exit 1
          fi
          
          ;;
    t)    TESTCONF="$OPTARG"
          if [ -f $TESTCONF ]; then
            USETESTCONF=1
            cd $(dirname $TESTCONF)
            TESTCONFDIR=$(pwd)
            cd $INITIAL_WD
            echo ">>> Using test definition: $OPTARG"
          else
            echo "$TESTCONF is not a file"
            exit 1
          fi
          
          ;;
    ?)    usage
          exit 2
          ;;
    esac
done

# remove optional parameters from command line and keep end of command line
shift $(($OPTIND - 1))

# Choose where to build
if [ $# -gt 0 ]; then
    DESTDIR=$1
else
    NOW=$(date +"%Y%m%d%H%M%S")
    DESTDIR="$INITIAL_WD/tests$NOW"
fi

echo "Destination directory is $DESTDIR"

#create and move to destination dir
if [ -d $DESTDIR ]; then
    #nothing to do
    echo "Destination already exists ... Updating"
else
    echo "Creating new destination $DESTDIR"
    mkdir -p $DESTDIR
fi

cd $DESTDIR

#cmake Mihini
cmake -DPLATFORM=default $RA_SVNDIR

#Build Agent and tests suites
make testsauto mihini_misc lua all
makeret=$?
if [ $makeret -ne 0 ]; then
    echo "Make error"
    exit $makeret
fi

DESTCONFFILES="$DESTDIR/runtime/lua/tester"
if [ $USETARCONF -ne 0 ]; then
  echo "Copying $TARCONF to destination ($DESTCONFFILES)"
  cp $TARCONFDIR/$(basename $TARCONF) $DESTCONFFILES
fi

cpret=$?
if [ $cpret -ne 0 ]; then
  echo "File copy execution error"
  exit $cpret
fi

if [ $USETESTCONF -ne 0 ]; then
  echo "Copying $TESTCONF to destination"
  cp $TESTCONFDIR/$(basename $TESTCONF) $DESTCONFFILES
fi

cpret=$?
if [ $cpret -ne 0 ]; then
  echo "File copy execution error"
  exit $cpret
fi

echo "Environment ready - starting tests"
#run tests
#change working directory into lua directory
cd runtime/lua

#start lua program
TESTCONFFILE=$(basename $TESTCONF)
TESTCONFNAME=${TESTCONFFILE/\.lua/}
TARCONFFILE=$(basename $TARCONF)
TARCONFNAME=${TARCONFFILE/\.lua/}
echo "Using $TARCONFNAME as target definition file"
../bin/lua ./tester/testMain.lua $RA_SVNDIR "policy=$POLICY;target=$NAME" $TESTCONFNAME $TARCONFNAME

testret=$?

#restore initial working dir
cd $INITIAL_WD

if [ $testret -ne 0 ]; then
    echo "Tests execution error"
    exit $testret
fi
