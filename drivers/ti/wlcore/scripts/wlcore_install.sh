#!/bin/sh
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# TI WIFI IoT board is managed by SDIO/MMC bus.
# TI WIFI IoT conflicts with others devices using the SDIO/MMC bus

# Some things are very specific to kernel version we are running.
KVERSION=

KO_PATH=$1

echo -n /legato/systems/current/modules/files/wlcore/bin > /sys/module/firmware_class/parameters/path

# Make sure path thing is set properly.
export PATH=$PATH:/usr/bin:/bin:/usr/sbin:/sbin


# Extract kernel version
KVERSION=$( uname -r | awk -F. '{ print $1$2 }' )

# This is only required for WP85, because kernel is different.
if [ "${KVERSION}" = "314" ] ; then
    # Check if MMC/SDIO module is inserted. Because WIFI use SDIO/MMC bus
    # we need to remove the SDIO/MMC module
    lsmod | grep msm_sdcc >/dev/null
    if [ $? -eq 0 ]; then
        grep -q mmcblk /proc/mounts
        if [ $? -ne 0 ]; then
            rmmod msm_sdcc
        else
            false
        fi
        if [ $? -ne 0 ]; then
            # Unable to remove. May be others devices use SDIO/MMC bus
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Unable to remove the SDIO/MMC module... May be in use ?"
            echo "Please, free all SDIO/MMC devices before using TI WIFI."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            exit 127
        fi
    fi
    modprobe msm_sdcc || exit 127
fi

modprobe mac80211 || exit 127
insmod $KO_PATH || exit 127

exit 0
