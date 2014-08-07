//--------------------------------------------------------------------------------------------------
/** @file supervisor/config.c
 *
 * A temporary API to access the configuration tree.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * The Legato config tree's root.
 */
//--------------------------------------------------------------------------------------------------
#define CONFIG_TREE_ROOT    "/tmp/LegatoConfigTree/"


//--------------------------------------------------------------------------------------------------
/**
 * The maximum number of values per node.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_VALUES      50


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for value strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ValueStrPool;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for value lists.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ValueListPool;


//--------------------------------------------------------------------------------------------------
/**
 * Line feed character.
 */
//--------------------------------------------------------------------------------------------------
#define LF_CHAR     10


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the configuration tree.
 */
//--------------------------------------------------------------------------------------------------
void cfg_Init
(
    void
)
{
    ValueListPool = le_mem_CreatePool("ConfigLists", (sizeof(char*)) * MAX_NUM_VALUES);
    ValueStrPool = le_mem_CreatePool("ConfigStrs", LIMIT_MAX_PATH_BYTES);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of values for the specified node.  The last value in the list is set to NULL.
 * The returned list must be freed by calling cfg_Free when it is no longer needed.
 *
 * @return
 *      The list of values.  Each value is a string.
 *      NULL if the node could not be found.
 *
 * @todo This is currently not thread safe.  Should fix this.
 */
//--------------------------------------------------------------------------------------------------
char** cfg_Get
(
    const char* nodePath    ///< [IN] The full path of the node.
)
{
    LE_INFO("Reading config tree node '%s'.", nodePath);

    // Prepend the config tree root to the node path.
    char fullNodePath[LIMIT_MAX_PATH_BYTES] = CONFIG_TREE_ROOT;

    if (nodePath[0] == '/')
    {
        if (le_utf8_Append(fullNodePath, nodePath + 1, LIMIT_MAX_PATH_BYTES, NULL) != LE_OK)
        {
            return NULL;
        }
    }
    else
    {
        if (le_utf8_Append(fullNodePath, nodePath, LIMIT_MAX_PATH_BYTES, NULL) != LE_OK)
        {
            return NULL;
        }
    }

    // Attempt to read the node.
    FILE* nodeFilePtr = fopen(fullNodePath, "r");

    if (nodeFilePtr == NULL)
    {
        return NULL;
    }

    // Get a value list object.
    char** valueListPtr = le_mem_ForceAlloc(ValueListPool);

    // Read each line in the file.
    size_t numLines = 0;
    char* valueStrPtr = le_mem_ForceAlloc(ValueStrPool);

    while ( (fgets(valueStrPtr, LIMIT_MAX_PATH_BYTES, nodeFilePtr) != NULL) &&
            (numLines < MAX_NUM_VALUES) )
    {
        // Strip the line feed.
        int endIndex = le_utf8_NumBytes(valueStrPtr) - 1;

        if (valueStrPtr[endIndex] == LF_CHAR)
        {
            valueStrPtr[endIndex] = '\0';
        }

        // Put the value string in the value list.
        valueListPtr[numLines++] = valueStrPtr;

        valueStrPtr = le_mem_ForceAlloc(ValueStrPool);
    }

    // The last value string from the pool is never used and should be released here.
    le_mem_Release(valueStrPtr);

    // Set the last entry in the list to NULL.
    valueListPtr[numLines] = NULL;

    fclose(nodeFilePtr);

    return valueListPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of values for the specified node.  The last value in the list is set to NULL.
 * The returned list must be freed by calling cfg_Free when it is no longer needed.
 *
 * @return
 *      The list of values.  Each value is a string.
 *      NULL if the node could not be found.
 *
 * @todo This is currently not thread safe.  Should fix this.
 */
//--------------------------------------------------------------------------------------------------
char** cfg_GetRelative
(
    const char* rootPath,   ///< [IN] The root path of the node.
    const char* nodePath    ///< [IN] The node path relative to the root path.
)
{
    char fullPath[LIMIT_MAX_PATH_BYTES];
    size_t rootLen = le_utf8_NumBytes(rootPath);

    // Create the full path.
    if ( (rootPath[rootLen-1] != '/') && (nodePath[0] != '/') )
    {
        if (snprintf(fullPath, LIMIT_MAX_PATH_BYTES, "%s/%s", rootPath, nodePath) >= LIMIT_MAX_PATH_BYTES)
        {
            return NULL;
        }
    }
    else if ( (fullPath[rootLen-1] == '/') && (nodePath[0] == '/') )
    {
        if (snprintf(fullPath, LIMIT_MAX_PATH_BYTES, "%s%s", rootPath, nodePath + 1) >= LIMIT_MAX_PATH_BYTES)
        {
            return NULL;
        }
    }
    else
    {
        if (snprintf(fullPath, LIMIT_MAX_PATH_BYTES, "%s%s", rootPath, nodePath) >= LIMIT_MAX_PATH_BYTES)
        {
            return NULL;
        }
    }

    return cfg_Get(fullPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Releases the resources for a value list.
 */
//--------------------------------------------------------------------------------------------------
void cfg_Free
(
    char** valueListPtr   ///< [IN] The list to release.
)
{
    // Release each value string in the list.
    size_t i = 0;

    while (valueListPtr[i] != NULL)
    {
        le_mem_Release(valueListPtr[i]);
        i++;
    }

    // Release the list.
    le_mem_Release(valueListPtr);
}

