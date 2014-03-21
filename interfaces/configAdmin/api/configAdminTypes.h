
#ifndef CONFIG_ADMIN_TYPES_H_INCLUDE_GUARD
#define CONFIG_ADMIN_TYPES_H_INCLUDE_GUARD




// We need the regular config types as well.
#include "configTypes.h"




// -------------------------------------------------------------------------------------------------
/**
 *  .
 */
// -------------------------------------------------------------------------------------------------
typedef struct le_cfgAdmin_treeIterator* le_cfgAdmin_treeIteratorRef_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Call to initialize the configAPI library.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_Initialize
(
    void
);




#endif
