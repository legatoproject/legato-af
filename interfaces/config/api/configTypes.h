
#ifndef CONFIG_TYPES_H_INCLUDE_GUARD
#define CONFIG_TYPES_H_INCLUDE_GUARD




// -------------------------------------------------------------------------------------------------
/**
 *  Opaque iterator reference type.
 */
// -------------------------------------------------------------------------------------------------
typedef struct le_cfg_Iterator* le_cfg_IteratorRef_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Node data types.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    LE_CFG_TYPE_EMPTY,        ///< A node with no value.
    LE_CFG_TYPE_STRING,       ///< A string encoded as utf8.
    LE_CFG_TYPE_BOOL,         ///< Boolean value.
    LE_CFG_TYPE_INT,          ///< Signed 32-bit.
    LE_CFG_TYPE_FLOAT,        ///< 64-bit floating point value.
    LE_CFG_TYPE_STEM,         ///< Non-leaf node, this node is the parent of other nodes.
    LE_CFG_TYPE_DOESNT_EXIST  ///< Node doesn't exist.
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
