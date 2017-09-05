/** @file safeRef.c
 *
 * Legato @ref c_safeRef implementation.
 *
 * @note We use only odd numbers for Safe References.  This ensures that it will not be a
 *       word-aligned memory address modern systems (which are always even).
 *       This prevents Safe References from getting confused with pointers.
 *       If someone tries to dereference a Safe Reference, they will get a bus error on most
 *       processor architectures.  Also, if they try to use a memory address as a Safe Ref,
 *       the memory address is guaranteed to be detected as an invalid Safe Reference.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "limit.h"

// =============================================
//  PRIVATE DATA
// =============================================

#define MAX_NAME_BYTES LIMIT_MAX_MEM_POOL_NAME_BYTES

/// Default number of Map objects in the Map Pool.
/// @todo Make this configurable.
#define DEFAULT_MAP_POOL_SIZE 10

/// Name used for diagnostics.
static const char ModuleName[] = "ref";

//--------------------------------------------------------------------------------------------------
/**
 * Reference Map object, which stores mappings from Safe References to pointers.
 * The actual mapping is held in a hashmap.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_ref_Map
{
    uint32_t             nextRefNum;     ///< The next Safe Reference value to be assigned.

    le_hashmap_Ref_t    referenceMap;    ///< HashMap of Mapping objects.

    char          name[MAX_NAME_BYTES]; ///< The name of the map (for diagnostics).
}
Map_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool of Map objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MapPool;

// =============================================
//  PRIVATE FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * SafeRef hashing function.
 *
 * @return  Returns the SafeRef itself as it can used as a hash
 *
 */
//--------------------------------------------------------------------------------------------------

size_t hashSafeRef
(
    const void* safeRefPtr    ///< [in] Safe Ref to be hashed
)
{
    // Return the key value itself.
    return (size_t) safeRefPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * SafeRef equality function.
 *
 * @return  Returns true if the integers are equal, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------

bool equalsSafeRef
(
    const void* firstSafeRef,    ///< [in] First SafeRef for comparing.
    const void* secondSafeRef    ///< [in] Second SafeRef for comparing.
)
{
    return firstSafeRef == secondSafeRef;
}

// =============================================
//  PROTECTED (Intra-Module) FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Safe Reference Module.
 *
 * This function must be called exactly once at process start-up, before any Safe Reference API
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void safeRef_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Initialize the Map Pool.
    MapPool = le_mem_CreatePool("SafeRef-Map", sizeof(Map_t));
    le_mem_ExpandPool(MapPool, DEFAULT_MAP_POOL_SIZE);
}


// =============================================
//  PUBLIC API FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Create a Reference Map that can hold mappings from Safe References to pointers.
 *
 * @return A reference to the Reference Map object.
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t le_ref_CreateMap
(
    const char* name,   ///< [in] The name of the map (for diagnostics).

    size_t      maxRefs ///< [in] The maximum number of Safe References expected to be kept in
                        ///       this Reference Map at any one time.
)
//--------------------------------------------------------------------------------------------------
{
    Map_t* mapPtr = le_mem_ForceAlloc(MapPool);

    size_t strLen;

    LE_ASSERT(le_utf8_Copy(mapPtr->name, ModuleName, sizeof(mapPtr->name), &strLen) == LE_OK);

    if (   le_utf8_Copy(mapPtr->name + strLen, name, sizeof(mapPtr->name) - strLen, NULL)
        == LE_OVERFLOW)
    {
        LE_WARN("Map name '%s%s' truncated to '%s'.", ModuleName, name, mapPtr->name);
    }

    /// @todo Make this a random number so that using a reference from another Map is unlikely to
    ///       get by undetected.
    mapPtr->nextRefNum = 0x10000001; // Use only odd numbers.

    mapPtr->referenceMap = le_hashmap_Create(mapPtr->name,
                                             maxRefs,
                                             hashSafeRef,
                                             equalsSafeRef
                                            );

    return mapPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a Safe Reference, storing a mapping between that reference and a given pointer for
 * future lookup.
 *
 * @return The Safe Reference.
 */
//--------------------------------------------------------------------------------------------------
void* le_ref_CreateRef
(
    le_ref_MapRef_t mapRef, ///< [in] The Reference Map in which the mapping should be kept.
    void*           ptr     ///< [in] Pointer value to which the new Safe Reference will be mapped.
)
//--------------------------------------------------------------------------------------------------
{
    ssize_t thisRef = mapRef->nextRefNum;

    le_hashmap_Put(mapRef->referenceMap, (const void*)(thisRef), ptr);

    mapRef->nextRefNum += 2; // Increment the reference number for next time, keeping it odd.

    return (void *)thisRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Translates a Safe Reference back into the pointer that was given when the Safe Reference
 * was created.
 *
 * @return The pointer that the Safe Reference maps to, or NULL if the Safe Reference has been
 *         deleted or is invalid.
 */
//--------------------------------------------------------------------------------------------------
void* le_ref_Lookup
(
    le_ref_MapRef_t mapRef, ///< [in] The Reference Map to do the lookup in.
    void*           safeRef ///< [in] The Safe Reference to be translated into a pointer.
)
//--------------------------------------------------------------------------------------------------
{
    return le_hashmap_Get(mapRef->referenceMap, safeRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a Safe Reference.
 */
//--------------------------------------------------------------------------------------------------
void le_ref_DeleteRef
(
    le_ref_MapRef_t mapRef, ///< [in] The Reference Map to delete the mapping from.
    void*           safeRef ///< [in] The Safe Reference to be deleted (invalidated).
)
//--------------------------------------------------------------------------------------------------
{
    if (le_hashmap_Remove(mapRef->referenceMap, safeRef) == NULL)
    {
        LE_ERROR("Deleting non-existent Safe Reference %p from Map '%s'.", safeRef, mapRef->name);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an interator for step-by-step iteration over the map. In this mode the iteration is
 * controlled by the calling function using the le_ref_NextNode() function.  There is one iterator
 * per map, and calling this function resets the iterator position to the start of the map.  The
 * iterator is not ready for data access until le_ref_NextNode() has been called at least once.
 *
 * @return  Returns A reference to a hashmap iterator which is ready for le_hashmap_NextNode() to be
 *          called on it.
 */
//--------------------------------------------------------------------------------------------------
le_ref_IterRef_t le_ref_GetIterator
(
    le_ref_MapRef_t mapRef ///< [in] Reference to the map.
)
{
    return (le_ref_IterRef_t)le_hashmap_GetIterator(mapRef->referenceMap);
}


//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator to the next key/value pair in the map.
 *
 * @return  Returns LE_OK unless you go past the end of the map, then returns LE_NOT_FOUND.
 *          If the iterator has been invalidated by the map changing or you have previously
 *          received a LE_NOT_FOUND then this returns LE_FAULT.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ref_NextNode
(
    le_ref_IterRef_t iteratorRef ///< [IN] Reference to the iterator.
)
{
    return le_hashmap_NextNode((le_hashmap_It_Ref_t)iteratorRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the safe ref iterator is currently pointing at.  If the iterator has just
 * been initialized and le_hashmap_NextNode() has not been called, or if the iterator has been
 * invalidated then this will return NULL.
 *
 * @return  A pointer to the current key, or NULL if the iterator has been invalidated or is not ready.
 *
 */
//--------------------------------------------------------------------------------------------------
const void* le_ref_GetSafeRef
(
    le_ref_IterRef_t iteratorRef ///< [IN] Reference to the iterator.
)
{
    return le_hashmap_GetKey((le_hashmap_It_Ref_t)iteratorRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the value which the iterator is currently pointing at.  If the iterator
 * has just been initialized and le_ref_NextNode() has not been called, or if the iterator has been
 * invalidated then this will return NULL.
 *
 * @return  A pointer to the current value, or NULL if the iterator has been invalidated or is not
 *          ready.
 */
//--------------------------------------------------------------------------------------------------
void* le_ref_GetValue
(
    le_ref_IterRef_t iteratorRef ///< [IN] Reference to the iterator.
)
{
    return le_hashmap_GetValue((le_hashmap_It_Ref_t)iteratorRef);
}
