//--------------------------------------------------------------------------------------------------
/** @file args.h
 *
 * Legato Command Line Argument module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#ifndef LEGATO_SRC_ARGS_INCLUDE_GUARD
#define LEGATO_SRC_ARGS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Sets argc and argv for later use.  This function must be called by Legato's generated main
 * function.
 */
//--------------------------------------------------------------------------------------------------
void arg_SetArgs
(
    const size_t    argc,   ///< [IN] argc from main.
    char**          argv    ///< [IN] argv from main.
);

#endif  // LEGATO_SRC_ARGS_INCLUDE_GUARD
