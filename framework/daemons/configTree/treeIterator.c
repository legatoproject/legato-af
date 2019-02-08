
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeIterator.c
 *
 *  Implementation of the configTree's treeIterator object.  When a tree iterator is created it
 *  takes a snapshot of the trees in the system.
 *
 *  This is done to isolate the user of the iterator from the fact that trees can appear and
 *  disappear from the system at any time.  However, because the iteration will happen over several
 *  context switches between client and server the window for things changing during an iteration
 *  only increses.  So, to save some sanity, this infomration is snapshotted and served up from a
 *  cache.
 *
 *  The structure is fairly simple.  On creation the tree iterator creates a sorted linked list of
 *  the trees it can find that have been loaded, and then searches the file system for all tree
 *  files it can find in the configTree's storage dir.  As it finds these files, duplicates are
 *  discarded.  A future enhancement would be to also keep track of wether or not the tree was
 *  loaded at the time of iteration.
 *
@verbatim

  +------------------+
  |  Tree Iterator   |
  +------------------+
      |
      | Tree List   +--------------+
      +------------>|  Tree Item   |
                    +--------------+
                        |
                        |             +--------------+
                        +------------>|  Tree Item   |
                                      +--------------+
                                          |
                                          |             +--------------+
                                          +------------>|  Tree Item   |
                                                        +--------------+
@endverbatim
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "treePath.h"
#include "treeDb.h"
#include "treeIterator.h"
#include "sysPaths.h"


//--------------------------------------------------------------------------------------------------
/**
 *  Information for one of the trees in the system which may or may not be loaded in memory.
 */
//--------------------------------------------------------------------------------------------------
typedef struct TreeItem
{
    le_dls_Link_t link;                  ///< Link to the next tree in the list.
    char treeName[MAX_TREE_NAME_BYTES];  ///< Name of the tree in question.
}
TreeItem_t;




//--------------------------------------------------------------------------------------------------
/**
 *  The iterator object holds a list of trees and keeps track of it's session and safe ref.  It also
 *  keeps track of the iteration state.
 */
//--------------------------------------------------------------------------------------------------
typedef struct TreeIterator
{
    le_msg_SessionRef_t sessionRef;     ///< The session this iterator was created on.
    le_cfgAdmin_IteratorRef_t safeRef;  ///< The safe reference to this object that was given to the
                                        ///<   user.

    le_dls_List_t treeList;             ///< The list of loaded and unloaded trees found at the time
                                        ///<   of iterator creation.
    le_dls_Link_t* currentItemPtr;      ///< The current item we've iterated to.
}
TreeIterator_t;


/// Static pool for iterators
LE_MEM_DEFINE_STATIC_POOL(TreeIteratorPool, LE_CONFIG_CFGTREE_MAX_TREE_ITERATOR_POOL_SIZE,
    sizeof(TreeIterator_t));

/// Pool for allocating tree iterator objects.
static le_mem_PoolRef_t TreeIteratorPoolRef = NULL;

/// The static pool for handling tree iterator safe references.
LE_REF_DEFINE_STATIC_MAP(TreeIteratorMap, LE_CONFIG_CFGTREE_MAX_TREE_ITERATOR_POOL_SIZE);

/// The pool for handing tree iterator safe references.
static le_ref_MapRef_t IteratorRefMap = NULL;


/// Static pool for tree items
LE_MEM_DEFINE_STATIC_POOL(TreeItemPool, LE_CONFIG_CFGTREE_MAX_TREE_POOL_SIZE + 1,
    sizeof(TreeItem_t));

/// Pool for allocating tree names for iterating.
static le_mem_PoolRef_t TreeItemPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 *  Create a new tree info block.
 *
 *  @return Always returns a new block, should never return NULL.
 */
//--------------------------------------------------------------------------------------------------
static TreeItem_t* NewTreeItem
(
    const char* treeName  ///< [IN] The name of the tree we found.
)
//--------------------------------------------------------------------------------------------------
{
    TreeItem_t* itemPtr = le_mem_ForceAlloc(TreeItemPoolRef);

    itemPtr->link = LE_DLS_LINK_INIT;
    LE_ASSERT(le_utf8_Copy(itemPtr->treeName, treeName, sizeof(itemPtr->treeName), NULL) == LE_OK);

    return itemPtr;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Insert a tree name into the sorted tree list.  If the name is a duplicate it is ignored.
 */
//--------------------------------------------------------------------------------------------------
static void InsertTreeName
(
    ti_TreeIteratorRef_t treeIteratorPtr,  ///< [IN] The iterator we're inserting this name into.
    const char* treeName                   ///< [IN] The name that's being inserted.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&treeIteratorPtr->treeList);

    // Try to add the item, somewhere in the list.
    while (linkPtr != NULL)
    {
        TreeItem_t* itemPtr = CONTAINER_OF(linkPtr, TreeItem_t, link);

        // If the new tree name should go before the current list item.  Insert it in that previous
        // position and we're done.  Otherwise if it's a duplicate, we're done.  If it's larger,
        // continue the search until we either find the end of the list or we find a suitable
        // insertion location.
        int result = strcmp(itemPtr->treeName, treeName);

        if (result > 0)
        {
            TreeItem_t* newItemPtr = NewTreeItem(treeName);
            le_dls_AddBefore(&treeIteratorPtr->treeList, &itemPtr->link, &newItemPtr->link);

            return;
        }
        else if (result == 0)
        {
            // Already inserted.
            return;
        }

        linkPtr = le_dls_PeekNext(&treeIteratorPtr->treeList, linkPtr);
    }

    // Looks like we've found the end of the list, so insert this item there.
    TreeItem_t* newItemPtr = NewTreeItem(treeName);
    le_dls_Queue(&treeIteratorPtr->treeList, &newItemPtr->link);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Find all of the trees currently loaded in memory.
 */
//--------------------------------------------------------------------------------------------------
static void FindLoadedTrees
(
    ti_TreeIteratorRef_t treeIteratorPtr  ///< [IN] The iterator to populate with tree info.
)
//--------------------------------------------------------------------------------------------------
{
    le_hashmap_It_Ref_t iterRef = tdb_GetTreeIterRef();

    while (le_hashmap_NextNode(iterRef) == LE_OK)
    {
        InsertTreeName(treeIteratorPtr, le_hashmap_GetKey(iterRef));
    }
}



//--------------------------------------------------------------------------------------------------
/**
 *  Check whether a directory entry is a regular file or not.
 *
 *  @return
 *       True if specified entry is a regular file
 *       False otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsRegularFile
(
    struct dirent* dirEntryPtr              ///< [IN] Directory entry in question.
)
{
    if (dirEntryPtr->d_type == DT_REG)
    {
        return true;
    }
#ifdef DT_UNKNOWN
    else if (dirEntryPtr->d_type == DT_UNKNOWN)
    {
        // As per man page (http://man7.org/linux/man-pages/man3/readdir.3.html), DT_UNKNOWN
        // should be handled properly for portability purpose. Use lstat(2) to check file info.
        struct stat stbuf;

        if (lstat(dirEntryPtr->d_name, &stbuf) != 0)
        {
            LE_ERROR("Error when trying to lstat '%s'. (%m)", dirEntryPtr->d_name);
            return false;
        }

        return S_ISREG(stbuf.st_mode);
    }
#endif

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Search the file system and find all of the tree files stored there.  This function will not
 *  check to see if the file is properly formated, just if it's in the right directory and has the
 *  correct extension.
 */
//--------------------------------------------------------------------------------------------------
static void FindFileTrees
(
    ti_TreeIteratorRef_t treeIteratorPtr  ///< [IN] The iterator to populate with tree info.
)
//--------------------------------------------------------------------------------------------------
{
    // Open the directory and make sure this is successful.
    DIR* dirPtr = opendir(CFG_TREE_PATH);

    if (dirPtr == NULL)
    {
        LE_WARN("Could not open configTree dir, '%s' for iterating.", CFG_TREE_PATH);
        return;
    }

    // Now iterate through the directory list, and for each entry that's a regular file make sure
    // that it has one of the proper extensions.  If the file looks good, add it to the iterator's
    // list.
    struct dirent* dirEntryPtr;

    while ((dirEntryPtr = readdir(dirPtr)) != NULL)
    {
        if (IsRegularFile(dirEntryPtr))
        {
            char* dotStrPtr = strrchr(dirEntryPtr->d_name, '.');
            if (NULL == dotStrPtr)
            {
               LE_ERROR("dotStrPtr is null.");
               closedir(dirPtr);
               return;
            }

            if (   (strcmp(dotStrPtr, ".rock") != 0)
                && (strcmp(dotStrPtr, ".paper") != 0)
                && (strcmp(dotStrPtr, ".scissors") != 0))
            {
                continue;
            }

            char treeName[MAX_TREE_NAME_BYTES] = "";

            if (le_utf8_CopyUpToSubStr(treeName,
                                       dirEntryPtr->d_name,
                                       dotStrPtr,
                                       sizeof(treeName),
                                       NULL) == LE_OK)
            {
                InsertTreeName(treeIteratorPtr, treeName);
            }
            else
            {
                LE_ERROR("Ignoring configTree file '%s' during iteration because "
                         "the name is too large.",
                         dirEntryPtr->d_name);
            }
        }
    }

    // All done.
    closedir(dirPtr);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the tree iterator subsystem.
 */
//--------------------------------------------------------------------------------------------------
void ti_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Initialize Tree Iterator subsystem.");

    TreeIteratorPoolRef = le_mem_InitStaticPool(TreeIteratorPool,
                                                LE_CONFIG_CFGTREE_MAX_TREE_ITERATOR_POOL_SIZE,
                                                sizeof(TreeIterator_t));
    TreeItemPoolRef = le_mem_InitStaticPool(TreeItemPool,
                                            LE_CONFIG_CFGTREE_MAX_TREE_POOL_SIZE + 1,
                                            sizeof(TreeItem_t));
    IteratorRefMap = le_ref_InitStaticMap(TreeIteratorMap,
                                          LE_CONFIG_CFGTREE_MAX_TREE_ITERATOR_POOL_SIZE);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Create a new tree iterator object.
 *
 *  @return An external reference to a newly created tree iterator object.
 */
//--------------------------------------------------------------------------------------------------
le_cfgAdmin_IteratorRef_t ti_CreateIterator
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The user session this request occured on.
)
//--------------------------------------------------------------------------------------------------
{
    // Allocate memory for the iterator object, then create safe ref.
    ti_TreeIteratorRef_t iteratorPtr = le_mem_ForceAlloc(TreeIteratorPoolRef);
    le_cfgAdmin_IteratorRef_t safeRef = le_ref_CreateRef(IteratorRefMap, iteratorPtr);

    // Initialize the iterator to default values.
    iteratorPtr->sessionRef = sessionRef;
    iteratorPtr->safeRef = safeRef;
    iteratorPtr->treeList = LE_DLS_LIST_INIT;
    iteratorPtr->currentItemPtr = NULL;

    // Gather all in memory trees, then gather all of the unloaded trees from the filesystem.
    FindLoadedTrees(iteratorPtr);
    FindFileTrees(iteratorPtr);

    // Now, move the iterator to the first item.
    iteratorPtr->currentItemPtr = NULL;

    // Return safe ref.
    return safeRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Given an external reference to an iterator, get the original iterator pointer.
 *
 *  @return An internal reference to the tree iterator object if sucdesful.  NULL if the safe ref
 *          lookup fails.
 */
//--------------------------------------------------------------------------------------------------
ti_TreeIteratorRef_t ti_InternalRefFromExternalRef
(
    le_msg_SessionRef_t sessionRef,        ///< [IN] The user session this request occured on.
    le_cfgAdmin_IteratorRef_t externalRef  ///< [IN] The reference we're to look up.
)
//--------------------------------------------------------------------------------------------------
{
    // Find the external reference in the safe ref map and make sure that it belongs to the session
    // requesting it.
    ti_TreeIteratorRef_t iteratorRef = le_ref_Lookup(IteratorRefMap, externalRef);

    if (   (iteratorRef == NULL)
        || (iteratorRef->sessionRef != sessionRef))
    {
        LE_ERROR("Iterator reference <%p> not found.", externalRef);
        return NULL;
    }

    return iteratorRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Release an iterator and invalidate it's external reference.
 */
//--------------------------------------------------------------------------------------------------
void ti_ReleaseIterator
(
    ti_TreeIteratorRef_t iteratorRef  ///< [IN] The iterator to free.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    // Pop each tree item from the list and free each in turn.
    while ((linkPtr = le_dls_Pop(&iteratorRef->treeList)) != NULL)
    {
        TreeItem_t* itemPtr = CONTAINER_OF(linkPtr, TreeItem_t, link);

        itemPtr->link = LE_DLS_LINK_INIT;
        le_mem_Release(itemPtr);
    }

    // Release the safe ref and the memory behind the iterator.
    le_ref_DeleteRef(IteratorRefMap, iteratorRef->safeRef);
    le_mem_Release(iteratorRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  This function takes care of releasing any open iterators that have been orphaned as a result of
 *  a session being closed.
 */
//--------------------------------------------------------------------------------------------------
void ti_CleanUpForSession
(
    le_msg_SessionRef_t sessionRef  ///< [IN] Reference to the session that's going away.
)
//--------------------------------------------------------------------------------------------------
{
    // Simply iterate through the safe ref collection and free any newly orphaned iterators.
    le_ref_IterRef_t safeRefIter = le_ref_GetIterator(IteratorRefMap);

    while (le_ref_NextNode(safeRefIter) == LE_OK)
    {
        ti_TreeIteratorRef_t iterRef = (ti_TreeIteratorRef_t)le_ref_GetValue(safeRefIter);

        if (iterRef->sessionRef == sessionRef)
        {
            ti_ReleaseIterator(iterRef);
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name of the tree that the iterator is currently pointed at.
 *
 *  @return LE_OK if the name copy is successful.  LE_OVERFLOW if the name will not fit in the
 *          target buffer.  LE_NOT_FOUND is returned if the tree list is empty.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ti_GetCurrent
(
    ti_TreeIteratorRef_t iteratorRef,  ///< [IN]  Internal reference to the iterator in question.
    char* namePtr,                     ///< [OUT] Buffer to hold the tree name.
    size_t nameSize                    ///< [IN]  The size of the buffer in question.
)
//--------------------------------------------------------------------------------------------------
{
    // No list?  No current item.
    if (iteratorRef->currentItemPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    // There is a list and a current item.  So, copy the tree name out for the caller.
    TreeItem_t* currentItemPtr = CONTAINER_OF(iteratorRef->currentItemPtr, TreeItem_t, link);

    return le_utf8_Copy(namePtr, currentItemPtr->treeName, nameSize, NULL);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator object onto the next tree in it's list.
 *
 *  @return LE_OK if there is another item to move to.  LE_NOT_FOUND if the iterator is at the end
 *          of the list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ti_MoveNext
(
    ti_TreeIteratorRef_t iteratorRef  ///< [IN] Internal reference to an interator object to move.
)
//--------------------------------------------------------------------------------------------------
{
    // If we haven't started the iteration yet, then try to do so now.
    if (iteratorRef->currentItemPtr == NULL)
    {
        iteratorRef->currentItemPtr = le_dls_Peek(&iteratorRef->treeList);

        return (iteratorRef->currentItemPtr == NULL) ? LE_NOT_FOUND : LE_OK;
    }

    // We have a current item, so peek into the list for the next one.
    le_dls_Link_t* nextPtr = le_dls_PeekNext(&iteratorRef->treeList, iteratorRef->currentItemPtr);

    if (nextPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    // Looks like there was an item found.  Make it current and return success.
    iteratorRef->currentItemPtr = nextPtr;

    return LE_OK;
}
