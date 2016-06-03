/**
 * @file pa_sms_simu.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#ifndef LEGATO_PA_SMS_SIMU_INCLUDE_GUARD
#define LEGATO_PA_SMS_SIMU_INCLUDE_GUARD

typedef struct __attribute__((__packed__)) {
    uint8_t origAddress[LE_MDMDEFS_PHONE_NUM_MAX_LEN];
    uint8_t destAddress[LE_MDMDEFS_PHONE_NUM_MAX_LEN];
    pa_sms_Protocol_t protocol;
    uint32_t dataLen;
    uint8_t data[];
}
pa_sms_SimuPdu_t;

#endif
