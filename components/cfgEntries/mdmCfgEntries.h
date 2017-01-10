/**
 * @file mdmCfgEntries.h
 *
 * Configuration Tree for ModemServices
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
#define LEGATO_MDMCFGENTRIES_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Paths to eCall data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define MODEMSERVICE_CONFIG_TREE_ROOT_DIR  "modemService:"

#define CFG_NODE_ECALL                      "eCall"
#define CFG_MODEMSERVICE_ECALL_PATH         MODEMSERVICE_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_ECALL
#define CFG_NODE_SYSTEM_STD                 "systemStandard"
#define CFG_NODE_MSDVERSION                 "msdVersion"
#define CFG_NODE_VEH                        "vehicleType"
#define CFG_NODE_VIN                        "vin"
#define CFG_NODE_PROP                       "propulsionType"

#endif // LEGATO_MDMCFGENTRIES_INCLUDE_GUARD
