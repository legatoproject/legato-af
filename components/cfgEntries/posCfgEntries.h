/**
 * @file posCfgEntries.h
 *
 * Configuration Tree for Positioning
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_POSCFGENTRIES_INCLUDE_GUARD
#define LEGATO_POSCFGENTRIES_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of path strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PATH_LEN              512
#define LIMIT_MAX_PATH_BYTES            (LIMIT_MAX_PATH_LEN + 1)


#define LEGATO_CONFIG_TREE_ROOT_DIR "system:"

#define CFG_NODE_POSITIONING        "positioning"

#define CFG_NODE_RATE               "acquisitionRate"

#define CFG_POSITIONING_PATH        LEGATO_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_POSITIONING
#define CFG_POSITIONING_RATE_PATH   CFG_POSITIONING_PATH"/"CFG_NODE_RATE

#endif // LEGATO_POSCFGENTRIES_INCLUDE_GUARD
