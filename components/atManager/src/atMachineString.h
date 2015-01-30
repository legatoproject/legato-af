/** @file atMachineString.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINESTRING_INCLUDE_GUARD
#define LEGATO_ATMACHINESTRING_INCLUDE_GUARD

#include "legato.h"

#define     ATSTRING_SIZE       64

//--------------------------------------------------------------------------------------------------
/**
 * typedef of atmachinestring_t
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atstring {
    char            line[ATSTRING_SIZE];    ///< string value
    le_dls_Link_t   link;                   ///< link for list (intermediate, final or unsolicited)
} atmachinestring_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atstring
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinestring_Init
(
    void
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release all string list.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinestring_ReleaseFromList
(
    le_dls_List_t *pList
);

#endif /* LEGATO_ATMACHINESTRING_INCLUDE_GUARD */
