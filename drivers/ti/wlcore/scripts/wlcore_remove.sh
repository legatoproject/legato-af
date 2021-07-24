#!/bin/sh
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#

KO_PATH=$1

rmmod "$KO_PATH" >/dev/null 2>&1

exit 0
