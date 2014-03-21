/**
 * @file cfgEntries.h
 *
 * Configuration Tree for ModemServices
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_CFGENTRIES_INCLUDE_GUARD
#define LEGATO_CFGENTRIES_INCLUDE_GUARD


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

#define CFG_MODEMSERVICE_MDC_PATH   CFG_MODEMSERVICE_PATH"/"CFG_NODE_MDC

#define CFG_NODE_SIM                "sim"
#define CFG_NODE_PIN                "pin"

#define CFG_MODEMSERVICE_SIM_PATH   CFG_MODEMSERVICE_PATH"/"CFG_NODE_SIM

#endif // LEGATO_CFGENTRIES_INCLUDE_GUARD
