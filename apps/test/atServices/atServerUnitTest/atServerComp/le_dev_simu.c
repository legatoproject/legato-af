/** @file le_dev.c
 *
 * Implementation of device access stub.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"

le_fdMonitor_HandlerFunc_t HandlerFunc;
le_sem_Ref_t  Semaphore;
le_sem_Ref_t  SemaphoreMain;
le_sem_Ref_t  SemaphoreRsp;
void* HandlerContext;
le_thread_Ref_t DevThreadRef;
char ExpectedResponse[LE_ATSERVER_COMMAND_MAX_BYTES];

uint8_t  RxDataPtr[LE_ATSERVER_COMMAND_MAX_LEN];
uint32_t   RxDataLen;

//--------------------------------------------------------------------------------------------------
/**
 * Stub for le_fdMonitor_GetContextPtr
 *
 */
//--------------------------------------------------------------------------------------------------
void* My_fdMonitor_GetContextPtr
(
    void
)
{
    return HandlerContext;
}

//--------------------------------------------------------------------------------------------------
/**
 * Warn le_atServer that data are ready to be read
 *
 */
//--------------------------------------------------------------------------------------------------
static void PollinInt
(
    void* param1Ptr,
    void* param2Ptr
)
{
    HandlerFunc (1, POLLIN);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Read
(
    Device_t  *devicePtr,    ///< device pointer
    uint8_t   *rxDataPtr,    ///< Buffer where to read
    uint32_t   size          ///< size of buffer
)
{
    LE_ASSERT( size >= RxDataLen );

    memcpy( rxDataPtr, RxDataPtr, RxDataLen );

    LE_INFO("Receive: %s", rxDataPtr);

    le_sem_Post(Semaphore);

    return RxDataLen;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Write
(
    Device_t *devicePtr,    ///< device pointer
    uint8_t  *txDataPtr,    ///< Buffer to write
    uint32_t  size          ///< size of buffer
)
{
    char string[size+1];

    memset(string,0,size+1);
    snprintf(string, size+1, "%s", txDataPtr);

    LE_INFO("Send: %s", string);
    LE_INFO("ExpectedResponse: %s", ExpectedResponse);

    LE_ASSERT(strncmp(ExpectedResponse, (char*) txDataPtr, strlen((char*) txDataPtr) ) == 0);

    le_sem_Post(SemaphoreMain);

    // Wait provision of the next rsp
    le_sem_Wait(SemaphoreRsp);

    LE_ASSERT(le_sem_GetValue(SemaphoreRsp) == 0);

    return size;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to open a device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_Open
(
    Device_t *devicePtr,    ///< device pointer
    le_fdMonitor_HandlerFunc_t handlerFunc, ///< [in] Handler function.
    void* contextPtr
)
{
    DevThreadRef = le_thread_GetCurrent();
    HandlerFunc = handlerFunc;
    HandlerContext = contextPtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to close a device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_Close
(
    Device_t *devicePtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision the data which will be read by le_atServer
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_NewData
(
    char* stringPtr,
    uint32_t len
)
{
    LE_ASSERT(len <= LE_ATSERVER_COMMAND_MAX_LEN);

    memcpy(RxDataPtr, stringPtr, len);
    RxDataLen = len;

    le_event_QueueFunctionToThread(DevThreadRef, PollinInt, NULL, NULL);

    le_sem_Wait(Semaphore);
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the expected response
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ExpectedResponse
(
    char* rspPtr
)
{
    static bool first = true;
    memset(ExpectedResponse,0,LE_ATSERVER_COMMAND_MAX_BYTES);
    strncpy(ExpectedResponse, rspPtr, LE_ATSERVER_COMMAND_MAX_BYTES);
    if (!first)
    {
        le_sem_Post(SemaphoreRsp);
    }
    first=false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Wait for device semaphore
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_WaitSemaphore
(
    void
)
{
    le_sem_Wait(SemaphoreMain);
    LE_ASSERT(le_sem_GetValue(SemaphoreMain) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * device stub initialization
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Init
(
    void
)
{
    Semaphore = le_sem_Create("DevSem",0);
    SemaphoreRsp = le_sem_Create("DevSemRsp",0);
    SemaphoreMain = le_sem_Create("DevSemMain",0);
}
