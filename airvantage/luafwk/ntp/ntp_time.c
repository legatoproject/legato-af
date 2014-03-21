/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <sys/time.h>
#include <stdio.h>
#include <errno.h>


#include "swi_log.h"
#include "ntp.h"

extern void print_uint64_hexa(const char* name, uint64_t val);

static uint64_t MAX_NTPFRAC = (((uint64_t) 1) << 32);


/* internal_getntptime: get time as NTP format
 *
 * inputs:
 * time: pointer to uint64_t to put NTP timestamp in.
 *
 * outputs:
 * 0 on success, 1 on error
 */
int internal_getntptime(uint64_t *time) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL)) {
        perror("Cannot do gettimeofday in ntp_time.c");
        return 1;
    }
    if (time == NULL)
        return 1;

    uint64_t res = 0;
    //seconds part
    res = ((uint64_t) (tv.tv_sec + NTP_TIME_BASE)) << 32;
    //frac sec part
    uint64_t tmp = ((uint64_t) tv.tv_usec) * MAX_NTPFRAC;
    tmp /= 1e6;
    res += tmp;

    //SWI_LOG("NTP", DEBUG, "getntptime: tv.tv_sec=%lx, tv.tv_usec=%lx\n", tv.tv_sec, tv.tv_usec);
    //print_uint64_hexa("getntptime: res", res);

    *time = res;
    return 0;
}


/* internal_setntptime: set system time
 *
 * inputs:
 * time: uint64_t NTP timestamp
 *
 * outputs:
 * 0 on success, 1 on error
 */
int internal_setntptime(uint64_t time) {
    struct timeval tv;

    tv.tv_sec = (time >> 32) - NTP_TIME_BASE;

    uint64_t tmp = (time & 0xFFFFFFFF) * 1e6;
    tmp /= MAX_NTPFRAC;
    tv.tv_usec = (int) tmp;

    //SWI_LOG("NTP", DEBUG, "setntptime: tv.tv_sec=%lx, tv.tv_usec=%lx\n", tv.tv_sec, tv.tv_usec);
    if (settimeofday(&tv, NULL)) {
        perror("Cannot do settimeofday in ntp_time.c");
        return 1;
    }
    return 0;
}







