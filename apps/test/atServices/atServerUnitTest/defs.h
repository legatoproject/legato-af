/**
 * defs.h implemets shared definitions, variables, ... between main and server
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#ifndef _DEFS_H
#define _DEFS_H

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * SharedData_t definition
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*     devPathPtr;     ///< device path
    pthread_mutex_t mutex;          ///< used for sync
    pthread_cond_t  cond;           ///< signal connection is ready
    bool            ready;          ///< ready for connection
}
SharedData_t;

#endif /* defs.h */
