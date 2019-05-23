/**
 * @page c_cdata Component Data API
 *
 * @subpage le_cdata.h "API Reference" <br>
 * <hr>
 *
 * In some contexts (e.g. on real-time operating systems) a single process may contain multiple
 * instances of a component.  Ordinary global or static variables will be shared across all
 * instances of the component in the process.  The Legato Component Data API provides a method
 * to associate data with a specific component instance.
 *
 * To create per-component instance data, use the LE_CDATA_DECLARE macro to declare your
 * per-instance data at file scope:
 *
 * @code
 * LE_CDATA_DECLARE({
 *     int numFoo;
 *     char* fooStr;
 * });
 * @endcode
 *
 * Only one component instance data can be declared per-file, and this data is only available
 * within the file where it's declared.
 *
 * Then use @c LE_CDATA_THIS to access the current component instance's data.  For example:
 * @code
 * LE_CDATA_THIS->numFoo = 5;
 * @endcode
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/**
 * @file le_cdata.h
 *
 * Legato @ref c_cdata include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_CDATA_INCLUDE_GUARD
#define LEGATO_CDATA_INCLUDE_GUARD

#ifndef LE_CDATA_COMPONENT_COUNT
// If legato.h is used in a program built outside of the mktools framework there's only one
// instance of each component.
#define LE_CDATA_COMPONENT_COUNT 1
#endif

/**
 * Define per-component instance data.
 */
#define LE_CDATA_DECLARE(x) static struct x _le_cdata_Instance[LE_CDATA_COMPONENT_COUNT]

#if LE_CDATA_COMPONENT_COUNT == 1
/* Only one instance of this component; macros refer to a simple structure.
 * This is optimized to avoid TLS lookup if there's only one instance of the component,
 * which is the most common case. */
/**
 * Fetch per-instance data for the current instance.
 */
#  define LE_CDATA_THIS       (&_le_cdata_Instance[0])
#else
/**
 * Fetch per-instance data for the current instance.
 */
#  define LE_CDATA_THIS       (&_le_cdata_Instance[le_cdata_GetInstance(LE_CDATA_KEY)])
#endif

/**
 * Get this component instance.
 *
 * @note This should typically not be used by a user application; use LE_CDATA_THIS instead.
 */
unsigned int le_cdata_GetInstance(unsigned int componentKey);

#endif /* LEGATO_CDATA_INCLUDE_GUARD */
