/**
 * This module implements the unit tests for LPT API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_lpt_simu.h"
#include "le_lpt_local.h"


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * eDRX handler context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_thread_Ref_t                     handlerThreadRef;   ///< Handler thread reference
    le_lpt_EDrxParamsChangeHandlerRef_t handlerRef;         ///< eDRX handler reference
    le_lpt_EDrxRat_t                    rat;                ///< eDRX RAT for the notification
    le_onoff_t                          activation;         ///< eDRX state for the notification
    uint8_t                             eDrxValue;          ///< eDRX value for the notification
    uint8_t                             pagingTimeWindow;   ///< PTW for the notification
}
EDrxHandlerContext_t;


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize initialization
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t InitSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize eDRX thread
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t ThreadSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 * Application handler thread.
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr    ///< [IN] Context pointer.
)
{
    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for notifications of a change in the eDRX parameters.
 */
//--------------------------------------------------------------------------------------------------
void EDrxParamsChangeHandler
(
    le_lpt_EDrxRat_t rat,       ///< [IN] Radio Access Technology.
    le_onoff_t activation,      ///< [IN] eDRX activation state.
    uint8_t eDrxValue,          ///< [IN] eDRX cycle value, defined in 3GPP
                                ///<      TS 24.008 Rel-13 section 10.5.5.32.
    uint8_t pagingTimeWindow,   ///< [IN] Paging Time Window, defined in 3GPP
                                ///<      TS 24.008 Rel-13 section 10.5.5.32.
    void* contextPtr            ///< [IN] Context pointer.
)
{
    EDrxHandlerContext_t* eDrxCtxPtr = (EDrxHandlerContext_t*) contextPtr;

    LE_DEBUG("New eDRX parameters for RAT %d: activation = %c, eDRX value = %d, PTW = %d",
             rat, ((LE_ON == activation) ? 'Y' : 'N'), eDrxValue, pagingTimeWindow);

    LE_ASSERT(eDrxCtxPtr->rat == rat);
    LE_ASSERT(eDrxCtxPtr->activation == activation);
    LE_ASSERT(eDrxCtxPtr->eDrxValue == eDrxValue);
    LE_ASSERT(eDrxCtxPtr->pagingTimeWindow == pagingTimeWindow);

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add eDRX events handler.
 */
//--------------------------------------------------------------------------------------------------
static void AddEDrxHandler
(
    void* param1Ptr,    ///< [IN] First parameter.
    void* param2Ptr     ///< [IN] Second parameter.
)
{
    EDrxHandlerContext_t* eDrxCtxPtr = (EDrxHandlerContext_t*) param1Ptr;

    eDrxCtxPtr->handlerRef = le_lpt_AddEDrxParamsChangeHandler(EDrxParamsChangeHandler, param1Ptr);
    LE_ASSERT(eDrxCtxPtr->handlerRef);

    LE_DEBUG("Added eDRX handler %p", eDrxCtxPtr->handlerRef);

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove eDRX events handler.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveEDrxHandler
(
    void* param1Ptr,    ///< [IN] First parameter.
    void* param2Ptr     ///< [IN] Second parameter.
)
{
    EDrxHandlerContext_t* eDrxCtxPtr = (EDrxHandlerContext_t*) param1Ptr;

    le_lpt_RemoveEDrxParamsChangeHandler(eDrxCtxPtr->handlerRef);

    LE_DEBUG("Removed eDRX handler %p", eDrxCtxPtr->handlerRef);

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: eDRX feature.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_lpt_EDrx
(
    void
)
{
    int i = 0;
    uint8_t eDrxValue = 16;
    uint8_t pagingTimeWindow = 0;
    EDrxHandlerContext_t eDrxCtx;
    le_clk_Time_t timeToWait = {2, 0};

    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_UNKNOWN, LE_ON));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_MAX+1, LE_ON));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_GSM, 3));
    LE_ASSERT_OK(le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_LTE_M1, LE_ON));
    LE_ASSERT_OK(le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_LTE_M1, LE_OFF));

    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_SetRequestedEDrxValue(LE_LPT_EDRX_RAT_UNKNOWN, 0));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_SetRequestedEDrxValue(LE_LPT_EDRX_RAT_MAX+1, 0));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_SetRequestedEDrxValue(LE_LPT_EDRX_RAT_GSM, eDrxValue));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetRequestedEDrxValue(LE_LPT_EDRX_RAT_UNKNOWN,
                                                               &eDrxValue));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetRequestedEDrxValue(LE_LPT_EDRX_RAT_MAX+1, &eDrxValue));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetRequestedEDrxValue(LE_LPT_EDRX_RAT_GSM, NULL));

    for (i = LE_LPT_EDRX_RAT_EC_GSM_IOT; i < LE_LPT_EDRX_RAT_MAX; i++)
    {
        LE_ASSERT(LE_UNAVAILABLE == le_lpt_GetRequestedEDrxValue(i, &eDrxValue));
        LE_ASSERT_OK(le_lpt_SetRequestedEDrxValue(i, i));
        LE_ASSERT_OK(le_lpt_GetRequestedEDrxValue(i, &eDrxValue));
        LE_ASSERT(i == eDrxValue);
    }

    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetNetworkProvidedEDrxValue(LE_LPT_EDRX_RAT_UNKNOWN,
                                                                     &eDrxValue));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetNetworkProvidedEDrxValue(LE_LPT_EDRX_RAT_MAX+1,
                                                                     &eDrxValue));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetNetworkProvidedEDrxValue(LE_LPT_EDRX_RAT_GSM, NULL));
    LE_ASSERT_OK(le_lpt_GetNetworkProvidedEDrxValue(LE_LPT_EDRX_RAT_GSM, &eDrxValue));

    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetNetworkProvidedPagingTimeWindow(LE_LPT_EDRX_RAT_UNKNOWN,
                                                                            &pagingTimeWindow));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetNetworkProvidedPagingTimeWindow(LE_LPT_EDRX_RAT_MAX+1,
                                                                            &pagingTimeWindow));
    LE_ASSERT(LE_BAD_PARAMETER == le_lpt_GetNetworkProvidedPagingTimeWindow(LE_LPT_EDRX_RAT_GSM,
                                                                            NULL));
    LE_ASSERT_OK(le_lpt_GetNetworkProvidedPagingTimeWindow(LE_LPT_EDRX_RAT_GSM, &pagingTimeWindow));

    // Create a semaphore and an application thread to test notifications
    ThreadSemaphore = le_sem_Create("HandlerSem", 0);
    eDrxCtx.handlerThreadRef = le_thread_Create("HandlerThread", AppHandler, NULL);
    le_thread_Start(eDrxCtx.handlerThreadRef);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    // Add a handler for eDRX parameters change indications
    le_event_QueueFunctionToThread(eDrxCtx.handlerThreadRef, AddEDrxHandler, &eDrxCtx, NULL);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    // Simulate eDRX events
    eDrxCtx.rat = LE_LPT_EDRX_RAT_GSM;
    eDrxCtx.activation = LE_OFF;
    eDrxCtx.eDrxValue = 0;
    eDrxCtx.pagingTimeWindow = 0;
    pa_lptSimu_ReportEDrxParamsChange(eDrxCtx.rat,
                                      eDrxCtx.activation,
                                      eDrxCtx.eDrxValue,
                                      eDrxCtx.pagingTimeWindow);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    eDrxCtx.rat = LE_LPT_EDRX_RAT_LTE_M1;
    eDrxCtx.activation = LE_ON;
    eDrxCtx.eDrxValue = 7;
    eDrxCtx.pagingTimeWindow = 10;
    pa_lptSimu_ReportEDrxParamsChange(eDrxCtx.rat,
                                      eDrxCtx.activation,
                                      eDrxCtx.eDrxValue,
                                      eDrxCtx.pagingTimeWindow);

    // Remove the handler
    le_event_QueueFunctionToThread(eDrxCtx.handlerThreadRef, RemoveEDrxHandler, &eDrxCtx, NULL);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    // Simulate an event
    pa_lptSimu_ReportEDrxParamsChange(LE_LPT_EDRX_RAT_GSM, LE_OFF, 0, 0);
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    // Clean up
    LE_ASSERT_OK(le_thread_Cancel(eDrxCtx.handlerThreadRef));
    le_sem_Delete(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * UnitTestInit thread: this function initializes the test and runs an eventLoop.
 */
//--------------------------------------------------------------------------------------------------
static void* UnitTestInit
(
    void* ctxPtr
)
{
    // Initialize simulated PA
    pa_lptSimu_Init();
    // Initialize LPT service
    le_lpt_Init();

    le_sem_Post(InitSemaphore);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t initThreadRef = le_thread_Create("UnitTestInit", UnitTestInit, NULL);
    InitSemaphore = le_sem_Create("InitSem", 0);
    le_thread_Start(initThreadRef);
    le_sem_Wait(InitSemaphore);

    LE_INFO("======== Start UnitTest of LPT API ========");

    LE_INFO("======== eDRX Test ========");
    Testle_lpt_EDrx();
    LE_INFO("======== eDRX Test PASSED ========");

    LE_INFO("======== UnitTest of LPT API ends with SUCCESS ========");

    // Clean-up
    le_thread_Cancel(initThreadRef);
    le_sem_Delete(InitSemaphore);

    exit(EXIT_SUCCESS);
}
