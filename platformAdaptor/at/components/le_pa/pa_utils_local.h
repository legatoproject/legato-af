/** @file pa_utils_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAUTILSLOCAL_INCLUDE_GUARD
#define LEGATO_PAUTILSLOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Compare two strings
 */
//--------------------------------------------------------------------------------------------------
#define FIND_STRING(stringToFind, intoString) \
                   (strncmp(stringToFind, intoString, sizeof(stringToFind)-1)==0)

//--------------------------------------------------------------------------------------------------
/**
 * Default Timeout for AT Command.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_AT_CMD_TIMEOUT          30000

//--------------------------------------------------------------------------------------------------
/**
 * Default Expected AT Command Response.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_AT_RESPONSE             "OK|ERROR|+CME ERROR:"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to count the number of parameters in a line, between ',' and ':' and to set
 * all ',' with '\0' and the new char after ':' to '\0'
 *
 * @return the number of parameter in the line
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint32_t pa_utils_CountAndIsolateLineParameters
(
    char* linePtr       ///< [IN/OUT] line to parse
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to isolate a line parameter
 *
 * @return pointer to the isolate string in the line
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED char* pa_utils_IsolateLineParameter
(
    const char* linePtr,    ///< [IN] Line to read
    uint32_t    pos         ///< [IN] Position to read
);


#endif // LEGATO_PAUTILSLOCAL_INCLUDE_GUARD
