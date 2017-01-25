/**
 * @file posCfgEntries.h
 *
 * Configuration Tree for Positioning
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_POSCFGENTRIES_INCLUDE_GUARD
#define LEGATO_POSCFGENTRIES_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Paths to positioning data in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define POSSERVICE_CONFIG_TREE_ROOT_DIR  "positioningService:"

#define CFG_NODE_POSITIONING        "positioning"
#define CFG_POSITIONING_PATH        POSSERVICE_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_POSITIONING

#define CFG_NODE_RATE               "acquisitionRate"
#define CFG_POSITIONING_RATE_PATH   CFG_POSITIONING_PATH"/"CFG_NODE_RATE

#endif // LEGATO_POSCFGENTRIES_INCLUDE_GUARD
