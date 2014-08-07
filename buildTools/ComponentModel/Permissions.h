//--------------------------------------------------------------------------------------------------
/**
 * File permissions typedefs.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PERMISSIONS_H_INCLUDE_GUARD
#define PERMISSIONS_H_INCLUDE_GUARD

namespace legato
{

//--------------------------------------------------------------------------------------------------
/**
 * Permissions flags that can be bitwise or'd together into an integer.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PERMISSION_READABLE   = 1,
    PERMISSION_WRITEABLE  = 2,
    PERMISSION_EXECUTABLE = 4,
}
permission_Flags_t;


}

#endif // PERMISSIONS_H_INCLUDE_GUARD
