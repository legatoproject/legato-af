/**
 * @page c_atomic Atomic Operation API
 *
 * @subpage le_atomic.ch "API Reference"
 *
 * <HR>
 *
 * An atomic operation interface for Legato.  Using an atomic operation has two effects:
 *  - Ensures all threads either see the whole operation performed, or none of it, and
 *  - Provides guarantees on when the effects of an operation are seen by other threads,
 *    relative to the surrounding code.
 */

#ifndef LEGATO_ATOMIC_INCLUDE_GUARD
#define LEGATO_ATOMIC_INCLUDE_GUARD

#ifdef __GNUC__

/**
 * Does not create ordering constraints between threads.
 */
#define LE_ATOMIC_ORDER_RELAXED __ATOMIC_RELAXED

/**
 * Creates a constraint that guarantees this operation will occur before all later operations,
 * as seen by all threads.
 */
#define LE_ATOMIC_ORDER_ACQUIRE __ATOMIC_ACQUIRE

/**
 * Creates a constraint that guarantees this operation will occur after all previous operations,
 * as seen by all threads.
 */
#define LE_ATOMIC_ORDER_RELEASE __ATOMIC_RELEASE

/**
 * Combines the effects of LE_ATOMIC_ORDER_ACQUIRE and LE_ATOMIC_ORDER_RELEASE
 */
#define LE_ATOMIC_ORDER_ACQ_REL __ATOMIC_ACQ_REL

/**
 * Test if a value has previously been set, and set it to true.  This returns true if and only if
 * the value was previously true.
 *
 * @param ptr points to a bool.
 * @param order ordering constraint
 */
#define LE_ATOMIC_TEST_AND_SET(ptr, order) __atomic_test_and_set((ptr), (order))

/**
 * Performs an atomic add operation. Results are stored in address pointed to by ptr
 *
 * @return output value of operation
 */
#define LE_ATOMIC_ADD_FETCH(ptr, value, order)  __sync_add_and_fetch((ptr), (value))

/**
 * Performs an atomic subtract operation. Results are stored in address pointed to by ptr
 *
 * @return output value of operation
 */
#define LE_ATOMIC_SUB_FETCH(ptr, value, order)  __sync_sub_and_fetch((ptr), (value))

/**
 * Performs an atomic bitwise-OR operation. Results are stored in address pointed to by ptr
 *
 * @return output value of operation
 */
#define LE_ATOMIC_OR_FETCH(ptr, value, order)  __sync_or_and_fetch((ptr), (value))

/**
 * Performs an atomic bitwise-AND operation. Results are stored in address pointed to by ptr
 *
 * @return output value of operation
 */
#define LE_ATOMIC_AND_FETCH(ptr, value, order) __sync_and_and_fetch((ptr), (value))

/**
 * Perform an atomic compare and swap. If the current value of *ptr is oldval, then write newval
 * into *ptr.
 *
 * @return True if comparison is successful and newval was written
 */
#define LE_SYNC_BOOL_COMPARE_AND_SWAP(ptr, oldval, newval) \
            __sync_bool_compare_and_swap((ptr), (oldval), (newval))

#else /* !__GCC__ */

// On compilers other than GCC, the framework adaptor must provide an implementation.
#ifndef LE_ATOMIC_ORDER_RELAXED
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_ORDER_RELAXED"
#endif

#ifndef LE_ATOMIC_ORDER_ACQUIRE
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_ORDER_ACQUIRE"
#endif

#ifndef LE_ATOMIC_ORDER_RELEASE
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_ORDER_RELEASE"
#endif

#ifndef LE_ATOMIC_ORDER_ACQ_REL
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_ORDER_ACQ_REL"
#endif

#ifndef LE_ATOMIC_TEST_AND_SET
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_TEST_AND_SET"
#endif

#ifndef LE_ATOMIC_ADD_FETCH
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_ADD_FETCH"
#endif

#ifndef LE_ATOMIC_SUB_FETCH
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_SUB_FETCH"
#endif

#ifndef LE_ATOMIC_OR_FETCH
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_OR_FETCH"
#endif

#ifndef LE_ATOMIC_AND_FETCH
#error "The frameworkAdaptor is missing a definition of LE_ATOMIC_AND_FETCH"
#endif

#ifndef LE_SYNC_BOOL_COMPARE_AND_SWAP
#error "The frameworkAdaptor is missing a definition of LE_SYNC_BOOL_COMPARE_AND_SWAP"
#endif

#endif /* __GCC__ */

#endif // LEGATO_ATOMIC_INCLUDE_GUARD
