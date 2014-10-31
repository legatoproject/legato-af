/**
 * @file mdmCfgEntries.h
 *
 * Configuration Tree for ModemServices
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
#define LEGATO_MDMCFGENTRIES_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of path strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PATH_LEN              512
#define LIMIT_MAX_PATH_BYTES            (LIMIT_MAX_PATH_LEN + 1)


#define LEGATO_CONFIG_TREE_ROOT_DIR  "system:"

#define CFG_NODE_MODEMSERVICE       "modemServices"

#define CFG_MODEMSERVICE_PATH       LEGATO_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_MODEMSERVICE

#define CFG_NODE_SIM                "sim"
#define CFG_NODE_PIN                "pin"

#define CFG_MODEMSERVICE_SIM_PATH   CFG_MODEMSERVICE_PATH"/"CFG_NODE_SIM

#define CFG_NODE_MRC                "radio"
#define CFG_NODE_PREF_OPERATORS     "preferredOperators"
#define CFG_NODE_MCC                "mcc"
#define CFG_NODE_MNC                "mnc"
#define CFG_NODE_RAT                "rat"
#define CFG_NODE_SCANMODE           "scanMode"
#define CFG_NODE_MANUAL             "manual"
#define CFG_NODE_MRC_PREFS          "preferences"
#define CFG_NODE_BAND               "band"
#define CFG_NODE_LTE_BAND           "lteBand"
#define CFG_NODE_TDSCDMA_BAND       "tdScdmaBand"

#define CFG_MODEMSERVICE_MRC_PATH               CFG_MODEMSERVICE_PATH"/"CFG_NODE_MRC
#define CFG_MODEMSERVICE_MRC_PREFS_PATH         CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_MRC_PREFS
#define CFG_MODEMSERVICE_MRC_RAT_PATH           CFG_MODEMSERVICE_MRC_PREFS_PATH"/"CFG_NODE_RAT
#define CFG_MODEMSERVICE_MRC_BAND_PATH          CFG_MODEMSERVICE_MRC_PREFS_PATH"/"CFG_NODE_BAND
#define CFG_MODEMSERVICE_MRC_LTE_BAND_PATH      CFG_MODEMSERVICE_MRC_PREFS_PATH"/"CFG_NODE_LTE_BAND
#define CFG_MODEMSERVICE_MRC_TDSCDMA_BAND_PATH  CFG_MODEMSERVICE_MRC_PREFS_PATH"/"CFG_NODE_TDSCDMA_BAND

#define CFG_NODE_ECALL                "eCall"
#define CFG_NODE_PSAP                 "psap"
#define CFG_NODE_PUSHPULL             "pushPull"
#define CFG_NODE_MAX_REDIAL_ATTEMPTS  "maxRedialAttempts"
#define CFG_NODE_MSDVERSION           "msdVersion"
#define CFG_NODE_VEH                  "vehicleType"
#define CFG_NODE_VIN                  "vin"
#define CFG_NODE_PROP                 "propulsionType"

#define CFG_MODEMSERVICE_ECALL_PATH CFG_MODEMSERVICE_PATH"/"CFG_NODE_ECALL
#endif // LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
