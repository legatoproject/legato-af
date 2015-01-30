/**
 * @file pa_simu.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_SIMU_INCLUDE_GUARD
#define PA_SIMU_INCLUDE_GUARD

#define PA_SIMU_CFG_MODEM_ROOT	"/simulation/modem"

/* Info */

#define PA_SIMU_INFO_DEFAULT_IMEI           "314159265358979"
#define PA_SIMU_INFO_DEFAULT_FW_VERSION     "Firmware 1.00"
#define PA_SIMU_INFO_DEFAULT_BOOT_VERSION   "Bootloader 1.00"
#define PA_SIMU_INFO_DEFAULT_DEVICE_MODEL   "VIRT_X86"

/* Radio Control */

#define PA_SIMU_MRC_DEFAULT_NAME    "Simu"
#define PA_SIMU_MRC_DEFAULT_RAT     "UMTS"
#define PA_SIMU_MRC_DEFAULT_MCC     "01"
#define PA_SIMU_MRC_DEFAULT_MNC     "001"

le_result_t mrc_simu_Init
(
    void
);

bool mrc_simu_IsOnline
(
    void
);

/* SIM */

#define PA_SIMU_SIM_DEFAULT_ICCID   "12345678901234567890"
#define PA_SIMU_SIM_DEFAULT_IMSI    "424242424242424"
#define PA_SIMU_SIM_DEFAULT_NUM     "+33600112233"
#define PA_SIMU_SIM_DEFAULT_PIN     "0000"
#define PA_SIMU_SIM_DEFAULT_PUK     "12345678"
#define PA_SIMU_SIM_DEFAULT_STATE   "READY"
#define PA_SIMU_SIM_DEFAULT_CARRIER "Simu"
#define PA_SIMU_SIM_DEFAULT_MCC     "01"
#define PA_SIMU_SIM_DEFAULT_MNC     "001"

le_result_t sim_simu_Init
(
    void
);

/* SMS */

#define PA_SIMU_SMS_DEFAULT_SMSC    ""

le_result_t sms_simu_Init
(
    void
);

/* MDC */

#define PA_SIMU_MDC_DEFAULT_IF      "eth0"
#define PA_SIMU_MDC_DEFAULT_APN     "internet"
#define PA_SIMU_MDC_DEFAULT_GW      "192.168.100.1"
#define PA_SIMU_MDC_DEFAULT_IP      "192.168.100.10"
#define PA_SIMU_MDC_PRIMARY_DNS     "8.8.8.8"
#define PA_SIMU_MDC_SECONDARY_DNS   "8.8.4.4"

le_result_t mdc_simu_Init
(
    void
);

/* eCall */

#define PA_SIMU_ECALL_DEFAULT_PSAP                  "+4953135409300"
#define PA_SIMU_ECALL_DEFAULT_MAX_REDIAL_ATTEMPTS   3
#define PA_SIMU_ECALL_DEFAULT_MSD_TX_MODE           LE_ECALL_TX_MODE_PUSH

le_result_t ecall_simu_Init
(
    void
);

/* Firmware Update */

le_result_t fwupdate_simu_Init
(
    void
);

#endif /* PA_SIMU_INCLUDE_GUARD */
