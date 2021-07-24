#!/bin/sh
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
KO_PATH=$1

printf "/legato/systems/current/modules/files/wlcore/bin" > /sys/module/firmware_class/parameters/path

insmod "$KO_PATH" || exit 127

exit 0
