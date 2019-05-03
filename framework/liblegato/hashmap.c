/** @file hashmap.c
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 * Parts of this file are Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "legato.h"
#include "hsieh_hash.h"
#include "limit.h"

#if LE_CONFIG_REDUCE_FOOTPRINT
#   define BUCKET_LIST_INIT LE_SLS_LIST_INIT
#   define BUCKET_LINK_INIT LE_SLS_LINK_INIT
#   define bucket_IsEmpty   le_sls_IsEmpty
#   define bucket_NumLinks  le_sls_NumLinks
#   define bucket_Peek      le_sls_Peek
#   define bucket_PeekNext  le_sls_PeekNext
#   define bucket_Queue     le_sls_Queue
#   define bucket_Stack     le_sls_Stack
#   define bucket_PeekTail  le_sls_PeekTail

//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_hashmap.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_HASHMAP_NAMES_ENABLED
LE_DEFINE_INLINE le_hashmap_Ref_t le_hashmap_Create
(
    const char                *nameStr,
    size_t                     capacity,
    le_hashmap_HashFunc_t      hashFunc,
    le_hashmap_EqualsFunc_t    equalsFunc
);
#endif

//--------------------------------------------------------------------------------------------------
/**
 *  Remove an entry from a bucket list.
 *
 *  @param  listHeadPtr Head of the list.
 *  @param  theLinkPtr  Link to remove.
 *  @param  prevLinkPtr Previous link, or NULL if item is theLinkPtr the first item in the list.
 */
//--------------------------------------------------------------------------------------------------
static inline void bucket_Remove
(
    le_sls_List_t *listHeadPtr,
    le_sls_Link_t *theLinkPtr,
    le_sls_Link_t *prevLinkPtr
)
{
    LE_UNUSED(theLinkPtr);

    le_sls_RemoveAfter(listHeadPtr, prevLinkPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the link previous to currentLinkPtr without removing it from the list.  User must
 * ensure that currentLinkPtr is in the list otherwise the behaviour of this function is undefined.
 *
 * @return
 *      Pointer to the previous link if successful.
 *      NULL if there is no link previous to the currentLinkPtr (currentLinkPtr is at the head of
 *      the list).
 */
//--------------------------------------------------------------------------------------------------
static inline le_sls_Link_t *bucket_PeekPrev
(
    const le_sls_List_t *listPtr,       ///< [IN] List containing currentLinkPtr.
    const le_sls_Link_t *currentLinkPtr ///< [IN] Get the link that is relative to this link.
)
{
    le_sls_Link_t *linkPtr = le_sls_Peek(listPtr);
    if (linkPtr == currentLinkPtr || linkPtr == NULL)
    {
        return NULL;
    }

    while (linkPtr != NULL && currentLinkPtr != le_sls_PeekNext(listPtr, linkPtr))
    {
        linkPtr = le_sls_PeekNext(listPtr, linkPtr);
    }
    return linkPtr;
}
#else /* if not LE_CONFIG_REDUCE_FOOTPRINT */
#   define BUCKET_LIST_INIT LE_DLS_LIST_INIT
#   define BUCKET_LINK_INIT LE_DLS_LINK_INIT
#   define bucket_IsEmpty   le_dls_IsEmpty
#   define bucket_NumLinks  le_dls_NumLinks
#   define bucket_Peek      le_dls_Peek
#   define bucket_PeekNext  le_dls_PeekNext
#   define bucket_PeekPrev  le_dls_PeekPrev
#   define bucket_PeekTail  le_dls_PeekTail
#   define bucket_Queue     le_dls_Queue
#   define bucket_Stack     le_dls_Stack

//--------------------------------------------------------------------------------------------------
/**
 *  Remove an entry from a bucket list.
 *
 *  @param  listHeadPtr Head of the list.
 *  @param  theLinkPtr  Link to remove.
 *  @param  prevLinkPtr Previous link, or NULL if item is theLinkPtr the first item in the list.
 */
//--------------------------------------------------------------------------------------------------
#   define bucket_Remove(listHeadPtr, theLinkPtr, prevLinkPtr) \
    ((void) (prevLinkPtr), le_dls_Remove((listHeadPtr), (theLinkPtr)))

#endif /* end LE_CONFIG_REDUCE_FOOTPRINT */

//--------------------------------------------------------------------------------------------------
/**
 * Trace if tracing is enabled for a given hashmap.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
#   define HASHMAP_TRACE(mapRef, ...)                       \
        if ((mapRef)->traceRef != NULL)                     \
        {                                                   \
            LE_TRACE((mapRef)->traceRef, ##__VA_ARGS__);    \
        }
#else /* if not LE_CONFIG_HASHMAP_NAMES_ENABLED */
#   define HASHMAP_TRACE(mapRef, ...)   (void) (mapRef)
#endif /* end LE_CONFIG_HASHMAP_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Calculate a hash. First this calls the user-supplied hash function.
 * Then it does some defensive coding to avoid bad hashes from outside hash functions
 *
 * @param map A pointer to the hashmap instance
 * @param key A pointer to the key to hash
 * @return  Returns a new hash
 *
 */
//--------------------------------------------------------------------------------------------------
static inline size_t HashKey(le_hashmap_Hashmap_t* map, const void* key) {
    size_t h = map->hashFuncPtr(key);

    // If the Hseih hash has been used then we can just return h
    if (map->hashFuncPtr == &le_hashmap_HashString)
    {
        return h;
    }

    // We apply this secondary hashing discovered by Doug Lea to defend
    // against bad hashes. This is important for user-supplied hash fns
    h += ~(h << 9);
    h ^= (((unsigned int) h) >> 14);
    h += (h << 4);
    h ^= (((unsigned int) h) >> 10);

    return h;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new entry to put in the map. Allocates the entry from the pool which was created
 * during construction of the map.
 *
 * @param newKeyPtr A pointer to the key
 * @param newValuePtr A pointer to the value
 * @param poolRef A memory pool reference to use for allocating memory. This pool must have been
 * created as a pool of Entry_t.
 * @return  Returns a pointer to the newly allocated and filled-in Entry_t
 *
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Entry_t* CreateEntry
(
    const void* newKeyPtr,
    const void* newValuePtr,
    le_mem_PoolRef_t poolRef
)
{
    le_hashmap_Entry_t* entryPtr = le_mem_ForceAlloc(poolRef);
    LE_ASSERT(entryPtr);

    entryPtr->keyPtr = newKeyPtr;
    entryPtr->valuePtr = newValuePtr;
    entryPtr->entryListLink = BUCKET_LINK_INIT;
    return entryPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Given a hash and a map size, calculate the index at which to store the entry
 *
 * @param bucketCount The number of buckets in the map
 * @param hash The hash to use
 * @return  Returns the index to use in the bucket array
 *
 */
//--------------------------------------------------------------------------------------------------
static inline size_t CalculateIndex(size_t bucketCount, size_t hash) {
    return (hash) & (bucketCount - 1);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks if 2 keys are equal (or are actually the same key)
 *
 * @param keyAPtr Pointer to the first key
 * @param keyBPtr Pointer to the second key
 * @param equalsFuncPtr The equality function in use by the map
 * @return  Returns true if the keys are identical, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------
static inline bool EqualKeys
(
    const void* keyAPtr,
    const void* keyBPtr,
    le_hashmap_EqualsFunc_t equalsFuncPtr
)
{
    if (keyAPtr == keyBPtr) {
        return true;
    }
    return equalsFuncPtr(keyAPtr, keyBPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Look up the head of a bucket list by index.
 *
 *  @return  Bucket list.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Bucket_t *IndexToBucket
(
    le_hashmap_Hashmap_t    *mapRef,    ///< Map instance.
    size_t                   index      ///< Bucket index.
)
{
    return (index < mapRef->bucketCount ? &mapRef->bucketsPtr[index] : NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get number of buckets required for a given capacity
 */
//--------------------------------------------------------------------------------------------------
size_t GetBucketCount
(
    size_t capacity ///< [IN] Number of expected items in the hashmap
)
{
    size_t buckets;

    // Check for no overflow
    LE_ASSERT(4*capacity >= capacity);

    capacity = (4*capacity)/3;

    // Round capacity up to a power of 2
    for (buckets = 4; buckets && buckets < capacity; buckets <<= 1)
    {
        /* no body */
    }

    return buckets;
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal function to initialize a statically-defined hashmap
 *
 * @note use le_hashmap_InitStatic() macro instead
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t _le_hashmap_InitStatic
(
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    const char*                nameStr,          ///< [in] Name of the HashMap
#endif
    size_t                     capacity,         ///< [in] Expected capacity of the map
    le_hashmap_HashFunc_t      hashFunc,         ///< [in] The hash function
    le_hashmap_EqualsFunc_t    equalsFunc,       ///< [in] The equality function
    le_hashmap_Hashmap_t*      mapPtr,           ///< [in] The static hash map to initialize
    le_mem_PoolRef_t           entryPoolRef,     ///< [in] The memory pool for map entries
    le_hashmap_Bucket_t*       bucketsPtr        ///< [in] The bucket lists
)
{
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    LE_ASSERT(nameStr);
#endif
    LE_ASSERT(hashFunc);
    LE_ASSERT(equalsFunc);
    LE_ASSERT(mapPtr);
    LE_ASSERT(entryPoolRef);
    LE_ASSERT(bucketsPtr);

    // Do not zero members as these are pre-zeroed entering this function.
    // Not zeroing also helps debug double-initialization bugs.

    mapPtr->bucketCount = LE_HASHMAP_BUCKET_COUNT(capacity);

    mapPtr->entryPoolRef = entryPoolRef;
    le_mem_SetNumObjsToForce(mapPtr->entryPoolRef, mapPtr->bucketCount / 8);

    mapPtr->bucketsPtr = bucketsPtr;

    mapPtr->hashFuncPtr = hashFunc;
    mapPtr->equalsFuncPtr = equalsFunc;
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    mapPtr->nameStr = nameStr;
#endif

    le_hashmap_GetIterator(mapPtr);
    return mapPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a HashMap
 *
 * @return  Returns a reference to the map.
 *
 * @note Terminates the process on failure, so no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
le_hashmap_Ref_t le_hashmap_Create
(
    const char*                nameStr,          ///< [in] Name of the HashMap
    size_t                     capacity,         ///< [in] Expected capacity of the map
    le_hashmap_HashFunc_t      hashFunc,         ///< [in] The hash function
    le_hashmap_EqualsFunc_t    equalsFunc        ///< [in] The equality function
)
#else
le_hashmap_Ref_t _le_hashmap_Create
(
    size_t                     capacity,         ///< [in] Expected capacity of the map
    le_hashmap_HashFunc_t      hashFunc,         ///< [in] The hash function
    le_hashmap_EqualsFunc_t    equalsFunc        ///< [in] The equality function
)
#endif
{
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    char poolName[LIMIT_MAX_MEM_POOL_NAME_BYTES] = "hashMap_";
    le_utf8_Append(poolName, nameStr, sizeof(poolName), NULL);
#else
    char poolName[] = "";
#endif

    size_t bucketCount = GetBucketCount(capacity);

    // Use same function internally as static allocation, but take pointers from
    // heap instead of static memory
    return _le_hashmap_InitStatic(
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
        nameStr,
#endif
        capacity,
        hashFunc,
        equalsFunc,
        calloc(1, sizeof(le_hashmap_Hashmap_t)),
        le_mem_ExpandPool(le_mem_CreatePool(poolName,
                                            sizeof(le_hashmap_Entry_t)),
                          bucketCount / 2),
        calloc(bucketCount, sizeof(le_hashmap_Bucket_t)));
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a key-value pair to a HashMap. If the key already exists in the map then the previous value
 * will be replaced with the new value passed into this function.
 *
 * The process will terminate if this fails as it implies an inability to allocate any more memory
 *
 */
//--------------------------------------------------------------------------------------------------

void* le_hashmap_Put
(
    le_hashmap_Ref_t mapRef,   ///< [in] Reference to the map
    const void* keyPtr,        ///< [in] Pointer to the key to be stored
    const void* valuePtr       ///< [in] Pointer to the value to be stored
)
{
    size_t hash = HashKey(mapRef, keyPtr);
    size_t index = CalculateIndex(mapRef->bucketCount, hash);

    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Generated index of %" PRIuS " for hash %u",
        mapRef->nameStr,
        index,
        (int)hash
    );

    le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[index]);

    if (bucket_IsEmpty(listHeadPtr))
    {
        le_hashmap_Entry_t* newEntryPtr = CreateEntry(keyPtr, valuePtr, mapRef->entryPoolRef);
        LE_ASSERT(newEntryPtr);

        bucket_Stack(listHeadPtr, &(newEntryPtr->entryListLink));
        mapRef->size++;

        HASHMAP_TRACE(
            mapRef,
            "Hashmap %s: Added first entry to bucket. Total map size now %" PRIuS,
            mapRef->nameStr,
            mapRef->size
        );

        return NULL;
    }
    else
    {
        le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

        while (true) {
            le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                               le_hashmap_Entry_t,
                                                               entryListLink);

            // Replace existing value if the keys match.
            if (EqualKeys(currentEntryPtr->keyPtr,
                          keyPtr,
                          mapRef->equalsFuncPtr)
                          )
            {
                const void* oldValue = currentEntryPtr->valuePtr;
                currentEntryPtr->valuePtr = valuePtr;

                HASHMAP_TRACE(
                    mapRef,
                    "Hashmap %s: Replaced entry in bucket. Total map size now %" PRIuS,
                    mapRef->nameStr,
                    mapRef->size
                );

                return (void *)oldValue;
            }

            // Add a new entry if we are at the tail
            if (bucket_PeekNext(listHeadPtr, theLinkPtr) == NULL) {
                le_hashmap_Entry_t* newEntryPtr = CreateEntry(keyPtr,
                                                              valuePtr,
                                                              mapRef->entryPoolRef);
                LE_ASSERT(newEntryPtr);

                bucket_Queue(listHeadPtr, &(newEntryPtr->entryListLink));
                mapRef->size++;

                HASHMAP_TRACE(
                    mapRef,
                    "Hashmap %s: Added entry to bucket at tail. Map size now %" PRIuS,
                    mapRef->nameStr,
                    mapRef->size
                );

                HASHMAP_TRACE(
                    mapRef,
                    "Hashmap %s: Bucket now contains %" PRIuS " entries",
                    mapRef->nameStr,
                    bucket_NumLinks(listHeadPtr)
                );

                return NULL;
            }

            // Move to next entry.
            theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve a value from a HashMap.
 *
 * @return  Returns a pointer to the value or NULL if the key is not found.
 *
 */
//--------------------------------------------------------------------------------------------------

void* le_hashmap_Get
(
    le_hashmap_Ref_t mapRef,   ///< [in] Reference to the map
    const void* keyPtr         ///< [in] Pointer to the key to be retrieved
)
{
    size_t hash = HashKey(mapRef, keyPtr);
    size_t index = CalculateIndex(mapRef->bucketCount, hash);
    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Generated index of %" PRIuS " for hash %" PRIuS,
        mapRef->nameStr,
        index,
        hash
    );

    le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[index]);
    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Looked up list contains %" PRIuS " links",
        mapRef->nameStr,
        bucket_NumLinks(listHeadPtr)
    );

    le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

    while (theLinkPtr != NULL) {
        le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                           le_hashmap_Entry_t,
                                                           entryListLink);
        if (EqualKeys(currentEntryPtr->keyPtr,
                          keyPtr,
                          mapRef->equalsFuncPtr)
                          )
        {
            HASHMAP_TRACE(
                mapRef,
                "Hashmap %s: Returning found value for key",
                mapRef->nameStr
            );
            return (void*)(currentEntryPtr->valuePtr);
        }
        theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
    }

    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Key not found",
        mapRef->nameStr
    );
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve a stored key from a HashMap.
 *
 * @return  Returns a pointer to the key that was stored in the HashMap by le_hashmap_Put() or
 *          NULL if the key is not found.
 *
 */
//--------------------------------------------------------------------------------------------------
void* le_hashmap_GetStoredKey
(
    le_hashmap_Ref_t mapRef,   ///< [in] Reference to the map.
    const void* keyPtr         ///< [in] Pointer to the key to be retrieved.
)
{
    size_t hash = HashKey(mapRef, keyPtr);
    size_t index = CalculateIndex(mapRef->bucketCount, hash);
    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Generated index of %" PRIuS " for hash %" PRIuS,
        mapRef->nameStr,
        index,
        hash
    );

    le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[index]);
    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Looked up list contains %" PRIuS " links",
        mapRef->nameStr,
        bucket_NumLinks(listHeadPtr)
    );

    le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

    while (theLinkPtr != NULL) {
        le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                           le_hashmap_Entry_t,
                                                           entryListLink);
        if (EqualKeys(currentEntryPtr->keyPtr,
                          keyPtr,
                          mapRef->equalsFuncPtr)
                          )
        {
            HASHMAP_TRACE(
                mapRef,
                "Hashmap %s: Returning original key",
                mapRef->nameStr
            );
            return (void*)(currentEntryPtr->keyPtr);
        }
        theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
    }

    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Key not found",
        mapRef->nameStr
    );
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a value from a HashMap.
 *
 * @note  If the iterator is currently on the item being removed, then it's value is invalidated.
 *        The iterator will have to be moved before values and keys can be read from it again.
 *
 * @return  Returns a pointer to the value or NULL if the key is not found.
 *
 */
//--------------------------------------------------------------------------------------------------

void* le_hashmap_Remove
(
   le_hashmap_Ref_t mapRef, ///< [in] Reference to the map
   const void* keyPtr       ///< [in] Pointer to the key to be removed
)
{
    int hash = HashKey(mapRef, keyPtr);
    size_t index = CalculateIndex(mapRef->bucketCount, hash);

    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Generated index of %" PRIuS " for hash %u",
        mapRef->nameStr,
        index,
        hash
    );

    le_hashmap_Bucket_t *listHeadPtr = &(mapRef->bucketsPtr[index]);
    le_hashmap_Link_t   *theLinkPtr = bucket_Peek(listHeadPtr);
    le_hashmap_Link_t   *prevLinkPtr = NULL;

    while (theLinkPtr != NULL) {
        le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                           le_hashmap_Entry_t,
                                                           entryListLink);
        if (EqualKeys(currentEntryPtr->keyPtr,
                          keyPtr,
                          mapRef->equalsFuncPtr)
                          )
        {
            if (mapRef->iterator.currentLinkPtr == theLinkPtr)
            {
                le_hashmap_PrevNode(&mapRef->iterator);
            }

            void* value = (void*)(currentEntryPtr->valuePtr);
            bucket_Remove(listHeadPtr, theLinkPtr, prevLinkPtr);
            le_mem_Release( currentEntryPtr );
            mapRef->size--;

            HASHMAP_TRACE(
                mapRef,
                "Hashmap %s: Removing key from map",
                mapRef->nameStr
            );

            return value;
        }

        prevLinkPtr = theLinkPtr;
        theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
    }

    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Key not found",
        mapRef->nameStr
    );
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests if the HashMap is empty (i.e. contains zero keys).
 *
 * @return  Returns true if empty, false otherwise.
 *
 */
//--------------------------------------------------------------------------------------------------

bool le_hashmap_isEmpty
(
    le_hashmap_Ref_t mapRef    ///< [in] Reference to the map
)
{
    return (mapRef->size == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculates the number of keys in the HashMap.
 *
 * @return  The number of keys in the HashMap.
 *
 */
//--------------------------------------------------------------------------------------------------

size_t le_hashmap_Size
(
    le_hashmap_Ref_t mapRef    ///< [in] Reference to the map
)
{
    return mapRef->size;
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests if the HashMap contains a particular key
 *
 * @return  Returns true if the key is found, false otherwise.
 *
 */
//--------------------------------------------------------------------------------------------------

bool le_hashmap_ContainsKey
(
    le_hashmap_Ref_t mapRef,  ///< [in] Reference to the map
    const void* keyPtr        ///< [in] Pointer to the key to be searched for
)
{
    int hash = HashKey(mapRef, keyPtr);
    size_t index = CalculateIndex(mapRef->bucketCount, hash);

    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Generated index of %" PRIuS " for hash %u",
        mapRef->nameStr,
        index,
        hash
    );

    le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[index]);
    le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

    while (theLinkPtr != NULL) {
        le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                           le_hashmap_Entry_t,
                                                           entryListLink);
        if (EqualKeys(currentEntryPtr->keyPtr,
                          keyPtr,
                          mapRef->equalsFuncPtr)
                          )
        {
            HASHMAP_TRACE(
                mapRef,
                "Hashmap %s: Key found",
                mapRef->nameStr
            );

            return true;
        }
        theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
    }

    HASHMAP_TRACE(
       mapRef,
       "Hashmap %s: Key not found",
       mapRef->nameStr
    );

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes all the entries held in the hashmap. This will not delete the data pointed to by the
 * key and value pointers. That cleanup is the responsibility of the caller.
 * This allows the map to be re-used. Currently maps cannot be deleted.
 *
 */
//--------------------------------------------------------------------------------------------------

void le_hashmap_RemoveAll
(
    le_hashmap_Ref_t mapRef    ///< [in] Reference to the map
)
{
    // Reset the iterator
    le_hashmap_GetIterator(mapRef);

    uint32_t i;
    for (i = 0; i < mapRef->bucketCount; i++) {
        le_hashmap_Bucket_t *listHeadPtr = &(mapRef->bucketsPtr[i]);
        le_hashmap_Link_t   *theLinkPtr = bucket_Peek(listHeadPtr);

        while (theLinkPtr != NULL) {
            le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                               le_hashmap_Entry_t,
                                                               entryListLink);
            le_hashmap_Link_t* linkPtrToRemove = theLinkPtr;
            theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
            bucket_Remove(listHeadPtr, linkPtrToRemove, NULL);
            le_mem_Release( currentEntryPtr );
        }
        mapRef->bucketsPtr[i] = BUCKET_LIST_INIT;
    }
    mapRef->size = 0;

    HASHMAP_TRACE(
       mapRef,
       "Hashmap %s: All entries deleted from map",
       mapRef->nameStr
    );
}


//--------------------------------------------------------------------------------------------------
/**
 * Iterates over the whole map, calling the supplied callback with each key-value pair. If the
 * callback returns false for any key then this function will return.
 *
 * @return  Returns true if all elements were checked; or false if iteration was stopped early
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_hashmap_ForEach
(
    le_hashmap_Ref_t mapRef,                ///< [in] Reference to the map
    le_hashmap_ForEachHandler_t forEachFn,  ///< [in] Callback function to be called with each pair
    void* context                           ///< [in] Pointer to a context to be supplied to the
                                            ///<      callback
)
{
    uint32_t i;
    for (i = 0; i < mapRef->bucketCount; i++) {
        le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[i]);
        le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

        while (theLinkPtr != NULL) {
            le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                               le_hashmap_Entry_t,
                                                               entryListLink);
            if (!forEachFn(currentEntryPtr->keyPtr, currentEntryPtr->valuePtr, context))
            {
                // Check to see if this is the last element, and return false if not.
                if (bucket_PeekNext(listHeadPtr, theLinkPtr) != NULL)
                {
                     return false;
                }
                uint32_t j;
                for (j = i; j < mapRef->bucketCount; ++j)
                {
                    le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[j]);
                    if (bucket_Peek(listHeadPtr))
                    {
                        return false;
                    }
                }
                return true;   // Despite stopping early, all elements have been examined.
            }
            theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets an interator for step-by-step iteration over the map. In this mode
 * the iteration is controlled by the calling function using the le_hashmap_NextNode function.
 * There is one iterator per map, and calling this function resets the
 * iterator position to the start of the map.
 *
 * @return  Returns A reference to a hashmap iterator which is ready
 *          for le_hashmap_NextNode to be called on it
 *
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_It_Ref_t le_hashmap_GetIterator
(
    le_hashmap_Ref_t mapRef                 ///< [in] Reference to the map
)
{
    mapRef->iterator.currentIndex = 0;
    mapRef->iterator.currentLinkPtr = NULL;
    return &mapRef->iterator;
}


//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator to the next key/value pair in the map. Order is dependent
 * on the hash algorithm and the order of inserts and is not sorted at all.
 *
 * @return  Returns LE_OK unless you go past the end of the map, then returns LE_NOT_FOUND
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_hashmap_NextNode
(
    le_hashmap_It_Ref_t iteratorRef        ///< [IN] Reference to the iterator
)
{
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    bool                 isHead = false;
#endif
    le_hashmap_Bucket_t *listHeadPtr;
    le_hashmap_Ref_t     mapRef = CONTAINER_OF(iteratorRef, le_hashmap_Hashmap_t, iterator);

    // If the map is empty immediately return LE_NOT_FOUND
    if (le_hashmap_isEmpty(mapRef))
    {
        return LE_NOT_FOUND;
    }

    for (;;)
    {
        listHeadPtr = IndexToBucket(mapRef, iteratorRef->currentIndex);
        if (listHeadPtr == NULL)
        {
            // At end of map
            return LE_NOT_FOUND;
        }

        if (iteratorRef->currentLinkPtr != NULL)
        {
            iteratorRef->currentLinkPtr = bucket_PeekNext(listHeadPtr, iteratorRef->currentLinkPtr);
        }
        else
        {
            iteratorRef->currentLinkPtr = bucket_Peek(listHeadPtr);
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
            isHead = true;
#endif
        }

        if (iteratorRef->currentLinkPtr != NULL)
        {
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
            HASHMAP_TRACE(
                mapRef,
                "Found index %s match, index is %" PRIuS,
                (isHead ? "head" : "list"),
                iteratorRef->currentIndex
            );
#endif
            return LE_OK;
        }
        else
        {
            ++iteratorRef->currentIndex;
            if (iteratorRef->currentIndex >= mapRef->bucketCount)
            {
                // At end of map
                return LE_NOT_FOUND;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator to the previous key/value pair in the map. Order is dependent
 * on the hash algorithm and the order of inserts, and is not sorted at all.
 *
 * @return  Returns LE_OK unless you go past the beginning of the map, then returns LE_NOT_FOUND.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_hashmap_PrevNode
(
    le_hashmap_It_Ref_t iteratorRef        ///< [IN] Reference to the iterator
)
{
    le_hashmap_Bucket_t *listHeadPtr;
    le_hashmap_Ref_t     mapRef = CONTAINER_OF(iteratorRef, le_hashmap_Hashmap_t, iterator);

    // If the map is empty or we are at the beginning immediately return LE_NOT_FOUND
    if (le_hashmap_isEmpty(mapRef) ||
        (iteratorRef->currentIndex == 0 && iteratorRef->currentLinkPtr == NULL))
    {
        return LE_NOT_FOUND;
    }

    if (iteratorRef->currentIndex >= mapRef->bucketCount)
    {
        iteratorRef->currentIndex = mapRef->bucketCount - 1;
    }
    for (;;)
    {
        listHeadPtr = IndexToBucket(mapRef, iteratorRef->currentIndex);

        if (iteratorRef->currentLinkPtr != NULL)
        {
            iteratorRef->currentLinkPtr = bucket_PeekPrev(listHeadPtr, iteratorRef->currentLinkPtr);
        }
        else
        {
            iteratorRef->currentLinkPtr = bucket_PeekTail(listHeadPtr);
        }

        if (iteratorRef->currentLinkPtr != NULL)
        {
            HASHMAP_TRACE(
                mapRef,
                "Found index match, index is %" PRIuS,
                iteratorRef->currentIndex
            );
            return LE_OK;
        }
        else if (iteratorRef->currentIndex > 0)
        {
            --iteratorRef->currentIndex;
        }
        else
        {
            // Reached start of map
            return LE_NOT_FOUND;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the key which the iterator is currently pointing at
 *
 * @return  A pointer to the current key, or NULL if the iterator has been invalidated
 *
 */
//--------------------------------------------------------------------------------------------------
const void* le_hashmap_GetKey
(
    le_hashmap_It_Ref_t iteratorRef        ///< [IN] Reference to the iterator
)
{
    le_hashmap_Entry_t *entryPtr;

    if (iteratorRef->currentLinkPtr == NULL)
    {
        return NULL;
    }

    entryPtr = CONTAINER_OF(iteratorRef->currentLinkPtr, le_hashmap_Entry_t, entryListLink);
    return entryPtr->keyPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the value which the iterator is currently pointing at
 *
 * @return  A pointer to the current value, or NULL if the iterator has been invalidated
 *
 */
//--------------------------------------------------------------------------------------------------
void* le_hashmap_GetValue
(
    le_hashmap_It_Ref_t iteratorRef        ///< [IN] Reference to the iterator
)
{
    le_hashmap_Entry_t *entryPtr;

    if (iteratorRef->currentLinkPtr == NULL)
    {
        return NULL;
    }

    // Need to cast away the const
    entryPtr = CONTAINER_OF(iteratorRef->currentLinkPtr, le_hashmap_Entry_t, entryListLink);
    return (void *) entryPtr->valuePtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the key and value of the first node stored in the hashmap.
 * The hashmap is not sorted so this will simply return the first node stored in the map.
 * There is no guarantee that a subsequent call to this function will return the same pair
 * if new keys have been added to the map.
 * If NULL is passed as the firstValuePointer then only the key will be returned.
 *
 * @return  LE_OK if the first node is returned or LE_NOT_FOUND if the map is empty.
 *          LE_BAD_PARAMETER if the key is NULL.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_hashmap_GetFirstNode
(
    le_hashmap_Ref_t mapRef,   ///< [in] Reference to the map
    void **firstKeyPtr,        ///> [out] Pointer to the first key
    void **firstValuePtr       ///> [out] Pointer to the first value
)
{
    // If the map is empty immediately return LE_NOT_FOUND
    if (le_hashmap_isEmpty(mapRef))
    {
        return LE_NOT_FOUND;
    }

    // If the keyPtr is NULL return LE_BAD_PARAMETER
    if (firstKeyPtr == NULL)
    {
        LE_ERROR("NULL key");
        return LE_BAD_PARAMETER;
    }

    // Find the first list head
    size_t index = 0;
    for (
           ;
           index < mapRef->bucketCount;
           index++ )
    {
        le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[index]);
        le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

        if (NULL != theLinkPtr)
        {
            le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                               le_hashmap_Entry_t,
                                                               entryListLink);
            *firstKeyPtr = (void *)currentEntryPtr->keyPtr;
            *firstValuePtr = (void *)currentEntryPtr->valuePtr;
            break;
        }
    }
    return LE_OK;
};

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the key and value of the node after the passed in key.
 * The hashmap is not sorted so this will simply return the next node stored in the map.
 * There is no guarantee that a subsequent call to this function will return the same pair
 * if new keys have been added to the map.
 * If NULL is passed as the nextValuePtr then only the key will be returned.
 *
 * @return  LE_OK if the next node is returned. If the keyPtr is not found in the
 *          map then LE_BAD_PARAMETER is returned. LE_NOT_FOUND is returned if the passed
 *          in key is the last one in the map.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_hashmap_GetNodeAfter
(
    le_hashmap_Ref_t mapRef,   ///< [in] Reference to the map
    const void* keyPtr,        ///< [in] Pointer to the key to be searched for
    void **nextKeyPtr,         ///> [out] Pointer to the first key
    void **nextValuePtr        ///> [out] Pointer to the first value
)
{
    // If the map is empty or the key is invalid
    if (
          (le_hashmap_isEmpty(mapRef)) ||
          (keyPtr == NULL) ||
          (nextKeyPtr == NULL)
        )
    {
        return LE_BAD_PARAMETER;
    }

    // Find the node pointed to by the key
    size_t hash = HashKey(mapRef, keyPtr);
    size_t index = CalculateIndex(mapRef->bucketCount, hash);
    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Generated index of %" PRIuS " for hash %" PRIuS,
        mapRef->nameStr,
        index,
        hash
    );

    le_hashmap_Bucket_t* listHeadPtr = &(mapRef->bucketsPtr[index]);
    HASHMAP_TRACE(
        mapRef,
        "Hashmap %s: Looked up list contains %" PRIuS " links",
        mapRef->nameStr,
        bucket_NumLinks(listHeadPtr)
    );

    le_hashmap_Link_t* theLinkPtr = bucket_Peek(listHeadPtr);

    while (theLinkPtr != NULL) {
        le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                           le_hashmap_Entry_t,
                                                           entryListLink);
        if (EqualKeys(currentEntryPtr->keyPtr,
                          keyPtr,
                          mapRef->equalsFuncPtr)
                          )
        {
            HASHMAP_TRACE(
                mapRef,
                "Hashmap %s: Found value for key",
                mapRef->nameStr
            );
            // Now find the next node, if there is one
            theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
            if (NULL == theLinkPtr)
            {
                // Find the next list head
                for (
                       index++;
                       index < mapRef->bucketCount;
                       index++ )
                {
                    listHeadPtr = &(mapRef->bucketsPtr[index]);
                    theLinkPtr = bucket_Peek(listHeadPtr);

                    if (NULL != theLinkPtr)
                    {
                        le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                                           le_hashmap_Entry_t,
                                                                           entryListLink);
                        *nextKeyPtr = (void *)currentEntryPtr->keyPtr;
                        if (NULL != nextValuePtr)
                        {
                            *nextValuePtr = (void *)currentEntryPtr->valuePtr;
                        }
                        return LE_OK;
                    }
                }
                // There was no list head - we are off the end of the map
                return LE_NOT_FOUND;
            }
            else
            {
                le_hashmap_Entry_t* currentEntryPtr = CONTAINER_OF(theLinkPtr,
                                                                   le_hashmap_Entry_t,
                                                                   entryListLink);
                *nextKeyPtr = (void *)currentEntryPtr->keyPtr;
                if (NULL != nextValuePtr)
                {
                    *nextValuePtr = (void *)currentEntryPtr->valuePtr;
                }
                return LE_OK;
            }
        }
        theLinkPtr = bucket_PeekNext(listHeadPtr, theLinkPtr);
    }

    // The original key was never found
    return LE_BAD_PARAMETER;
}


//--------------------------------------------------------------------------------------------------
/**
 * Counts the total number of collisions in the map. A collision occurs
 * when more than one entry is stored in the map at the same index.
 *
 * @return  Returns The sum of the collisions in the map
 *
 */
//--------------------------------------------------------------------------------------------------

size_t le_hashmap_CountCollisions
(
    le_hashmap_Ref_t mapRef     ///< [in] Reference to the map
)
{
    size_t i, collCount = 0;
    for (i = 0; i < mapRef->bucketCount; i++) {
        size_t chainLength = bucket_NumLinks(&mapRef->bucketsPtr[i]);
        if (chainLength > 1)
        {
            collCount += chainLength - 1;
        }
    }
    return collCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * String hashing function. This can be used as a parameter to le_hashmap_Create if the key to
 * the table is a string
 *
 * @return  Returns the hash value of the string pointed to by stringToHash
 *
 */
//--------------------------------------------------------------------------------------------------

size_t le_hashmap_HashString
(
    const void* stringToHashPtr    ///< [in] Pointer to the string to be hashed
)
{
    int len = le_utf8_NumBytes(stringToHashPtr);
    return SuperFastHash(stringToHashPtr, len);
}

//--------------------------------------------------------------------------------------------------
/**
 * String equality function. This can be used as a paramter to le_hashmap_Create if the key to
 * the table is a string
 *
 * @return  Returns true if the strings are identical, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------

bool le_hashmap_EqualsString
(
    const void* firstStringPtr,    ///< [in] Pointer to the first string for comparing.
    const void* secondStringPtr    ///< [in] Pointer to the second string for comparing.
)
{
    if (firstStringPtr == secondStringPtr)
    {
        return true;
    }

    return (strcmp(firstStringPtr, secondStringPtr) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Integer hashing function. This can be used as a paramter to le_hashmap_Create if the key to
 * the table is a uint32_t
 *
 * @return  Returns the hash value of the uint32_t pointed to by intToHash
 *
 */
//--------------------------------------------------------------------------------------------------

size_t le_hashmap_HashUInt32
(
    const void* intToHashPtr    ///< [in] Pointer to the integer to be hashed
)
{
    uint32_t ui = *((uint32_t *)intToHashPtr);
    return (size_t)ui;
}

//--------------------------------------------------------------------------------------------------
/**
 * Integer equality function. This can be used as a paramter to le_hashmap_Create if the key to
 * the table is a uint32_t
 *
 * @return  Returns true if the integers are equal, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------

bool le_hashmap_EqualsUInt32
(
    const void* firstIntPtr,    ///< [in] Pointer to the first integer for comparing.
    const void* secondIntPtr    ///< [in] Pointer to the second integer for comparing.
)
{
    int a = *((uint32_t*) firstIntPtr);
    int b = *((uint32_t*) secondIntPtr);
    return a == b;
}

//--------------------------------------------------------------------------------------------------
/**
 * Long integer hashing function. This can be used as a paramter to le_hashmap_Create if the key to
 * the table is a uint64_t
 *
 * @return  Returns the hash value of the uint64_t pointed to by intToHash
 *
 */
//--------------------------------------------------------------------------------------------------

size_t le_hashmap_HashUInt64
(
    const void* intToHashPtr    ///< [in] Pointer to the long integer to be hashed
)
{
    uint64_t ui = *((uint64_t *)intToHashPtr);
    return (size_t)ui;
}

//--------------------------------------------------------------------------------------------------
/**
 * Long integer equality function. This can be used as a paramter to le_hashmap_Create if the key to
 * the table is a uint64_t
 *
 * @return  Returns true if the integers are equal, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------

bool le_hashmap_EqualsUInt64
(
    const void* firstIntPtr,    ///< [in] Pointer to the first long integer for comparing.
    const void* secondIntPtr    ///< [in] Pointer to the second long integer for comparing.
)
{
    int a = *((uint64_t*) firstIntPtr);
    int b = *((uint64_t*) secondIntPtr);
    return a == b;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pointer hashing function. This can be used as a parameter to le_hashmap_Create() if the key to
 * the table is an pointer or reference. Simply pass in the address as the key.
 *
 * @return  Returns the hash value of the pointer pointed to by voidToHashPtr
 *
 */
//--------------------------------------------------------------------------------------------------

size_t le_hashmap_HashVoidPointer
(
    const void* voidToHashPtr    ///< [in] Pointer to be hashed
)
{
    // Return the key value itself.
    return (size_t) voidToHashPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pointer equality function. This can be used as a parameter to le_hashmap_Create() if the key to
 * the table is an pointer or reference.
 *
 * @return  Returns true if the pointers are equal, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------

bool le_hashmap_EqualsVoidPointer
(
    const void* firstVoidPtr,    ///< [in] First pointer for comparing.
    const void* secondVoidPtr    ///< [in] PSecond pointer for comparing.
)
{
    return firstVoidPtr == secondVoidPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Makes a particular hashmap traceable without enabling the tracing.  After this is called, when
 * the trace keyword for this hashmap (the hashmap's name) is enabled for the "framework" component
 * in the process, then tracing will commence.  If that keyword was already enabled before
 * this function is called, then tracing will commence immediately when this function is called.
 **/
//--------------------------------------------------------------------------------------------------
void le_hashmap_MakeTraceable
(
    le_hashmap_Ref_t mapRef     ///< [in] Reference to the map
)
{
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    mapRef->traceRef = le_log_GetTraceRef(mapRef->nameStr);

    LE_TRACE(mapRef->traceRef, "Tracing enabled for hashmap %s", mapRef->nameStr);
    LE_TRACE(
        mapRef->traceRef,
        "Hashmap %s: Bucket count calculated as %" PRIdS,
        mapRef->nameStr,
        mapRef->bucketCount
    );
#else /* if not LE_CONFIG_HASHMAP_NAMES_ENABLED */
    LE_UNUSED(mapRef);
    LE_WARN("Hashmap tracing disabled by LE_CONFIG_HASHMAP_NAMES_ENABLED setting.");
#endif /* end LE_CONFIG_HASHMAP_NAMES_ENABLED */
}


//--------------------------------------------------------------------------------------------------
/**
 * Immediately enables tracing on a particular hashmap object.
 **/
//--------------------------------------------------------------------------------------------------
void le_hashmap_EnableTrace
(
    le_hashmap_Ref_t mapRef     ///< [in] Reference to the map
)
{
#if LE_CONFIG_HASHMAP_NAMES_ENABLED
    le_log_EnableTrace(le_log_GetTraceRef(mapRef->nameStr));
    le_hashmap_MakeTraceable(mapRef);
#else /* if not LE_CONFIG_HASHMAP_NAMES_ENABLED */
    LE_UNUSED(mapRef);
    LE_WARN("Hashmap tracing disabled by LE_CONFIG_HASHMAP_NAMES_ENABLED setting.");
#endif /* end LE_CONFIG_HASHMAP_NAMES_ENABLED */
}
