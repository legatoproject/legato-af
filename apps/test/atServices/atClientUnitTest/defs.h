/**
 * defs.h implemets shared definitions, variables, ... between main and client
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef _DEFS_H
#define _DEFS_H

#include "legato.h"
#include "le_atClient_interface.h"

#define DSIZE                      1024               // default buffer size

//--------------------------------------------------------------------------------------------------
/**
 * SharedData_t definition
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                    devPathPtr;        //< device path
    le_sem_Ref_t                   semRef;            //< semaphore
    le_atClient_DeviceRef_t        devRef;            //< device reference
    le_thread_Ref_t                atClientThread;    //< AT Server reference
}
SharedData_t;

//--------------------------------------------------------------------------------------------------
/**
 * AtClientServer function to receive the response from the server
 *
 */
//--------------------------------------------------------------------------------------------------
void AtClientServer
(
    SharedData_t* sharedDataPtr
);

#endif /* defs.h */
