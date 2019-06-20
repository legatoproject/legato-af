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

#endif /* __GCC__ */

#endif // LEGATO_ATOMIC_INCLUDE_GUARD
