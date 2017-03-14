/**
 * defs.h implemets shared definitions, variables, ... between main and server
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef _DEFS_H
#define _DEFS_H

#include "legato.h"
#include "le_atServer_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * SharedData_t definition
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*     devPathPtr;     ///< device path
    le_sem_Ref_t    semRef;         ///< semaphore
    le_atServer_DeviceRef_t devRef; ///< device reference
    le_thread_Ref_t atServerThread; ///< AT Server reference
}
SharedData_t;

#endif /* defs.h */
