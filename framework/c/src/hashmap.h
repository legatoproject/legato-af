/** @file hashmap.h
 *
 * This file contains definitions exported by the Hash Map module to other modules inside
 * the Legato framework.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef _LEGATO_HASHMAP_H_INCLUDE_GUARD
#define _LEGATO_HASHMAP_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Hash Map module.  This function must be called at start-up, before any other
 * Hash Map functions are called.
 **/
//--------------------------------------------------------------------------------------------------
void hashmap_Init
(
    void
);

#endif // _LEGATO_HASHMAP_H_INCLUDE_GUARD
