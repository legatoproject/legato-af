/** @file hashmap.h
 *
 * This file contains definitions exported by the Hash Map module to other modules inside
 * the Legato framework.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _LEGATO_HASHMAP_H_INCLUDE_GUARD
#define _LEGATO_HASHMAP_H_INCLUDE_GUARD

/**
 * A struct to hold the data in the table
 */
typedef struct Entry Entry_t;
struct Entry {
    const void* keyPtr;
    size_t hash;
    const void* valuePtr;
    le_dls_Link_t entryListLink;
};

/**
 * A hashmap iterator
 */
typedef struct le_hashmap_It {
    le_hashmap_Ref_t theMapPtr;
    int32_t currentIndex;
    le_dls_List_t* currentListPtr;
    le_dls_Link_t* currentLinkPtr;
    Entry_t* currentEntryPtr;
    bool isValueValid;
}
HashmapIt_t;

/**
 *  The hashmap itself
 */
typedef struct le_hashmap {
    size_t bucketCount;
    le_hashmap_HashFunc_t hashFuncPtr;
    le_hashmap_EqualsFunc_t equalsFuncPtr;
    size_t size;
    le_mem_PoolRef_t entryPoolRef;
    le_dls_List_t* bucketsPtr;
    size_t* chainLengthPtr;
    const char* nameStr;
    HashmapIt_t* iteratorPtr;
    le_log_TraceRef_t traceRef;
}
Hashmap_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Hash Map module.  This function must be called at start-up, before any other
 * Hash Map functions are called.
 **/
//--------------------------------------------------------------------------------------------------
void hashmap_Init
(
    void
);

#endif // _LEGATO_HASHMAP_H_INCLUDE_GUARD
