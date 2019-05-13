/**
 * @page c_safeRef Safe References API
 *
 * @subpage le_safeRef.h "API Reference"
 *
 * <HR>
 *
 *
 * The term "reference" is used to mean "opaque data that refers to some conceptual object".
 * It is intentionally vague to support "information hiding".  Behind the
 * scenes, different implementations can use almost anything that fits into a pointer as a
 * "reference".  Often, they are indexes into arrays or actual pointers to memory objects.
 * When passing those references through an API to outside clients, the implementation
 * becomes exposed to crash bugs when clients
 * pass those references back into the API damaged or stale ("stale" meaning
 * something that has been deleted).
 *
 * <b> Safe References </b> are designed to help protect against damaged or stale references being
 * used by clients.
 *
 * @section c_safeRef_create Create Safe Reference
 *
 * Client calls an API's "Create" function:
 *  - "Create" function creates an object.
 *  - "Create" function creates a "Safe Reference" for the new object @c le_ref_CreateRef()
 *  - "Create" function returns the Safe Reference.
 *
 * @section c_safeRef_lookup Lookup Pointer
 *
 * Followed by:
 *
 * Client calls another API function, passing in the Safe Reference:
 *  - API function translates the Safe Reference back into an object pointer @c le_ref_Lookup()
 *  - API function acts on the object.
 *
 * @section c_safeRef_delete Delete Safe Reference
 *
 * Finishing with:
 *
 * Client calls API's "Delete" function, passing in the Safe Reference:
 *  - "Delete" function translates the Safe Reference back into a pointer to its object.
 *  - "Delete" function invalidates the Safe Reference @c le_ref_DeleteRef()
 *  - "Delete" function deletes the object.
 *
 * At this point, if the Client calls an API function and passes that same (now invalid) Safe
 * Reference (or if the client accidentally passes in some garbage value, like a pointer or zero),
 * the API function will try to translate that into an object pointer. But it'll be told that it's
 * an invalid Safe Reference. The API function can then handle it gracefully, rather
 * than just acting as if it were a valid reference and clobbering the object's deallocated
 * memory or some other object that's reusing the old object's memory.
 *
 * @section c_safeRef_map Create Reference Map
 *
 * A <b> Reference Map </b> object can be used to create Safe References and keep track of the
 * mappings from Safe References to pointers.  At start-up, a Reference Map can be created
 * dynamically by calling @c le_ref_CreateMap(), or can be allocated statically at compile time
 * via @c LE_REF_DEFINE_STATIC_MAP() and initialized via @c le_ref_InitStaticMap().
 *
 * @section c_safeRef_multithreading Multithreading
 *
 * This API's functions are reentrant, but not thread safe. If there's the slightest
 * possibility the same Reference Map will be accessed by two threads at the same time, use
 * a mutex or some other thread synchronization mechanism to protect the Reference Map from
 * concurrent access.
 *
 * @section c_safeRef_example Sample Code
 *
 * Here's an API Definition sample:
 *
 * @code
 * // Opaque reference to Foo objects.
 * typedef struct xyz_foo_Obj* xyz_foo_ObjRef_t;
 *
 * xyz_foo_Ref_t xyz_foo_CreateObject
 * (
 *     void
 * );
 *
 * void xyz_foo_DoSomething
 * (
 *     xyz_foo_Ref_t objRef
 * );
 *
 * void xyz_foo_DeleteObject
 * (
 *     xyz_foo_Ref_t objRef
 * );
 * @endcode
 *
 * Here's an API Implementation sample:
 *
 * @code
 * // Maximum number of Foo objects we expect to have at one time.
 * #define MAX_FOO_OBJECTS  27
 *
 * // Actual Foo objects.
 * typedef struct
 * {
 *     ...
 * }
 * Foo_t;
 *
 * // Pool from which Foo objects are allocated.
 * le_mem_PoolRef_t FooPool;
 *
 * // Safe Reference Map for Foo objects.
 * le_ref_MapRef_t FooRefMap;
 *
 * COMPONENT_INIT
 * {
 *     // Create the Foo object pool.
 *     FooPool = le_mem_CreatePool("FooPool", sizeof(Foo_t));
 *     le_mem_ExpandPool(FooPool, MAX_FOO_OBJECTS);
 *
 *     // Create the Safe Reference Map to use for Foo object Safe References.
 *     FooRefMap = le_ref_CreateMap("FooMap", MAX_FOO_OBJECTS);
 * };
 *
 * xyz_foo_Ref_t xyz_foo_CreateObject
 * (
 *     void
 * )
 * {
 *     Foo_t* fooPtr = le_mem_ForceAlloc(FooPool);
 *
 *     // Initialize the new Foo object.
 *     ...
 *
 *     // Create and return a Safe Reference for this Foo object.
 *     return le_ref_CreateRef(FooRefMap, fooPtr);
 * }
 *
 * void xyz_foo_DoSomething
 * (
 *     xyz_foo_Ref_t objRef
 * )
 * {
 *     Foo_t* fooPtr = le_ref_Lookup(FooRefMap, objRef);
 *
 *     if (fooPtr == NULL)
 *     {
 *         LE_CRIT("Invalid reference (%p) provided!", objRef);
 *         return;
 *     }
 *
 *     // Do something to the object.
 *     ...
 * }
 *
 * void xyz_foo_DeleteObject
 * (
 *     xyz_foo_Ref_t objRef
 * )
 * {
 *     Foo_t* fooPtr = le_ref_Lookup(FooRefMap, objRef);
 *
 *     if (fooPtr == NULL)
 *     {
 *         LE_CRIT("Invalid reference (%p) provided!", objRef);
 *         return;
 *     }
 *
 *     // Invalidate the Safe Reference.
 *     le_ref_DeleteRef(FooRefMap, objRef);
 *
 *     // Release the Foo object.
 *     le_mem_Release(fooPtr);
 * }
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/** @file le_safeRef.h
 *
 * Legato @ref c_safeRef include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SAFEREF_INCLUDE_GUARD
#define LEGATO_SAFEREF_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of safe reference map names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_SAFE_REF_NAME_LEN     31
#define LIMIT_MAX_SAFE_REF_NAME_BYTES   (LIMIT_MAX_SAFE_REF_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a "Reference Map" object, which stores mappings from Safe References to pointers.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_ref_Map *le_ref_MapRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference to an "iterator" object, used to manage iterating a collection of safe refs.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_ref_Iter *le_ref_IterRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Internal block type.
 */
//--------------------------------------------------------------------------------------------------
struct le_ref_Block;


// Internal block sizing
#define LE_REF_BLOCK_SIZE(numRefs)  (1 + (numRefs))

//--------------------------------------------------------------------------------------------------
/**
 *  Reference Map object, which stores mappings from Safe References to pointers.
 *
 *  @note This should not be used directly.
 */
//--------------------------------------------------------------------------------------------------
struct le_ref_Map
{
    le_dls_Link_t        entry;     ///< Map list entry, for inspection tools.

#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    char                 name[LIMIT_MAX_SAFE_REF_NAME_BYTES];   ///< Descriptive name for debugging.
    le_log_TraceRef_t    traceRef;  ///< Trace reference for debugging.
#endif

    size_t               index;     ///< Iterator.
    bool                 advance;   ///< Iterator advance flag.
    size_t               size;      ///< Total allocated entries.
    size_t               maxRefs;   ///< Nominal maximum number of safe references.
    uint32_t             mapBase;   ///< Randomized "base" for references in this map.

    struct le_ref_Block *blocksPtr; ///< Block list head.
};

//--------------------------------------------------------------------------------------------------
/**
 *  Declare variables for a static safe reference map.
 *
 *  In a static safe reference map the space for the maximum number of references is allocated at
 *  compile time.
 */
//--------------------------------------------------------------------------------------------------
#define LE_REF_DEFINE_STATIC_MAP(name, maxRefs)                                 \
    static struct le_ref_Map     _ref_##name##Map;                              \
    static void                 *_ref_##name##Data[LE_REF_BLOCK_SIZE(maxRefs)]

/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_ref_InitStaticMap().
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t _le_ref_InitStaticMap
(
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
    const char      *name,
#endif
    size_t           maxRefs,
    le_ref_MapRef_t  mapPtr,
    void            *data
);
/// @endcond

//--------------------------------------------------------------------------------------------------
/**
 * Initialize an already allocated Reference Map that can hold mappings from Safe References to
 * pointers.
 *
 *  @param[in]  name    Name of the map as specified in LE_REF_DEFINE_STATIC_MAP().
 *  @param[in]  maxRefs Maximum number of Safe References expected to be kept in this Reference Map
 *                      at any one time.
 *
 *  @return The initialized reference map.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
#   define le_ref_InitStaticMap(name, maxRefs) \
        _le_ref_InitStaticMap(#name, (maxRefs), &_ref_##name##Map, _ref_##name##Data)
#else /* if not LE_CONFIG_SAFE_REF_NAMES_ENABLED */
#   define le_ref_InitStaticMap(name, maxRefs) \
    _le_ref_InitStaticMap((maxRefs), &_ref_##name##Map, _ref_##name##Data)
#endif /* end LE_CONFIG_SAFE_REF_NAMES_ENABLED */


#if LE_CONFIG_SAFE_REF_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Create a Reference Map that can hold mappings from Safe References to pointers.
 *
 *  @param[in]  name    Name of the map (for diagnostics).
 *  @param[in]  maxRefs Maximum number of Safe References expected to be kept in this Reference Map
 *                      at any one time.
 *
 *  @return A reference to the Reference Map object.
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t le_ref_CreateMap
(
    const char *name,
    size_t      maxRefs
);
#else /* if not LE_CONFIG_SAFE_REF_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_ref_CreateMap().
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t _le_ref_CreateMap(size_t maxRefs);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Create a Reference Map that can hold mappings from Safe References to pointers.
 *
 *  @param[in]  name    Name of the map (for diagnostics).
 *  @param[in]  maxRefs Maximum number of Safe References expected to be kept in this Reference Map
 *                      at any one time.
 *
 *  @return A reference to the Reference Map object.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_ref_MapRef_t le_ref_CreateMap
(
    const char *name,
    size_t      maxRefs
)
{
    LE_UNUSED(name);
    return _le_ref_CreateMap(maxRefs);
}
#endif /* end LE_CONFIG_SAFE_REF_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Creates a Safe Reference, storing a mapping between that reference and a specified pointer for
 * future lookup.
 *
 * @return The Safe Reference.
 */
//--------------------------------------------------------------------------------------------------
void* le_ref_CreateRef
(
    le_ref_MapRef_t mapRef, ///< [in] Reference Map in which the mapping should be kept.
    void*           ptr     ///< [in] Pointer value to which the new Safe Reference will be mapped.
);


//--------------------------------------------------------------------------------------------------
/**
 * Translates a Safe Reference back to the pointer from when the Safe Reference
 * was created.
 *
 * @return Pointer that the Safe Reference maps to, or NULL if the Safe Reference has been
 *         deleted or is invalid.
 */
//--------------------------------------------------------------------------------------------------
void* le_ref_Lookup
(
    le_ref_MapRef_t mapRef, ///< [in] Reference Map to do the lookup in.
    void*           safeRef ///< [in] Safe Reference to be translated into a pointer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a Safe Reference.
 */
//--------------------------------------------------------------------------------------------------
void le_ref_DeleteRef
(
    le_ref_MapRef_t mapRef, ///< [in] Reference Map to delete the mapping from.
    void*           safeRef ///< [in] Safe Reference to be deleted (invalidated).
);


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
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Immediately enables tracing on a particular safe reference map object.
 **/
//--------------------------------------------------------------------------------------------------
void le_ref_EnableTrace
(
    le_ref_MapRef_t mapRef ///< [in] Reference to the map
);

#endif // LEGATO_SAFEREF_INCLUDE_GUARD
