/**
 * @file pa_simu.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_SIMU_INCLUDE_GUARD
#define PA_SIMU_INCLUDE_GUARD

#define PA_SIMU_CFG_MODEM_ROOT  "/simulation/modem"

/* Info */

#define PA_SIMU_INFO_DEFAULT_IMEI           "314159265358979"
#define PA_SIMU_INFO_DEFAULT_FW_VERSION     "Firmware 1.00"
#define PA_SIMU_INFO_DEFAULT_BOOT_VERSION   "Bootloader 1.00"

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

#define PA_SIMU_SIM_DEFAULT_MCC     "01"
#define PA_SIMU_SIM_DEFAULT_MNC     "001"



/* SMS */

#define PA_SIMU_SMS_DEFAULT_SMSC    ""

le_result_t sms_simu_Init
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

/* Temperature */

#define PA_SIMU_TEMP_DEFAULT_RADIO_TEMP              29
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_TEMP           32

#define PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_WARN        110
#define PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_CRIT        140

#define PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_WARN      -40
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_WARN      85
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_CRIT      -45
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_CRIT     130

#endif /* PA_SIMU_INCLUDE_GUARD */
