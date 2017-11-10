 /**
  * This module implements the LPT integration tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize threads.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t ThreadSemaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * eDRX parameters change handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_lpt_EDrxParamsChangeHandlerRef_t EDrxHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Thread for eDRX parameters change notifications.
 */
//--------------------------------------------------------------------------------------------------
static void* MyEDrxParamsChangeThread
(
    void* ctxPtr    ///< [IN] Context pointer.
)
{
    le_lpt_ConnectService();

    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();

    return NULL;
}

//! [eDRX handler]
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
    LE_INFO("New eDRX parameters for RAT %d: activation = %c, eDRX value = %d, PTW = %d",
            rat, ((LE_ON == activation) ? 'Y' : 'N'), eDrxValue, pagingTimeWindow);

    le_sem_Post(ThreadSemaphore);
}
//! [eDRX handler]

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
    //! [Add eDRX handler]
    le_lpt_EDrxParamsChangeHandlerRef_t handlerRef;

    handlerRef = le_lpt_AddEDrxParamsChangeHandler(EDrxParamsChangeHandler, NULL);
    LE_ASSERT(handlerRef);
    //! [Add eDRX handler]

    EDrxHandlerRef = handlerRef;
    LE_DEBUG("Added eDRX handler %p", EDrxHandlerRef);

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
    le_lpt_RemoveEDrxParamsChangeHandler(EDrxHandlerRef);
    LE_DEBUG("Removed eDRX handler %p", EDrxHandlerRef);

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: eDRX feature.
 *
 * Tested APIs:
 *  - le_lpt_AddEDrxParamsChangeHandler() / le_lpt_RemoveEDrxParamsChangeHandler()
 *  - le_lpt_SetEDrxState()
 *  - le_lpt_GetRequestedEDrxValue() / le_lpt_SetRequestedEDrxValue()
 *  - le_lpt_GetNetworkProvidedEDrxValue()
 *  - le_lpt_GetNetworkProvidedPagingTimeWindow()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_lpt_EDrx
(
    void
)
{
    int i = 0;
    le_thread_Ref_t handlerThreadRef;
    le_clk_Time_t timeToWait = {30, 0};
    uint8_t value = 0;

    // Check if the platform supports the eDRX feature
    if (LE_UNSUPPORTED == le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_LTE_M1, LE_OFF))
    {
        // eDRX is not supported, no need to run the tests
        return;
    }

    ThreadSemaphore = le_sem_Create("HandlerSem", 0);
    handlerThreadRef = le_thread_Create("HandlerThread", MyEDrxParamsChangeThread, NULL);
    le_thread_Start(handlerThreadRef);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    // Add a handler for eDRX parameters change indications
    le_event_QueueFunctionToThread(handlerThreadRef, AddEDrxHandler, NULL, NULL);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    //! [Set state]
    LE_ASSERT_OK(le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_LTE_M1, LE_OFF));
    LE_ASSERT_OK(le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_LTE_M1, LE_ON));
    //! [Set state]

    for (i = LE_LPT_EDRX_RAT_EC_GSM_IOT; i < LE_LPT_EDRX_RAT_MAX; i++)
    {
        LE_ASSERT(LE_UNAVAILABLE == le_lpt_GetRequestedEDrxValue(i, &value));
        LE_ASSERT_OK(le_lpt_SetRequestedEDrxValue(i, i));
        LE_ASSERT_OK(le_lpt_GetRequestedEDrxValue(i, &value));
        LE_ASSERT(i == value);
    }

    //! [eDRX value]
    LE_ASSERT_OK(le_lpt_SetRequestedEDrxValue(LE_LPT_EDRX_RAT_LTE_M1, 1));

    uint8_t eDrxValue = 0;
    LE_ASSERT_OK(le_lpt_GetRequestedEDrxValue(LE_LPT_EDRX_RAT_LTE_M1, &eDrxValue));
    LE_INFO("Requested eDRX cycle value for LTE M1: %d", eDrxValue);
    //! [eDRX value]
    LE_ASSERT(1 == eDrxValue);

    //! [NP eDRX value]
    uint8_t npEDrxValue = 0;
    LE_ASSERT_OK(le_lpt_GetNetworkProvidedEDrxValue(LE_LPT_EDRX_RAT_LTE_M1, &npEDrxValue));
    LE_INFO("Network-provided eDRX cycle value for LTE M1: %d", npEDrxValue);
    //! [NP eDRX value]

    //! [NP PTW]
    uint8_t pagingTimeWindow = 0;
    LE_ASSERT_OK(le_lpt_GetNetworkProvidedPagingTimeWindow(LE_LPT_EDRX_RAT_LTE_M1,
                                                           &pagingTimeWindow));
    LE_INFO("Network-provided Paging Time Window for LTE M1: %d", pagingTimeWindow);
    //! [NP PTW]

    // Wait for an eDRX event
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));

    // Remove the handler
    le_event_QueueFunctionToThread(handlerThreadRef, RemoveEDrxHandler, NULL, NULL);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 * Main
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== Start LPT Modem Services implementation Test ========");

    LE_INFO("======== eDRX Test ========");
    Testle_lpt_EDrx();
    LE_INFO("======== eDRX Test PASSED ========");

    LE_INFO("======== Test LPT Modem Services implementation Test SUCCESS ========");

    exit(EXIT_SUCCESS);
}
