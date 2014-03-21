
#ifndef CONFIG_TYPES_H_INCLUDE_GUARD
#define CONFIG_TYPES_H_INCLUDE_GUARD




#include "legato.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Opaque iterator reference type.
 */
// -------------------------------------------------------------------------------------------------
typedef struct le_cfg_iterator* le_cfg_iteratorRef_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Iterator status types.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    LE_CFG_ACTION_STEM_DELETED = 0x01,
    LE_CFG_ACTION_STEM_CREATED = 0x02,
    LE_CFG_ACTION_LEAF_CREATED = 0x04,
    LE_CFG_ACTION_LEAF_UPDATED = 0x08,
    LE_CFG_ACTION_LEAF_DELETED = 0x10
}
le_cfg_nodeAction_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Node data types.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    LE_CFG_TYPE_EMPTY,
    LE_CFG_TYPE_STRING,
    LE_CFG_TYPE_BOOL,
    LE_CFG_TYPE_INT,      // Signed 32-bit.
    LE_CFG_TYPE_FLOAT,    // 32-bit floating point value.
    LE_CFG_TYPE_STEM,     // Non-leaf node.
    LE_CFG_TYPE_DENIED,   // Access denied.
}
le_cfg_nodeType_t;





// -------------------------------------------------------------------------------------------------
/**
 *  Call to initialize the configAPI library.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_Initialize
(
);




#endif
