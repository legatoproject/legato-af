
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeIterator.h
 *
 *  Implementation of the configTree's treeIterator object.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_TREE_ITERATOR_INCLUDE_GUARD
#define CFG_TREE_ITERATOR_INCLUDE_GUARD




/// Raw pointer ref for a tree iterator.
typedef struct TreeIterator* ti_TreeIteratorRef_t;


/// Const version of the iterator ref.
typedef struct TreeIterator const* ni_ConstTreeIteratorRef_t;




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the tree iterator subsystem.
 */
//--------------------------------------------------------------------------------------------------
void ti_Init
(
    void
);




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
);




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
    le_cfgAdmin_IteratorRef_t iteratorRef  ///< [IN] The reference we're to look up.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Release an iterator and invalidate it's external reference.
 */
//--------------------------------------------------------------------------------------------------
void ti_ReleaseIterator
(
    ti_TreeIteratorRef_t iteratorRef  ///< [IN] The iterator to free.
);




//--------------------------------------------------------------------------------------------------
/**
 *  This function takes care of releasing any open iterators that have been orphaned as a result of
 *  a session being closed.
 */
//--------------------------------------------------------------------------------------------------
void ti_CleanUpForSession
(
    le_msg_SessionRef_t sessionRef  ///< [IN] Reference to the session that's going away.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name of the tree that the iterator is currently pointed at.
 *
 *  @return LE_OK if the name copy is successful.  LE_OVERFLOW if the name will not fit in the
 *          target buffer.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ti_GetCurrent
(
    ti_TreeIteratorRef_t iteratorRef,  ///< [IN]  Internal reference to the iterator in question.
    char* namePtr,                     ///< [OUT] Buffer to hold the tree name.
    size_t nameSize                    ///< [IN]  The size of the buffer in question.
);




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
);




#endif
