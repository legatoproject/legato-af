
// -------------------------------------------------------------------------------------------------
/**
 *  @file configTreeAdminApi.c
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "treePath.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "treeIterator.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Get an iterator pointer from an iterator refereLIMIT_MAX_USER_NAME_LENnce.
 */
// -------------------------------------------------------------------------------------------------
static ni_IteratorRef_t GetIteratorFromRef
(
    le_cfg_IteratorRef_t externalRef  ///< [IN] Iterator reference to extract a pointer from.
)
// -------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_InternalRefFromExternalRef(tu_GetCurrentConfigAdminUserInfo(),
                                                                 externalRef);

    if (iteratorRef == NULL)
    {
        tu_TerminateConfigAdminClient(le_cfgAdmin_GetClientSessionRef(), "Bad iterator reference.");
    }

    return iteratorRef;
}




// -------------------------------------------------------------------------------------------------
//  Import and export of the tree data.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Read a subset of the configuration tree from the given filePath. That tree then overwrites the
 *  node at the given nodePath.
 *
 *  This function will import a sub-tree as part of the iterator's current transaction. This allows
 *  you to create an iterator on a given node. Import a sub-tree, and then examine the contents of
 *  the import before deciding to commit the new data.
 *
 *  \b Responds \b With:
 *
 *  Responds with one of the following values:
 *
 *          - LE_OK            - Commit was completed successfully.
 *          - LE_FAULT         - An I/O error occurred while reading the data.
 *          - LE_FORMAT_ERROR  - Configuration data being imported appears corrupted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ImportTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                            ///<      request.
    le_cfg_IteratorRef_t externalRef,       ///< [IN] Write iterator that is being used for the
                                            ///<      import.
    const char* filePathPtr,                ///< [IN] Import the tree data from the this file.
    const char* nodePathPtr                 ///< [IN] Where in the tree should this import happen?
                                            ///<      Leave as an empty string to use the iterator's
                                            ///<      current node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Importing a tree from '%s' onto node '%s', using iterator, '%p'.",
             filePathPtr, nodePathPtr, externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (iteratorRef == NULL)
    {
        le_cfgAdmin_ImportTreeRespond(commandRef, LE_OK);
        return;
    }

    tdb_NodeRef_t nodeRef = ni_TryCreateNode(iteratorRef, nodePathPtr);

    if (nodeRef == NULL)
    {
        le_cfgAdmin_ImportTreeRespond(commandRef, LE_NOT_FOUND);
    }
    else
    {
        // Open the requested file.
        LE_DEBUG("Opening file '%s'.", filePathPtr);

        FILE* filePtr = NULL;

        filePtr = fopen(filePathPtr, "r");

        if (!filePtr)
        {
            LE_ERROR("File '%s' could not be opened.", filePathPtr);
            le_cfgAdmin_ImportTreeRespond(commandRef, LE_FAULT);

            return;
        }

        // Now, attempt to import the requested data.
        LE_DEBUG("Importing config data.");

        le_result_t result;

        if (tdb_ReadTreeNode(nodeRef, filePtr))
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
        fclose(filePtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Take a node given from nodePath and stream it and it's children to the file given by filePath.
 *
 *  This function uses the iterator's read transaction, and takes a snapshot of the current state of
 *  the tree.  The data write happens immediately.
 *
 *  \b Responds \b With:
 *
 *  Responds with one of the following values:
 *
 *          - LE_OK            - Commit was completed successfully.
 *          - LE_FAULT         - An I/O error occurred while writing the data.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ExportTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                            ///<      request.
    le_cfg_IteratorRef_t externalRef,       ///< [IN] Write iterator that is being used for the
                                            ///<      export.
    const char* filePathPtr,                ///< [IN] Export the tree data to the this file.
    const char* nodePathPtr                 ///< [IN] Where in the tree should this export happen?
                                            ///<      Leave as an empty string to use the iterator's
                                            ///<      current node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Exporting a tree from node '%s' into file '%s', using iterator, '%p'.",
             nodePathPtr, filePathPtr, externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (iteratorRef == NULL)
    {
        le_cfgAdmin_ExportTreeRespond(commandRef, LE_OK);
        return;
    }

    LE_DEBUG("Opening file '%s'.", filePathPtr);

    FILE* filePtr = NULL;

    filePtr = fopen(filePathPtr, "w+");

    if (!filePtr)
    {
        LE_ERROR("File '%s' could not be opened.", filePathPtr);
        le_cfgAdmin_ExportTreeRespond(commandRef, LE_IO_ERROR);

        return;
    }


    LE_DEBUG("Exporting config data.");

    le_result_t result = LE_OK;

    if (tdb_WriteTreeNode(ni_GetNode(iteratorRef, nodePathPtr), filePtr) != LE_OK)
    {
        result = LE_FAULT;
    }

    fclose(filePtr);

    le_cfgAdmin_ExportTreeRespond(commandRef, result);
}




// -------------------------------------------------------------------------------------------------
//  Tree maintenance.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a tree from the system, both from the file system and from memory.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_DeleteTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                            ///<      request.
    const char* treeName                    ///< [IN] Name of the tree to delete.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_DeleteTree(tdb_GetTree(treeName));
    le_cfgAdmin_DeleteTreeRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
//  Iterating configuration trees.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new iterator object for iterating over the list of trees currently managed by the
 *  config tree daemon.
 *
 *  \b Responds \b With:
 *
 *  A newly created reference to a newly created tree iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_CreateTreeIterator
(
    le_cfgAdmin_ServerCmdRef_t commandRef  ///< [IN] Reference used to generate a reply for this
                                           ///<      request.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfgAdmin_CreateTreeIteratorRespond(commandRef,
                                          ti_CreateIterator(le_cfgAdmin_GetClientSessionRef()));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Release the iterator and free its memory back to the system.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ReleaseTreeIterator
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                            ///<      request.
    le_cfgAdmin_IteratorRef_t iteratorRef   ///< [IN] Iterator object to release.
)
// -------------------------------------------------------------------------------------------------
{
    le_msg_SessionRef_t sessionRef = le_cfgAdmin_GetClientSessionRef();
    ti_TreeIteratorRef_t internalRef = ti_InternalRefFromExternalRef(sessionRef, iteratorRef);

    if (internalRef != NULL)
    {
        ti_ReleaseIterator(internalRef);
    }

    le_cfgAdmin_ReleaseTreeIteratorRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the name of the tree currently pointed at by the iterator.
 *
 *  \b Responds \b With:
 *
 *  LE_OK if there is enough room to copy the name into the supplied buffer.  LE_OVERFLOW if
 *  not.  LE_NOT_FOUND is returned if the list is empty or if the iterator hasn't been moved
 *  onto the first item yet.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_GetTreeName
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                            ///<      request.
    le_cfgAdmin_IteratorRef_t iteratorRef,  ///< [IN] Iterator object to read.
    size_t nameSize                         ///< [IN] Size of the client's name buffer.
)
// -------------------------------------------------------------------------------------------------
{
    le_msg_SessionRef_t sessionRef = le_cfgAdmin_GetClientSessionRef();
    ti_TreeIteratorRef_t internalRef = ti_InternalRefFromExternalRef(sessionRef, iteratorRef);

    char treeName[MAX_TREE_NAME_BYTES] = "";
    le_result_t result = LE_OK;

    if (nameSize > MAX_TREE_NAME_BYTES)
    {
        nameSize = MAX_TREE_NAME_BYTES;
    }

    if (internalRef != NULL)
    {
        result = ti_GetCurrent(internalRef, treeName, nameSize);
    }

    le_cfgAdmin_GetTreeNameRespond(commandRef, result, treeName);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move onto the next tree in the list.  f there are no more trees, returns false,
 *  otherwise returns true.
 *
 *  \b Responds \b With:
 *
 *  LE_OK if there are more trees to iterate through.  LE_NOT_FOUND if not.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_NextTree
(
    le_cfgAdmin_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                            ///<      request.
    le_cfgAdmin_IteratorRef_t iteratorRef   ///< [IN] Iterator to iterate.
)
// -------------------------------------------------------------------------------------------------
{
    le_msg_SessionRef_t sessionRef = le_cfgAdmin_GetClientSessionRef();
    ti_TreeIteratorRef_t internalRef = ti_InternalRefFromExternalRef(sessionRef, iteratorRef);

    le_result_t result = LE_OK;

    if (internalRef != NULL)
    {
        result = ti_MoveNext(internalRef);
    }

    le_cfgAdmin_NextTreeRespond(commandRef, result);
}
