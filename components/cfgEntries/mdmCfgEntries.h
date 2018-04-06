/**
 * @file mdmCfgEntries.h
 *
 * Configuration Tree for ModemServices
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
#define LEGATO_MDMCFGENTRIES_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Paths to modemService data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define MODEMSERVICE_CONFIG_TREE_ROOT_DIR  "modemService:"

//--------------------------------------------------------------------------------------------------
/**
 * Paths to eCall data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_ECALL                      "eCall"
#define CFG_MODEMSERVICE_ECALL_PATH         MODEMSERVICE_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_ECALL
#define CFG_NODE_SYSTEM_STD                 "systemStandard"
#define CFG_NODE_MSDVERSION                 "msdVersion"
#define CFG_NODE_VEH                        "vehicleType"
#define CFG_NODE_VIN                        "vin"
#define CFG_NODE_PROP                       "propulsionType"

//--------------------------------------------------------------------------------------------------
/**
 * Paths to SMS data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SMS                        "sms"
#define CFG_MODEMSERVICE_SMS_PATH           MODEMSERVICE_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_SMS
#define CFG_NODE_COUNTING                   "counting"
#define CFG_NODE_RX_COUNT                   "rxCount"
#define CFG_NODE_TX_COUNT                   "txCount"
#define CFG_NODE_RX_CB_COUNT                "rxCbCount"
#define CFG_NODE_STATUS_REPORT              "statusReportEnabled"

//--------------------------------------------------------------------------------------------------
/**
 * Paths to MDC data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_MDC                        "mdc"
#define CFG_MODEMSERVICE_MDC_PATH           MODEMSERVICE_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_MDC
#define CFG_NODE_RX_BYTES                   "rxBytes"
#define CFG_NODE_TX_BYTES                   "txBytes"

//--------------------------------------------------------------------------------------------------
/**
 * Paths to SIM data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SIM                        "sim"
#define CFG_MODEMSERVICE_SIM_PATH           MODEMSERVICE_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_SIM
#define CFG_NODE_ICCID                      "iccid"


#endif // LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
