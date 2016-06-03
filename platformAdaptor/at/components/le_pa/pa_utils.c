/** @file pa_utils.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "le_atClient.h"
#include "pa_utils_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to count the number of parameters in a line,
 * between ',' and ':' and to set all ',' with '\0' and the new char after ':' to '\0'
 *
 * @return the number of parameters in the line
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_utils_CountAndIsolateLineParameters
(
    char* linePtr       ///< [IN/OUT] line to parse
)
{
    uint32_t cpt = 1;
    uint32_t lineSize = strlen(linePtr);

    if (lineSize) {
        while (lineSize)
        {
            if ( linePtr[lineSize] == ',' ) {
                linePtr[lineSize] = '\0';
                cpt++;
            } else if (linePtr[lineSize] == ':' ) {
                linePtr[lineSize+1] = '\0';
                cpt++;
            }

            lineSize--;
        }
        return cpt;
    }
    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to isolate a line parameter
 *
 * @return pointer to the isolated string in the line
 */
//--------------------------------------------------------------------------------------------------
char* pa_utils_IsolateLineParameter
(
    const char* linePtr,    ///< [IN] Line to read
    uint32_t    pos         ///< [IN] Position to read
)
{
    uint32_t i;
    char*    pCurr = (char*)linePtr;

    for(i=1;i<pos;i++)
    {
        pCurr=pCurr+strlen(pCurr)+1;
    }

    return pCurr;
}