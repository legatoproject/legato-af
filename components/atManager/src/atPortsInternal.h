/** @file atPortsInternal.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATPORTS_LOCAL_INCLUDE_GUARD
#define LEGATO_ATPORTS_LOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set an interface for the given name.
 *
 * @warning This function is a memory leak, it is just use for testing should not be used otherwise.
 *
 */
//--------------------------------------------------------------------------------------------------
void atports_SetInterface
(
    atports_t    name,
    atmgr_Ref_t  interfaceRef
);

#endif /* LEGATO_ATPORTS_LOCAL_INCLUDE_GUARD */
