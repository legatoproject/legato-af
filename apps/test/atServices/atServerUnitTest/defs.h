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
    const char*                         devPathPtr;     ///< device path
    le_sem_Ref_t                        semRef;         ///< semaphore
    le_atServer_DeviceRef_t             devRef;         ///< device reference
    le_thread_Ref_t                     atServerThread; ///< AT Server reference
}
SharedData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Test on an expected result
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t TestResponses
(
    int fd,
    int epollFd,
    const char* expectedResponsePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * send an AT command and test on an expected result
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t SendCommandsAndTest
(
    int fd,
    int epollFd,
    const char* commandsPtr,
    const char* expectedResponsePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * server thread function
 *
 */
//--------------------------------------------------------------------------------------------------
void AtServer
(
    SharedData_t* sharedDataPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to test the AT server bridge feature.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t Testle_atServer_Bridge
(
    int socketFd,
    int epollFd,
    SharedData_t* sharedDataPtr
);

#endif /* defs.h */
