
// -------------------------------------------------------------------------------------------------
/**
 *  @file configTreeAdminApi.c
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Get an iterator pointer from an iterator reference.
 */
// -------------------------------------------------------------------------------------------------
static ni_IteratorRef_t GetIteratorFromRef
(
    le_cfg_IteratorRef_t externalRef  ///< The iterator reference to extract a pointer from.
)
// -------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_InternalRefFromExternalRef(tu_GetCurrentConfigAdminUserInfo(),
                                                                 externalRef);

    if (iteratorRef == NULL)
    {
        tu_TerminateClient(le_cfgAdmin_GetClientSessionRef(), "Bad iterator reference.");
    }

    return iteratorRef;
}




// -------------------------------------------------------------------------------------------------
//  Import and export of the tree data.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Read a subset of the configuration tree from the given filePath.  That tree then overwrites the
 *  node at the given nodePath.
 *
 *  This function will import a sub-tree as part of the iterator's current transaction.  This allows
 *  you to create an iterator on a given node.  Import a sub-tree, and then examine the contents of
 *  the import before deciding to commit the new data.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_NOT_PERMITTED - Attempted to import to a section of the tree the connection doesn't
 *                               have access to.
 *          - LE_FAULT         - An I/O error occured while reading the data.
 *          - LE_FORMAT_ERROR  - The configuration data being imported appears corrupted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ImportTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< Reference used to generate a reply for this
                                            ///<   request.
    le_cfg_IteratorRef_t externalRef,       ///< The write iterator that is being used for the
                                            ///<   import.
    const char* filePathPtr,                ///< Import the tree data from the this file.
    const char* nodePathPtr                 ///< Where in the tree should this import happen?  Leave
                                            ///<   as an empty string to use the iterator's current
                                            ///<   node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Importing a tree from <%s> onto node <%s>, using iterator, <%p>.",
             filePathPtr, nodePathPtr, externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (iteratorRef == NULL)
    {
        le_cfgAdmin_ImportTreeRespond(commandRef, LE_OK);
        return;
    }

    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, nodePathPtr);

    if (nodeRef == NULL)
    {
        le_cfgAdmin_ImportTreeRespond(commandRef, LE_NOT_FOUND);
    }
    else
    {
        // Open the requested file.
        LE_DEBUG("Opening file <%s>.", filePathPtr);

        int fid = -1;

        do
        {
            fid = open(filePathPtr, O_RDONLY);
        }
        while ((fid == -1) && (errno == EINTR));

        if (fid == -1)
        {
            LE_ERROR("File <%s> could not be opened.", filePathPtr);
            le_cfgAdmin_ImportTreeRespond(commandRef, LE_FAULT);

            return;
        }

        // Now, attempt to import the requested data.
        LE_DEBUG("Importing config data.");

        le_result_t result;

        if (tdb_ReadTreeNode(nodeRef, fid))
        {
            result = LE_OK;
        }
        else
        {
            result = LE_FORMAT_ERROR;
        }

        // Let the caller know we're done.
        le_cfgAdmin_ImportTreeRespond(commandRef, result);

        // Close up the file and we're done.
        int retVal = -1;

        do
        {
            retVal = close(fid);
        }
        while ((retVal == -1) && (errno == EINTR));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Take a node given from nodePath and stream it and it's children to the file given by filePath.
 *
 *  This funciton uses the iterator's read transaction, and takes a snapshot of the current state of
 *  the tree.  The data write happens immediately.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - The specified path does not exist in the config tree.
 *          - LE_NOT_PERMITTED - Attempted to export from a section of the tree the connection
 *                               doesn't have access to.
 *          - LE_FAULT         - An I/O error occured while writing the data.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ExportTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< Reference used to generate a reply for this
                                            ///<   request.
    le_cfg_IteratorRef_t externalRef,       ///< The write iterator that is being used for the
                                            ///<   export.
    const char* filePathPtr,                ///< Import the tree data from the this file.
    const char* nodePathPtr                 ///< Where in the tree should this export happen?  Leave
                                            ///<   as an empty string to use the iterator's current
                                            ///<   node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Exporting a tree from node <%s> into file <%s>, using iterator, <%p>.",
             nodePathPtr, filePathPtr, externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (iteratorRef == NULL)
    {
        le_cfgAdmin_ExportTreeRespond(commandRef, LE_OK);
        return;
    }

    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, nodePathPtr);

    if (nodeRef == NULL)
    {
        le_cfgAdmin_ExportTreeRespond(commandRef, LE_NOT_FOUND);
    }
    else
    {
        LE_DEBUG("Opening file <%s>.", filePathPtr);

        int fid = -1;

        do
        {
            fid = open(filePathPtr, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        }
        while ((fid == -1) && (errno == EINTR));


        if (fid == -1)
        {
            LE_ERROR("File <%s> could not be opened.", filePathPtr);
            le_cfgAdmin_ExportTreeRespond(commandRef, LE_FAULT);

            return;
        }


        LE_DEBUG("Importing config data.");

        tdb_WriteTreeNode(nodeRef, fid);
        le_cfgAdmin_ExportTreeRespond(commandRef, LE_OK);

        int retVal = -1;

        do
        {
            retVal = close(fid);
        }
        while ((retVal == -1) && (errno == EINTR));
    }
}




// -------------------------------------------------------------------------------------------------
//  Listing configuration trees.
// -------------------------------------------------------------------------------------------------




/// Ref to the treeDb's hash of trees iterator.
le_hashmap_It_Ref_t TreeIterRef = NULL;




// -------------------------------------------------------------------------------------------------
/**
 *  Read the name of the tree currently pointed at by the iterator.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *    - LE_OK if there is enough room to copy the name into the supplied buffer.
 *    - LE_OVERFLOW if not.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_GetTreeName
(
    le_cfgAdmin_ServerCmdRef_t commandRef,      ///< Reference used to generate a reply for this
                                                ///<   request.
    size_t maxNameLength                        ///< Size of the return buffer.
)
// -------------------------------------------------------------------------------------------------
{
    // Are we in the middle of an iteration?  If not, start one now.
    if (TreeIterRef == NULL)
    {
        TreeIterRef = tdb_GetTreeIterRef();
    }

    // Get the current tree name.  Let the caller know if it'll fit.
    char* namePtr = (char*)le_hashmap_GetKey(TreeIterRef);
    le_result_t result = LE_OK;

    if (strlen(namePtr) >= maxNameLength)
    {
        result = LE_OVERFLOW;
    }

    // Let the caller know what happened.
    le_cfgAdmin_GetTreeNameRespond(commandRef, result, namePtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move onto the next tree in the list.  If there are no more trees this function returns false,
 *  otherwise true is returned.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *    - LE_OK if there are more trees to iterate through.
 *    - LE_NOT_FOUND if not.
 *    - LE_FAULT if the tree collection was changed in the middle of an iterator.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_NextTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef  ///< Reference used to generate a reply for this request.
)
// -------------------------------------------------------------------------------------------------
{
    // If we haven't started an iteration yet, start one now.
    if (TreeIterRef == NULL)
    {
        TreeIterRef = tdb_GetTreeIterRef();
        le_cfgAdmin_NextTreeRespond(commandRef, LE_OK);

        return;
    }

    // Looks like we're in the middle of an iteration.  So, let's continue.
    le_result_t result = le_hashmap_NextNode(TreeIterRef);

    if (result != LE_OK)
    {
        TreeIterRef = NULL;
    }

    le_cfgAdmin_NextTreeRespond(commandRef, result);
}
