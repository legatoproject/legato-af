#!/bin/sh
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# TI WIFI IoT board is managed by SDIO/MMC bus.
# TI WIFI IoT conflicts with others devices using the SDIO/MMC bus

# Some things are very specific to kernel version we are running.
KVERSION=

KO_PATH=$1


# Make sure path thing is set properly.
export PATH=$PATH:/usr/bin:/bin:/usr/sbin:/sbin

# Extract kernel version
KVERSION=$( uname -r | awk -F. '{ print $1$2 }' )

    rmmod $KO_PATH >/dev/null 2>&1
    rmmod mac80211 >/dev/null 2>&1
    rmmod cfg80211 >/dev/null 2>&1

# compat and msm_sdcc exist on kernel 3.14 only.
if [ "${KVERSION}" = "314" ] ; then
    tmp=$( lsmod | grep "^compat " )
    if [ "x$tmp" != "x" ] ; then
        rmmod compat >/dev/null 2>&1
    fi

    rmmod msm_sdcc >/dev/null 2>&1
    # Insert back MMC/SDIO module
    modprobe msm_sdcc || exit 127
fi

exit 0
