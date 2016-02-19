/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * Common definitions potentially used across multiple .api files
 */

#ifndef COMMON_H_INCLUDE_GUARD
#define COMMON_H_INCLUDE_GUARD


#include "legato.h"

// Interface specific includes
#include "defn_interface.h"



//--------------------------------------------------------------------------------------------------
/**
 * Definition example
 */
//--------------------------------------------------------------------------------------------------
#define COMMON_FOUR 4


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
    COMMON_ZERO,
        ///< first enum

    COMMON_ONE,
        ///< second enum

    COMMON_TWO,
        ///< third enum

    COMMON_THREE
        ///< fourth enum
}
common_EnumExample_t;


//--------------------------------------------------------------------------------------------------
/**
 * BITMASK example
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    COMMON_BIT0 = 0x1,
        ///< first

    COMMON_BIT1 = 0x2,
        ///< second

    COMMON_BIT2 = 0x4
        ///< third
}
common_BitMaskExample_t;


#endif // COMMON_H_INCLUDE_GUARD

