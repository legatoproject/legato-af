/**
 * @page c_print Print APIs
 *
 * @subpage le_print.h "API Reference"
 *
 * <HR>
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

//--------------------------------------------------------------------------------------------------
/** @file le_print.h
 * *
 * Legato @ref c_print include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef LEGATO_PRINT_INCLUDE_GUARD
#define LEGATO_PRINT_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Print a single value using specified format.
 *
 * @param formatSpec @ref LE_DEBUG format; must be a string literal
 * @param value Variable name; must not be a literal
 */
//--------------------------------------------------------------------------------------------------
#define LE_PRINT_VALUE(formatSpec, value)                                                           \
    LE_DEBUG( STRINGIZE(value) "=" formatSpec, value)

//--------------------------------------------------------------------------------------------------
/**
 * Print a single value using specified format, if condition is met.
 *
 * @param condition Boolean
 * @param formatSpec @ref LE_DEBUG format; must be a string literal
 * @param value Variable name; must not be a literal
 */
//--------------------------------------------------------------------------------------------------
#define LE_PRINT_VALUE_IF(condition, formatSpec, value)                                             \
    if (condition)                                                                                  \
        { LE_PRINT_VALUE(formatSpec, value); }

//--------------------------------------------------------------------------------------------------
/**
 * Print an array of values using specified format for each value.
 *
 * @param formatSpec @ref LE_DEBUG format; must be a string literal
 * @param size Array size
 * @param array Array name
 */
//--------------------------------------------------------------------------------------------------
#define LE_PRINT_ARRAY(formatSpec, size, array)                                                     \
    {                                                                                               \
        int i;                                                                                      \
        LE_DEBUG( STRINGIZE(array) );                                                               \
        for (i=0; i<(size_t)size; i++)                                                              \
            LE_DEBUG( "\t" STRINGIZE(array) "[%i]=" formatSpec, i, array[i]);                       \
    }

#endif // LEGATO_PRINT_INCLUDE_GUARD

