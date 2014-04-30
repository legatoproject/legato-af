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


#define LEGATO_CONFIG_TREE_ROOT_DIR  ""

#define CFG_NODE_MODEMSERVICE       "modemServices"

#define CFG_MODEMSERVICE_PATH       LEGATO_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_MODEMSERVICE

#define CFG_NODE_MDC                "modemDataConnection"
#define CFG_NODE_APN                "accessPointName"
#define CFG_NODE_PDP                "packetDataProtocol"
#define CFG_NODE_AUTH               "authentication"
#define CFG_NODE_PAP                "pap"
#define CFG_NODE_CHAP               "chap"
#define CFG_NODE_ENABLE             "enable"
#define CFG_NODE_USER               "userName"
#define CFG_NODE_PWD                "password"

#define CFG_MODEMSERVICE_MDC_PATH   CFG_MODEMSERVICE_PATH"/"CFG_NODE_MDC

#define CFG_NODE_SIM                "sim"
#define CFG_NODE_PIN                "pin"

#define CFG_MODEMSERVICE_SIM_PATH   CFG_MODEMSERVICE_PATH"/"CFG_NODE_SIM

#define CFG_NODE_MRC                "radioControl"
#define CFG_NODE_PREFERREDLIST      "preferredList"
#define CFG_NODE_MCC                "mcc"
#define CFG_NODE_MNC                "mnc"
#define CFG_NODE_RAT                "rat"
#define CFG_NODE_SCANMODE           "scanMode"
#define CFG_NODE_MANUAL             "manual"



#define CFG_MODEMSERVICE_MRC_PATH   CFG_MODEMSERVICE_PATH"/"CFG_NODE_MRC


#endif // LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
