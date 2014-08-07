#ifndef LEGATO_MDMDEFS_INCLUDE_GUARD
#define LEGATO_MDMDEFS_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Cf. ITU-T recommendations E.164/E.163. E.164 numbers can have a maximum of 15 digits except the
 * international prefix ('+' or '00'). One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_MDMDEFS_PHONE_NUM_MAX_LEN      (15+2+1)

//--------------------------------------------------------------------------------------------------
/**
*  IP Version
*/
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_IPV4,    ///< IPv4 technology
    LE_IPV6,    ///< IPv6 technology
}
le_mdmDefs_IpVersion_t;

#endif // LEGATO_MDMDEFS_INCLUDE_GUARD
