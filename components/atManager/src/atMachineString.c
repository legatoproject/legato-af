/** @file atmachinestring.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "atMachineString.h"
#include "../inc/atCmdSync.h"

#define DEFAULT_ATSTRING_POOL_SIZE      1
static le_mem_PoolRef_t    AtStringPool;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atstring
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinestring_Init
(
    void
)
{
    AtStringPool = le_mem_CreatePool("AtStringPool",sizeof(atmachinestring_t));
    le_mem_ExpandPool(AtStringPool,DEFAULT_ATSTRING_POOL_SIZE);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add new string into a le_dls_List_t.
 *
 * @note
 * pList must be finished by a NULL.
 *
 * @return nothing
 */
//--------------------------------------------------------------------------------------------------
void atmachinestring_AddInList
(
    le_dls_List_t *list,          ///< List of atmachinestring_t
    const char   **patternListPtr ///< List of pattern
)
{
    uint32_t i = 0;

    if (!patternListPtr) {
        return;
    }

    while(patternListPtr[i] != NULL) {
        atmachinestring_t* newStringPtr = le_mem_ForceAlloc(AtStringPool);

        LE_FATAL_IF(
            (strlen(patternListPtr[i])>ATSTRING_SIZE),
                    "%s is too long (%zd): Max size %d",
                    patternListPtr[i],strlen(patternListPtr[i]),ATSTRING_SIZE);

        strncpy(newStringPtr->line,patternListPtr[i],ATSTRING_SIZE);
        newStringPtr->line[ATSTRING_SIZE-1]='\0';

        newStringPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(list,&(newStringPtr->link));
        i++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release all string list.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinestring_ReleaseFromList
(
    le_dls_List_t *pList
)
{
    le_dls_Link_t* linkPtr;

    while ((linkPtr=le_dls_Pop(pList)) != NULL)
    {
        atmachinestring_t * currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);

        le_mem_Release(currentPtr);
    }
    LE_DEBUG("All string has been released");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to count the number of paramter between ',' and ':' and to set
 * all ',' with '\0' and the new char after ':' to '\0'
 *
 * @return true if the line start with '+', or false
 */
//--------------------------------------------------------------------------------------------------
uint32_t atcmd_CountLineParameter
(
    char        *linePtr       ///< [IN/OUT] line to parse
)
{
    uint32_t  cpt=1;
    uint32_t  lineSize = strlen(linePtr);

    if ( lineSize ) {
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
 * This function must be called to count the get the @ref pos parameter of Line
 *
 *
 * @return pointer to the string at pos position in Line
 */
//--------------------------------------------------------------------------------------------------
char* atcmd_GetLineParameter
(
    const char* linePtr,   ///< [IN] Line to read
    uint32_t    pos     ///< [IN] Position to read
)
{
    uint32_t i;
    char* pCurr = (char*)linePtr;

    for(i=1;i<pos;i++) {
        pCurr=pCurr+strlen(pCurr)+1;
    }

    return pCurr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to copy string from inBuffer to outBuffer without '"'
 *
 *
 * @return number of char really copy
 */
//--------------------------------------------------------------------------------------------------
uint32_t atcmd_CopyStringWithoutQuote
(
    char*       outBufferPtr,  ///< [OUT] final buffer
    const char* inBufferPtr,   ///< [IN] initial buffer
    uint32_t    inBufferSize ///< [IN] Max char to copy from inBuffer to outBuffer
)
{
    uint32_t i,idx;

    for(i=0,idx=0;i<inBufferSize;i++)
    {
        if (inBufferPtr[i]=='\0') {
            break;
        }

        if (inBufferPtr[i] != '"') {
            outBufferPtr[idx++]=inBufferPtr[i];
        }
    }
    outBufferPtr[idx]='\0';

    return idx;
}
