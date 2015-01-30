 /**
  * This module is for unit testing of the modemServices component.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */
#include "main.h"


#define SERVICE_BASE_BINDINGS_CFG  "/users/root/bindings"

typedef void (*LegatoServiceInit_t)(void);

typedef struct {
    const char * appNamePtr;
    const char * serviceNamePtr;
    LegatoServiceInit_t serviceInitPtr;
} ServiceInitEntry_t;

#define SERVICE_ENTRY(aPP, sVC) { aPP, #sVC, sVC##_ConnectService },

typedef void (*LegatoServiceInit_t)(void);


const ServiceInitEntry_t ServiceInitEntries[] = {
    SERVICE_ENTRY("modemService", le_sms)
};


static void SetupBindings(void)
{
    int serviceIndex;
    char cfgPath[512];
    le_cfg_IteratorRef_t iteratorRef;

    /* Setup bindings */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        /* Update binding in config tree */
        LE_INFO("-> Bind %s", entryPtr->serviceNamePtr);

        snprintf(cfgPath, sizeof(cfgPath), SERVICE_BASE_BINDINGS_CFG "/%s", entryPtr->serviceNamePtr);

        iteratorRef = le_cfg_CreateWriteTxn(cfgPath);

        le_cfg_SetString(iteratorRef, "app", entryPtr->appNamePtr);
        le_cfg_SetString(iteratorRef, "interface", entryPtr->serviceNamePtr);

        le_cfg_CommitTxn(iteratorRef);
    }

    /* Tel legato to reload its bindings */
    system("sdir load");
}

static void ConnectServices(void)
{
    int serviceIndex;

    /* Init services */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        LE_INFO("-> Init %s", entryPtr->serviceNamePtr);
        entryPtr->serviceInitPtr();
    }

    LE_INFO("All services bound!");
}


//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
static void test(void* context)
{
    // Init the test case / test suite data structures

    CU_TestInfo smstest[] =
    {
       { "Test le_sms_SetGetSmsCenterAddress()", Testle_sms_SetGetSmsCenterAddress },
       { "Test le_sms_SetGetText()",    Testle_sms_SetGetText },
       { "Test le_sms_SetGetBinary()",  Testle_sms_SetGetBinary },
       { "Test le_sms_SetGetPDU()",     Testle_sms_SetGetPDU },
       { "Test le_sms_ReceivedList()",  Testle_sms_ReceivedList },
       { "Test le_sms_Send_Binary()",    Testle_sms_Send_Binary },
       { "Test le_sms_Send_Text()",      Testle_sms_Send_Text },
       { "Test le_sms_AsyncSendText()",  Testle_sms_AsyncSendText },
#if 0
        // PDU to encode for ASYNC le_sms_SendPdu Async fucntions.
        { "Test le_sms_AsyncSendPdu()",   Testle_sms_AsyncSendPdu },
        { "Test le_sms_Send_Pdu()",       Testle_sms_Send_Pdu },
#endif
        CU_TEST_INFO_NULL,
    };


    CU_SuiteInfo suites[] =
    {
        { "SMS tests",                NULL, NULL, smstest },
        CU_SUITE_INFO_NULL,
    };

    fprintf(stderr, "Please ensure that there is enough space on SIM to receive new SMS messages!\n");

#ifndef AUTOMATIC
    GetTel();
#endif

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
    }
}

/*
 * ME must be registered on Network with the SIM in ready state.
 * Checked the "Run Summary" result:
 *
 */
COMPONENT_INIT
{
    SetupBindings();
    ConnectServices();

    test(NULL);
}
