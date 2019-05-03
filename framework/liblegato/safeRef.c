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

// =============================================
//  PRIVATE DATA
// =============================================

// Number of pointers in each overflow block
#define OVERFLOW_BLOCK_SIZE 32

// Offset for safety bit in a safe ref
#define REF_SAFETY_OFFSET   UINT32_C(0)
// Bitmask for safety bit in a safe ref
#define REF_SAFETY_MASK     UINT32_C(0x1)

// Offset for map base value in a safe ref
#define REF_BASE_OFFSET     (REF_SAFETY_OFFSET + UINT32_C(1))
// Bitmask for map base value in a safe ref
#define REF_BASE_MASK       UINT32_C(0xFF)

// Offset of index in a safe ref
#define REF_INDEX_OFFSET    (REF_BASE_OFFSET + UINT32_C(8))
// Offset of index in a safe ref
#define REF_INDEX_MASK      UINT32_C(0x7FFFFF)

// Buffer length for dumping a safe reference
#define REF_DBG_BUFFER_LENGTH   64

/// Name used for diagnostics.
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
static const char ModuleName[] = "ref";
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
#   define  SAFEREF_NAME(var) (var)
#else
#   define  SAFEREF_NAME(var) "<omitted>"
#endif

//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_safeRef.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_EVENT_NAMES_ENABLED
LE_DEFINE_INLINE le_ref_MapRef_t le_ref_CreateMap
(
    const char *name,
    size_t      maxRefs
);
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Trace if tracing is enabled for a given reference map.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
#   define SAFE_REF_TRACE(mapRef, ...)                      \
        if ((mapRef)->traceRef != NULL)                     \
        {                                                   \
            LE_TRACE((mapRef)->traceRef, ##__VA_ARGS__);    \
        }
#else /* if not LE_CONFIG_SAFE_REF_NAMES_ENABLED */
#   define SAFE_REF_TRACE(mapRef, ...)   (void) (mapRef)
#endif /* end LE_CONFIG_SAFE_REF_NAMES_ENABLED */

//--------------------------------------------------------------------------------------------------
/**
 * Reference Block object, which stores pointer slots and their status.
 */
//--------------------------------------------------------------------------------------------------
struct le_ref_Block
{
    struct le_ref_Block *nextPtr;   ///< Next (overflow) block in the linked list.
    void                *slots[];   ///< Pointer slots.
};

//--------------------------------------------------------------------------------------------------
/**
 * Local list of all reference maps created within this process.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t RefMapList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to RefMapList.
 */
//--------------------------------------------------------------------------------------------------
static size_t    RefMapListChangeCount = 0;
static size_t   *RefMapListChangeCountRef = &RefMapListChangeCount;

// =============================================
//  PRIVATE FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 *  Determine the block number from a reference index.
 */
//--------------------------------------------------------------------------------------------------
static inline size_t IndexToBlockNum
(
    le_ref_MapRef_t mapRef, ///< Reference map instance.
    size_t          index   ///< Reference index.
)
{
    if (index < mapRef->maxRefs)
    {
        return 0;
    }
    return (index - mapRef->maxRefs) / OVERFLOW_BLOCK_SIZE + 1;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Determine the slot within a block from a reference index.
 */
//--------------------------------------------------------------------------------------------------
static inline size_t IndexToSlot
(
    le_ref_MapRef_t mapRef, ///< Reference map instance.
    size_t          index   ///< Reference index.
)
{
    if (index < mapRef->maxRefs)
    {
        return index;
    }
    return (index - mapRef->maxRefs) % OVERFLOW_BLOCK_SIZE;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Determine the number of slots in a given block.
 */
//--------------------------------------------------------------------------------------------------
static inline size_t SlotsInBlock
(
    le_ref_MapRef_t mapRef,     ///< Reference map instance.
    size_t          blockNum    ///< Block number.
)
{
    if (blockNum == 0)
    {
        return mapRef->maxRefs;
    }
    return OVERFLOW_BLOCK_SIZE;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Compose a reference index from a block number and a slot number.
 */
//--------------------------------------------------------------------------------------------------
static inline size_t BlockAndSlotToIndex
(
    le_ref_MapRef_t mapRef,     ///< Reference map instance.
    size_t          blockNum,   ///< Block number.
    size_t          slotNum     ///< Slot number.
)
{
    if (blockNum == 0)
    {
        return slotNum;
    }
    return mapRef->maxRefs + (blockNum - 1) * OVERFLOW_BLOCK_SIZE + slotNum;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initialize a reference map instance.
 */
//--------------------------------------------------------------------------------------------------
static void InitMap
(
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    const char          *name,          ///< Name of the map (for diagnostics).
#endif
    size_t               maxRefs,       ///< Maximum number of Safe References expected to be kept
                                        ///  in this Reference Map at any one time.
    le_ref_MapRef_t      mapPtr,        ///< Allocated reference map structure to initialize.
    struct le_ref_Block *initialBlock   ///< Allocated initial block.
)
{
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    size_t strLen;

    LE_ASSERT(le_utf8_Copy(mapPtr->name, ModuleName, sizeof(mapPtr->name), &strLen) == LE_OK);

    if (le_utf8_Copy(mapPtr->name + strLen, name, sizeof(mapPtr->name) - strLen, NULL) ==
        LE_OVERFLOW)
    {
        LE_WARN("Map name '%s%s' truncated to '%s'.", ModuleName, name, mapPtr->name);
    }
    mapPtr->traceRef = NULL;
    // le_ref_EnableTrace(mapPtr);
#endif /* end LE_CONFIG_SAFE_REF_NAMES_ENABLED */

    do
    {
        mapPtr->mapBase = le_rand_GetNumBetween(1, UINT32_MAX) & REF_BASE_MASK;
    }
    while (mapPtr->mapBase == 0);

    // Do not zero members as these are pre-zeroed entering this function.
    // Not zeroing also helps debug double-initialization bugs.
    mapPtr->size = maxRefs;
    mapPtr->index = maxRefs;
    mapPtr->maxRefs = maxRefs;
    mapPtr->blocksPtr = initialBlock;

    ++RefMapListChangeCount;
    le_dls_Stack(&RefMapList, &mapPtr->entry);

    SAFE_REF_TRACE(mapPtr, "Safe Reference Map '%s' initialized with base %" PRIX32 " and a maximum"
        " of %" PRIuS " references", SAFEREF_NAME(mapPtr->name), mapPtr->mapBase, mapPtr->maxRefs);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Compose a safe reference value from the map and block information.
 */
//--------------------------------------------------------------------------------------------------
static inline void *MakeRef
(
    uint32_t    mapBase,    ///< Map's base value.
    size_t      index       ///< Pointer index.
)
{
    uintptr_t ref = 0;

    ref |= REF_SAFETY_MASK << REF_SAFETY_OFFSET;
    ref |= (mapBase & REF_BASE_MASK) << REF_BASE_OFFSET;
    ref |= (index & REF_INDEX_MASK) << REF_INDEX_OFFSET;

    return (void *) ref;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Decompose a safe reference into the corresponding block index and slot.
 *
 *  @return Validity of the safe reference.
 */
//--------------------------------------------------------------------------------------------------
static bool ReadRef
(
    const le_ref_MapRef_t    mapRef,    ///< [IN]   Reference map instance.
    const void              *safeRef,   ///< [IN]   Safe reference to decompose.
    size_t                  *blockNum,  ///< [OUT]  Block index.
    size_t                  *slot       ///< [OUT]  Slot number within block.
)
{
    size_t      index;
    uint32_t    base;
    uintptr_t   ref = (uintptr_t) safeRef;
    uintptr_t   safety;

    safety = (ref >> REF_SAFETY_OFFSET) & REF_SAFETY_MASK;
    base = (ref >> REF_BASE_OFFSET) & REF_BASE_MASK;
    index = (ref >> REF_INDEX_OFFSET) & REF_INDEX_MASK;

    *blockNum = IndexToBlockNum(mapRef, index);
    *slot = IndexToSlot(mapRef, index);

    return (safety == REF_SAFETY_MASK && base == mapRef->mapBase && index < mapRef->size);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Retrieve the stored pointer corresponding to a safe reference.
 *
 *  @return A pointer to the slot, or NULL if the reference was invalid.
 */
//--------------------------------------------------------------------------------------------------
static void **FindSlot
(
    const le_ref_MapRef_t    mapRef,    ///< [IN]   Reference map instance.
    const void              *safeRef    ///< [IN]   Safe reference.
)
{
    size_t               blockNum;
    size_t               i;
    size_t               slot;
    struct le_ref_Block *block;

    if (!ReadRef(mapRef, safeRef, &blockNum, &slot))
    {
        return NULL;
    }

    block = mapRef->blocksPtr;
    for (i = 1; i <= blockNum; ++i)
    {
        if (block->nextPtr == NULL)
        {
            return NULL;
        }
        block = block->nextPtr;
    }

    return &block->slots[slot];
}

//--------------------------------------------------------------------------------------------------
/**
 *  Format a safe reference as a string for debugging.
 *
 *  @return The input buffer, containing the formatted reference.
 */
//--------------------------------------------------------------------------------------------------
const char *DebugSafeRef
(
    const le_ref_MapRef_t mapRef,   ///< [IN]   Reference map instance (for validation).
    const void *ref,                ///< [IN]   Safe reference to format.
    char *buffer                    ///< [OUT]  Buffer to write formatted string into.  Must be at
                                    ///         least REF_DBG_BUFFER_LENGTH bytes long.
)
{
    bool        valid;
    size_t      blockNum;
    size_t      slot;
    uint32_t    base;

    valid = ReadRef(mapRef, ref, &blockNum, &slot);
    base = (uint32_t) ((uintptr_t) ref >> REF_BASE_OFFSET) & REF_BASE_MASK;
    snprintf(buffer, REF_DBG_BUFFER_LENGTH,
        "<%p>(Bm:%" PRIX32 " Br:%" PRIX32 " N:%" PRIuS " S:%" PRIuS " V:%c)",
        ref, mapRef->mapBase, base, blockNum, slot, (valid ? 'T' : 'F'));

    return buffer;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Allocate a new overflow block.  This occurs if the maxRefs limit is exceeded.
 *
 *  @return The new overflow block.
 */
//--------------------------------------------------------------------------------------------------
static struct le_ref_Block *NewOverflowBlock
(
    void
)
{
    struct le_ref_Block *block = malloc(LE_REF_BLOCK_SIZE(OVERFLOW_BLOCK_SIZE) * sizeof(void *));
    LE_ASSERT(block != NULL);

    memset(block, 0, LE_REF_BLOCK_SIZE(OVERFLOW_BLOCK_SIZE) * sizeof(void *));
    return block;
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
static void *Lookup
(
    le_ref_MapRef_t  mapRef,    ///< [IN]   The Reference Map to do the lookup in.
    const void      *safeRef    ///< [IN]   The Safe Reference to be translated into a pointer.
)
{
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    char      buffer[REF_DBG_BUFFER_LENGTH];
#endif
    void    **result;

    SAFE_REF_TRACE(mapRef, "Looking up safe reference %s in %s",
        DebugSafeRef(mapRef, safeRef, buffer), SAFEREF_NAME(mapRef->name));

    result = FindSlot(mapRef, safeRef);
    if (result == NULL)
    {
        SAFE_REF_TRACE(mapRef, "    No matching entry found");
        return NULL;
    }

    SAFE_REF_TRACE(mapRef, "    Found entry %p at %p", *result, result);
    return *result;
}

// =============================================
//  PUBLIC API FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 *  Initialize a previously allocated reference map.
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t _le_ref_InitStaticMap
(
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    const char      *name,      ///< Name of the map (for diagnostics).
#endif
    size_t           maxRefs,   ///< Maximum number of Safe References expected to be kept in this
                                ///  Reference Map at any one time.
    le_ref_MapRef_t  mapPtr,    ///< Allocated reference map structure to initialize.
    void            *data       ///< Allocated initial pointer block to initialize.
)
{
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    InitMap(name, maxRefs, mapPtr, (struct le_ref_Block *) data);
#else
    InitMap(maxRefs, mapPtr, (struct le_ref_Block *) data);
#endif

    return mapPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Reference Map that can hold mappings from Safe References to pointers.
 *
 * @return A reference to the Reference Map object.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
le_ref_MapRef_t le_ref_CreateMap
(
    const char      *name,      ///< Name of the map (for diagnostics).
    size_t           maxRefs    ///< Maximum number of Safe References expected to be kept in this
                                ///  Reference Map at any one time.
)
#else
le_ref_MapRef_t _le_ref_CreateMap
(
    size_t           maxRefs    ///< Maximum number of Safe References expected to be kept in this
                                ///  Reference Map at any one time.
)
#endif
{
    le_ref_MapRef_t      mapPtr = calloc(1, sizeof(*mapPtr));
    struct le_ref_Block *initialBlockPtr = calloc(LE_REF_BLOCK_SIZE(maxRefs), sizeof(void *));

    LE_ASSERT(initialBlockPtr != NULL);
    LE_ASSERT(mapPtr != NULL);

#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    InitMap(name, maxRefs, mapPtr, initialBlockPtr);
#else
    InitMap(maxRefs, mapPtr, initialBlockPtr);
#endif

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
void *le_ref_CreateRef
(
    le_ref_MapRef_t mapRef, ///< [in] The Reference Map in which the mapping should be kept.
    void*           ptr     ///< [in] Pointer value to which the new Safe Reference will be mapped.
)
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    char                 buffer[REF_DBG_BUFFER_LENGTH];
#endif
    size_t               blockCount = IndexToBlockNum(mapRef, mapRef->size - 1) + 1;
    size_t               i;
    size_t               index;
    size_t               j;
    size_t               slotCount;
    struct le_ref_Block *block;
    void                *result = NULL;

    SAFE_REF_TRACE(mapRef, "Creating safe reference for %p in %s", ptr, SAFEREF_NAME(mapRef->name));
    if (ptr == NULL)
    {
        goto end;
    }

    block = mapRef->blocksPtr;
    for (i = 0; i < blockCount; ++i)
    {
        slotCount = SlotsInBlock(mapRef, i);
        for (j = 0; j < slotCount; ++j)
        {
            if (block->slots[j] == NULL)
            {
                index = BlockAndSlotToIndex(mapRef, i, j);
                block->slots[j] = ptr;
                SAFE_REF_TRACE(mapRef, "    Inserted %p at %" PRIuS " (%p)", ptr, index,
                    &block->slots[j]);
                result = MakeRef(mapRef->mapBase, index);
                goto end;
            }
        }

        if (block->nextPtr == NULL)
        {
            break;
        }
        block = block->nextPtr;
    }

    LE_WARN("Safe reference map maximum references exceeded for %s.", SAFEREF_NAME(mapRef->name));
    index = BlockAndSlotToIndex(mapRef, blockCount, 0);
    __atomic_store_n(&block->nextPtr, NewOverflowBlock(), __ATOMIC_RELAXED);
    block = block->nextPtr;
    SAFE_REF_TRACE(mapRef, "    Created new overflow block %p", block);

    block->slots[0] = ptr;
    SAFE_REF_TRACE(mapRef, "    Inserted %p at %" PRIuS " (%p)", ptr, index, &block->slots[0]);
    result = MakeRef(mapRef->mapBase, index);
    mapRef->size += OVERFLOW_BLOCK_SIZE;
    mapRef->index = mapRef->size;

end:
    SAFE_REF_TRACE(mapRef, "    Resulting safe reference is %s",
        DebugSafeRef(mapRef, result, buffer));
    return result;
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
void *le_ref_Lookup
(
    le_ref_MapRef_t mapRef, ///< [in] The Reference Map to do the lookup in.
    void*           safeRef ///< [in] The Safe Reference to be translated into a pointer.
)
//--------------------------------------------------------------------------------------------------
{
    return Lookup(mapRef, safeRef);
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
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    char      buffer[REF_DBG_BUFFER_LENGTH];
#endif
    void    **slot;

    SAFE_REF_TRACE(mapRef, "Deleting safe reference %s in %s",
        DebugSafeRef(mapRef, safeRef, buffer), SAFEREF_NAME(mapRef->name));

    slot = FindSlot(mapRef, safeRef);
    if (slot == NULL || *slot == NULL)
    {
        LE_ERROR("Deleting non-existent Safe Reference %p from Map '%s'.", safeRef,
            SAFEREF_NAME(mapRef->name));
    }
    else
    {
        *slot = NULL;
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
    mapRef->index = 0;
    mapRef->advance = false;
    SAFE_REF_TRACE(mapRef, "Starting iteration in %s", SAFEREF_NAME(mapRef->name));
    return (le_ref_IterRef_t) mapRef;
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
    le_ref_MapRef_t   mapRef = (le_ref_MapRef_t) iteratorRef;
    void            **slot;

    SAFE_REF_TRACE(mapRef, "Continuing iteration in %s", SAFEREF_NAME(mapRef->name));

    if (mapRef->index >= mapRef->size)
    {
        SAFE_REF_TRACE(mapRef, "    Passed end of items");
        return LE_FAULT;
    }
    if (mapRef->advance)
    {
        ++mapRef->index;
    }
    mapRef->advance = false;

    while (mapRef->index < mapRef->size)
    {
        slot = FindSlot(mapRef, MakeRef(mapRef->mapBase, mapRef->index));
        if (slot != NULL && *slot != NULL)
        {
            SAFE_REF_TRACE(mapRef, "    Found next item at index %" PRIuS, mapRef->index);
            mapRef->advance = true;
            return LE_OK;
        }

        ++mapRef->index;
    }

    // Set to an invalid reference
    mapRef->index = mapRef->size;
    SAFE_REF_TRACE(mapRef, "    End of items");
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the safe ref iterator is currently pointing at.  If the iterator has just
 * been initialized and le_hashmap_NextNode() has not been called, or if the iterator has been
 * invalidated then this will return NULL.
 *
 * @return  A pointer to the current key, or NULL if the iterator has been invalidated or is not
 *          ready.
 *
 */
//--------------------------------------------------------------------------------------------------
const void* le_ref_GetSafeRef
(
    le_ref_IterRef_t iteratorRef ///< [IN] Reference to the iterator.
)
{
    le_ref_MapRef_t mapRef = (le_ref_MapRef_t) iteratorRef;

    if (mapRef->index < mapRef->size)
    {
        return MakeRef(mapRef->mapBase, mapRef->index);
    }

    return NULL;
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
    const void      *safeRef = le_ref_GetSafeRef(iteratorRef);
    le_ref_MapRef_t  mapRef = (le_ref_MapRef_t) iteratorRef;

    return Lookup(mapRef, safeRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Immediately enables tracing on a particular safe reference map object.
 **/
//--------------------------------------------------------------------------------------------------
void le_ref_EnableTrace
(
    le_ref_MapRef_t mapRef ///< [in] Reference to the map
)
{
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    mapRef->traceRef = le_log_GetTraceRef(mapRef->name);
    le_log_EnableTrace(mapRef->traceRef);
#else /* if not LE_CONFIG_SAFE_REF_NAMES_ENABLED */
    LE_UNUSED(mapRef);
    LE_WARN("Safe Reference Map tracing disabled by LE_CONFIG_SAFE_REF_NAMES_ENABLED setting.");
#endif /* end LE_CONFIG_SAFE_REF_NAMES_ENABLED */
}

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the ref map list; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t *ref_GetRefMapList
(
    void
)
{
    return (&RefMapList);
}

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the ref map list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t **ref_GetRefMapListChgCntRef
(
    void
)
{
    return (&RefMapListChangeCountRef);
}
