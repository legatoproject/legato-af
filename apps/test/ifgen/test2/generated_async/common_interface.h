/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * Common definitions potentially across multiple .api files
 */

#ifndef COMMON_INTERFACE_H_INCLUDE_GUARD
#define COMMON_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Definition example
 */
//--------------------------------------------------------------------------------------------------
#define COMMON_SOME_VALUE 5


//--------------------------------------------------------------------------------------------------
/**
 * Example of using previously DEFINEd symbol within an imported file.
 */
//--------------------------------------------------------------------------------------------------
#define COMMON_TEN 10


//--------------------------------------------------------------------------------------------------
/**
 * Reference example
 */
//--------------------------------------------------------------------------------------------------
typedef struct common_OpaqueReference* common_OpaqueReferenceRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * ENUM example
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    COMMON_ZERO = 0,
        ///< first enum

    COMMON_ONE = 1,
        ///< second enum

    COMMON_TWO = 20,
        ///< third enum

    COMMON_THREE = 21
        ///< fourth enum
}
common_EnumExample_t;


#endif // COMMON_INTERFACE_H_INCLUDE_GUARD

