#!/bin/sh

#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#   http://www.eclipse.org/legal/epl-v10.html
# The Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************
BASEDIR=$(cd $(dirname $0) && pwd)
OUTPUT_DIR=$1/C
mkdir -p $1
rm -fr $OUTPUT_DIR
export BASEDIR=$BASEDIR
doxygen $BASEDIR/Doxyfile
cp -r C_User_API_doc/html $OUTPUT_DIR


