/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 *  @page api_configAdmin Configuration Tree Administration API
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_CFGADMIN_INTERFACE_H_INCLUDE_GUARD
#define LE_CFGADMIN_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "configAdminTypes.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_cfgAdmin_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_cfgAdmin_StopClient
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a subset of the configuration tree from the given filePath.  That tree then overwrites the
 *  node at the given nodePath.
 *
 *  This function will import a sub-tree as part of the iterator's current transaction.  This allows
 *  you to create an iterator on a given node.  Import a sub-tree, and then examine the contents of
 *  the import before deciding to commit the new data.
 *
 *  @return This function will return one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_NOT_PERMITTED - Attempted to import to a section of the tree the connection doesn't
 *                               have access to.
 *          - LE_FAULT         - An I/O error occured while reading the data.
 *          - LE_FORMAT_ERROR  - The configuration data being imported appears corrupted.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfgAdmin_ImportTree
(
    le_cfg_iteratorRef_t iteratorRef,
        ///< [IN]
        ///< The write iterator that is being used for the import.

    const char* filePath,
        ///< [IN]
        ///< Import the tree data from the this file.

    const char* nodePath
        ///< [IN]
        ///< Where in the tree should this import happen?  Leave
        ///<   as an empty string to use the iterator's current
        ///<   node.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Take a node given from nodePath and stream it and it's children to the file given by filePath.
 *
 *  This funciton uses the iterator's read transaction, and takes a snapshot of the current state of
 *  the tree.  The data write happens immediately.
 *
 *  @return This function will return one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - The specified path does not exist in the config tree.
 *          - LE_NOT_PERMITTED - Attempted to export from a section of the tree the connection
 *                               doesn't have access to.
 *          - LE_FAULT         - An I/O error occured while writing the data.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfgAdmin_ExportTree
(
    le_cfg_iteratorRef_t iteratorRef,
        ///< [IN]
        ///< The write iterator that is being used for the export.

    const char* filePath,
        ///< [IN]
        ///< Import the tree data from the this file.

    const char* nodePath
        ///< [IN]
        ///< Where in the tree should this export happen?  Leave
        ///<   as an empty string to use the iterator's current
        ///<   node.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Create an iterator that can list all of the trees registered with the system.
 */
//--------------------------------------------------------------------------------------------------
le_cfgAdmin_treeIteratorRef_t le_cfgAdmin_CreateTreeIterator
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 *  Call this function when you are done with the iterator.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgAdmin_DeleteTreeIterator
(
    le_cfgAdmin_treeIteratorRef_t iteratorRef
        ///< [IN]
        ///< The iterator object to dispose of.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read the name of the tree currently pointed at by the iterator.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgAdmin_GetTreeName
(
    le_cfgAdmin_treeIteratorRef_t iteratorRef,
        ///< [IN]
        ///< The iterator object to read.

    char* name,
        ///< [OUT]
        ///< The name of the currently referenced tree is
        ///<   passed in this out parameter.

    size_t nameNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 *  Move onto the next tree in the list.  If there are no more trees this function returns false,
 *  otherwise true is returned.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfgAdmin_NextTree
(
    le_cfgAdmin_treeIteratorRef_t iteratorRef
        ///< [IN]
        ///< The iterator object to increment.
);


#endif // LE_CFGADMIN_INTERFACE_H_INCLUDE_GUARD

