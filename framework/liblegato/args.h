/**
 * @file args.h
 *
 * Header file for internal definitions relating to le_arg module.
 */

#ifndef ARGS_INCLUDE_GUARD
#define ARGS_INLCUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the le_arg module
 */
//--------------------------------------------------------------------------------------------------
void arg_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Release the argument info (if any) for the current thread.
 */
//--------------------------------------------------------------------------------------------------
void arg_DestructThread
(
    void
);

#endif /* ARGS_INCLUDE_GUARD */
