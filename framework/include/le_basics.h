/**
 * @page c_basics Basic Type and Constant Definitions
 *
 * Cardinal types and commonly-used constants form the basic
 * foundation on which everything else is built. These include
 * error codes, portable integer types, and helpful macros that make things easier to use.
 *
 * See @ref le_basics.h for basic cardinal types and commonly-used constants info.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_basics.h
 *
 * Legato @ref c_basics include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_BASICS_INCLUDE_GUARD
#define LEGATO_BASICS_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Standard result codes.
 *
 * @note All error codes are negative integers. They allow functions with signed
 *       integers to return non-negative values when successful or standard error codes on failure.
 * @deprecated the result code LE_NOT_POSSIBLE is scheduled to be removed.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_OK = 0,                  ///< Successful.
    LE_NOT_FOUND = -1,          ///< Referenced item does not exist or could not be found.
    LE_NOT_POSSIBLE = -2,       ///< @deprecated It is not possible to perform the requested action.
    LE_OUT_OF_RANGE = -3,       ///< An index or other value is out of range.
    LE_NO_MEMORY = -4,          ///< Insufficient memory is available.
    LE_NOT_PERMITTED = -5,      ///< Current user does not have permission to perform requested action.
    LE_FAULT = -6,              ///< Unspecified internal error.
    LE_COMM_ERROR = -7,         ///< Communications error.
    LE_TIMEOUT = -8,            ///< A time-out occurred.
    LE_OVERFLOW = -9,           ///< An overflow occurred or would have occurred.
    LE_UNDERFLOW = -10,         ///< An underflow occurred or would have occurred.
    LE_WOULD_BLOCK = -11,       ///< Would have blocked if non-blocking behaviour was not requested.
    LE_DEADLOCK = -12,          ///< Would have caused a deadlock.
    LE_FORMAT_ERROR = -13,      ///< Format error.
    LE_DUPLICATE = -14,         ///< Duplicate entry found or operation already performed.
    LE_BAD_PARAMETER = -15,     ///< Parameter is invalid.
    LE_CLOSED = -16,            ///< The resource is closed.
    LE_BUSY = -17,              ///< The resource is busy.
    LE_UNSUPPORTED = -18,       ///< The underlying resource does not support this operation.
    LE_IO_ERROR = -19,          ///< An IO operation failed.
    LE_NOT_IMPLEMENTED = -20,   ///< Unimplemented functionality.
    LE_UNAVAILABLE = -21,       ///< A transient or temporary loss of a service or resource.
    LE_TERMINATED = -22,        ///< The process, operation, data stream, session, etc. has stopped.
    LE_IN_PROGRESS = -23,       ///< The operation is in progress.
}
le_result_t;


//--------------------------------------------------------------------------------------------------
/**
 * ON/OFF type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_OFF = 0,
    LE_ON  = 1,
}
le_onoff_t;


//--------------------------------------------------------------------------------------------------
/** @name Bit Masks
 *
 * Single byte bit definitions that can be used for bit masking.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define BIT0        0x01
#define BIT1        0x02
#define BIT2        0x04
#define BIT3        0x08
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80
// @}

//--------------------------------------------------------------------------------------------------
/**
 * Find the address of a containing structure or union, based on the address of one of its members.
 *
 * If @c countPtr points to the @c count member of an object type
 * @c my_class_t, then a pointer to that object should use this:
 *
 * @code
 * my_class_t* myObjPtr = CONTAINER_OF(countPtr, my_class_t, count);
 * @endcode
 */
//--------------------------------------------------------------------------------------------------
#define CONTAINER_OF(memberPtr, type, member)                                                       \
    ((type*)(((uint8_t*)(memberPtr))-((size_t)(&(((type*)0)->member)))))


//--------------------------------------------------------------------------------------------------
/**
 * Computes  number of members in an array at compile time.
 *
 * @warning Does NOT work for pointers to arrays.
 *
 * Here's a code sample:
 *
 * @code
 * const char message[] = "Hello world!";
 *
 * size_t x = NUM_ARRAY_MEMBERS(message);
 *
 * printf("%lu\n", x);
 * @endcode
 *
 * Will print @c 13 on a 32-bit target.
 *
 * But this example is @b incorrect:
 *
 * @code
 * const char message[] = "Hello world!";
 * const char* messagePtr = &message;
 *
 * size_t x = NUM_ARRAY_MEMBERS(messagePtr);    // NO! BAD PROGRAMMER! BAD!
 *
 * printf("%lu\n", x);
 * @endcode
 */
//--------------------------------------------------------------------------------------------------
#define NUM_ARRAY_MEMBERS(array) \
    (sizeof(array) / sizeof((array)[0]))


//--------------------------------------------------------------------------------------------------
/**
 * Computes the index of a member within an array.
 *
 * This code sample prints out "The 'w' is at index 6.":
 *
 * @code
 * const char message[] = "Hello world!";
 * const char* charPtr;
 * int i;
 *
 * for (i = 0; i < sizeof(message); i++)
 * {
 *     if (message[i] == 'w')
 *     {
 *         charPtr = &message[i];
 *     }
 * }
 *
 * printf("The 'w' is at index %zu.\n", INDEX_OF_ARRAY_MEMBER(message, charPtr));
 * @endcode
 **/
//--------------------------------------------------------------------------------------------------
#define INDEX_OF_ARRAY_MEMBER(array, memberPtr) \
    ((((size_t)memberPtr) - ((size_t)array)) / sizeof(*(memberPtr)))


//--------------------------------------------------------------------------------------------------
/**
 * This function takes the characters as an argument and puts quotes around them.
 *
 * Code sample:
 *
 * @code
 *    const char name[] = STRINGIZE(foo);
 * @endcode
 *
 * Is seen by the compiler as
 *
 * @code
 *    const char name[] = "foo";
 * @endcode
 *
 * The STRINGIZE() macro function passes names like macro definitions
 * on the compiler's command-line.  If the above code were changed to:
 *
 * @code
 *    const char name[] = STRINGIZE(NAME);
 * @endcode
 *
 * then compiling it using the following command line makes it equivalent to the
 * example above:
 *
 * @code
 *    $ gcc -c -DNAME=foo file.c
 * @endcode
 *
 * The <c>-DNAME=foo</c> defines a macro called <c>NAME</c> with a value <c>foo</c>.
 * The C preprocessor then replaces <c>STRINGIZE(NAME)</c> with <c>"foo"</c>.
 */
//--------------------------------------------------------------------------------------------------
#define STRINGIZE(x)  STRINGIZE_EXPAND(x)

//--------------------------------------------------------------------------------------------------
/**
 * Helper macro for @ref STRINGIZE(x).
 */
//--------------------------------------------------------------------------------------------------
#define STRINGIZE_EXPAND(x)     #x   // Needed to expand macros.


//--------------------------------------------------------------------------------------------------
/**
 * Macro used to declare that a symbol should be shared outside the dynamic shared object in
 * which it is defined.
 *
 * This can be used with either a declaration or a definition.
 *
 * E.g., with a declaration (perhaps in a header file):
 *
 * @code
 * LE_SHARED void my_Function();
 * @endcode
 *
 * E.g., with a definition (in a .c file):
 *
 * @code
 * LE_SHARED void my_OtherFunction()
 * {
 *     LE_INFO("Hello world.");
 * }
 * @endcode
 *
 **/
//--------------------------------------------------------------------------------------------------
#define LE_SHARED __attribute__((visibility ("default")))

//--------------------------------------------------------------------------------------------------
/**
 * @macro LE_DECLARE_INLINE
 *
 * Declare an inline function in a header.
 *
 * GNU by default uses a non-standard method of declaring inline functions with the
 * exact *opposite* meaning of the C99 standard.
 *
 * So use LE_DECLARE_INLINE with the function body in a header.  Then use LE_DEFINE_INLINE with
 * just the function prototype in a .c file to tell the compiler to emit the function definition
 * for cases where the function is not inlined.
 *
 * This is preferred over using "static inline" since if a static inline function is not inlined
 * by gcc, there may be multiple copies of the function included in the output.
 */
/**
 * @macro LE_DEFINE_INLINE
 *
 * Cause a function definition to be emitted for an inline function, in case the compiler
 * decides not to use the inline definition.
 *
 * @see @c LE_DECLARE_INLINE
 */
//--------------------------------------------------------------------------------------------------
#ifdef __GNUC_GNU_INLINE__
// Using gcc inline semantics
#  define LE_DECLARE_INLINE extern inline
#  define LE_DEFINE_INLINE inline
#else
// Use C99 inline semantics
#  define LE_DECLARE_INLINE inline
#  define LE_DEFINE_INLINE extern inline
#endif

// Clang feature check macros -- define to return sensible defaults if macro is not available.
#ifndef __is_identifier
#  define __is_identifier(x) 1
#endif

#ifndef __has_warning
#  define  __has_warning(x) 0
#endif

#ifndef __has_attribute
#  define __has_attribute(x) 0
#endif

#if !__is_identifier(_Nonnull)

// Nullability information is not complete.  Normally clang will warn in that case.
#  if __has_warning("-Wnullability-completeness")
#    pragma clang diagnostic ignored "-Wnullability-completeness"
#  endif

//--------------------------------------------------------------------------------------------------
/**
 * Mark a parameter or return value as never NULL.
 */
//--------------------------------------------------------------------------------------------------
#  define LE_NONNULL _Nonnull

//--------------------------------------------------------------------------------------------------
/**
 * Mark a parameter or return value as potentially NULL.
 */
//--------------------------------------------------------------------------------------------------
#  define LE_NULLABLE _Nullable
#else
#  define LE_NONNULL
#  define LE_NULLABLE
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Mark a variable as unused.
 *
 * @param   v   Variable to mark as unused.
 */
//--------------------------------------------------------------------------------------------------
#define LE_UNUSED(v)    ((void) (v))

//--------------------------------------------------------------------------------------------------
/**
 * Format specifiers for size_t
 */
//--------------------------------------------------------------------------------------------------
#if (__STDC_VERSION__ >= 199901L) || LE_CONFIG_LINUX
#   define __PRIS_PREFIX "z"
#elif defined(__LP64__) || defined(_LP64)
#   define __PRIS_PREFIX "ll"
#else
#   define __PRIS_PREFIX "l"
#endif

#define PRIdS __PRIS_PREFIX "d"
#define PRIxS __PRIS_PREFIX "x"
#define PRIuS __PRIS_PREFIX "u"
#define PRIXS __PRIS_PREFIX "X"
#define PRIoS __PRIS_PREFIX "o"


#if !defined(static_assert) && !defined(__cplusplus)
//--------------------------------------------------------------------------------------------------
/**
 * Provide static_assert if not available.
 */
//--------------------------------------------------------------------------------------------------
#   define static_assert(cond, msg) _Static_assert((cond), #cond ": " msg)
#endif
#define inline_static_assert(cond, msg) ({ static_assert(cond, msg); })

//--------------------------------------------------------------------------------------------------
/**
 * Test if a KConfig feature is enabled.
 */
//--------------------------------------------------------------------------------------------------
#define LE_CONFIG_IS_ENABLED(option)    (("" #option)[0] == '1')

#endif // LEGATO_BASICS_INCLUDE_GUARD
