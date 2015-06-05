/**
 * @file mdcUnitaryTest.c
 *
 * Unitary test for of @ref pa_mdc API.
 *
 *
 * TEST1: basic tests around le_mdc_GetProfile
 * 1.1: Try to get a profile
 * 1.2: Allocate the same profile as in test 1.1: le_mdc shouldn't allocate a new profile but returns
 * the profile allocated into 1.1
 * 1.3: allocate 3gpp2 profiles
 *
 *
 * TEST2: Get profile ans subscribe handler
 *
 * TEST3: test le_mdc_GetProfile with default profile
 * 3.1: No profile is allocated. Try to get the default profile. Check that the cid used is 1.
 * 3.2: Get the profile using the cid 1. Check that the profile reference is the same as in the previous
 * test.
 * 3.3: Get a default progile when the selected RAT is CDMA: the cid used is 101.
 *
 * TEST4: test le_mdc_StartSession/le_mdc_StopSession API
 * 4.1: Get a profile using le_mdc_GetProfile, then try to open a session using le_mdc_StartSession
 * result code should be LE_OK, internal callRef mustn't be null
 * 4.2: try again to start the same profile: result code should be an error, internal callRef mustn't
 * by modified
 * 4.3: stop the session using le_mdc_StopSession: return code is LE_OK
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */



#include "le_mdc_interface.h"
#include "le_mrc_interface.h"
#include "pa_mdc.h"
#include "le_mdc_local.h"


le_msg_ServiceRef_t le_mdc_GetServiceRef
(
    void
)
{
    return (le_msg_ServiceRef_t) 0x10000001;
}

le_msg_SessionRef_t le_mdc_GetClientSessionRef
(
    void
)
{
    return (le_msg_SessionRef_t) 0x10000001;
}

le_msg_SessionEventHandlerRef_t stub_le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return (le_msg_SessionEventHandlerRef_t) 0x10000002;
}

le_mrc_Rat_t CurrentRat = LE_MRC_RAT_GSM;

/* Stub GetRAT to return an internal RAT */
le_result_t le_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr  ///< [OUT] The Radio Access Technology.
)
{
    *ratPtr = CurrentRat;
    return LE_OK;
}

#include "le_mdc.c"

/* TEST 1 */
void test1( void )
{
    uint32_t i;
    le_mdc_ProfileRef_t profileTmp;
    le_mdc_ProfileRef_t profileRef[PA_MDC_MAX_PROFILE] = {0};

    /* TEST 1.1 */
    /* get a 3gpp profile */
    profileRef[0] = le_mdc_GetProfile(1);

    LE_ASSERT(profileRef[0] != NULL);

    /* TEST 1.2*/
    /* get the profile again: try it several times */
    for (i = 0; i < 5; i++)
    {
        profileTmp = le_mdc_GetProfile(1);

        LE_ASSERT(profileTmp == profileRef[0]);
    }

    /* TEST 1.3 */
    /* get 3gpp2 profiles: we should be able to allocate all profile */
    for (i = PA_MDC_MIN_INDEX_3GPP2_PROFILE; i < PA_MDC_MAX_INDEX_3GPP2_PROFILE-1; i++)
    {
         profileRef[1+i] = le_mdc_GetProfile(i);

        LE_ASSERT(profileRef[1+i] != NULL);
    }

    LE_INFO("Test 1 passed");
}


void handlerFunc
(
    bool isConnected,
    void* contextPtr
)
{

}

/* TEST 2 */
void test2( void )
{
    le_mdc_ProfileRef_t profile;
    le_mdc_SessionStateHandlerRef_t sessionStateHandler1,sessionStateHandler2;

    /* Allocate profile */
    profile = le_mdc_GetProfile(1);

    /* Add handlers */
    sessionStateHandler1 = le_mdc_AddSessionStateHandler (profile, handlerFunc, NULL);
    sessionStateHandler2 = le_mdc_AddSessionStateHandler (profile, handlerFunc, NULL);
    LE_ASSERT(sessionStateHandler1 != NULL);
    LE_ASSERT(sessionStateHandler2 != NULL);

    /* Remove the handler 1 */
    le_mdc_RemoveSessionStateHandler(sessionStateHandler1);

    /* Remove the handler 2 */
    le_mdc_RemoveSessionStateHandler(sessionStateHandler2);


    LE_INFO("Test 2 passed");
}

/* TEST 3 */
void test3( void )
{
    /* TEST 3.1 */
    /* allocate a profile */
    le_mdc_ProfileRef_t profileRef1, profileRef2;

    le_mdc_Profile_t* profilePtr;

    profileRef1 = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);
    LE_ASSERT(profileRef1 != NULL);
    LE_ASSERT(le_mdc_GetProfileIndex(profileRef1) == PA_MDC_MIN_INDEX_3GPP_PROFILE);

    /* check internal handler */
    profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef1);
    LE_ASSERT(profilePtr->profileIndex == 1);

    /* TEST 3.2 */
    profileRef2 = le_mdc_GetProfile(1);
    LE_ASSERT(profileRef1 == profileRef1);
    LE_ASSERT(le_mdc_GetProfileIndex(profileRef2) == PA_MDC_MIN_INDEX_3GPP_PROFILE);

    /* TEST 3.3 */
    /* allocate a profile on 3GPP2 */
    CurrentRat = LE_MRC_RAT_CDMA;
    profileRef1 = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);

    /* check internal handler */
    profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef1);
    LE_ASSERT(profilePtr->profileIndex==101);

    LE_INFO("Test 3 passed");
}

/* TEST 4 */
void test4( void )
{
    le_mdc_ProfileRef_t profile;
    le_mdc_Profile_t* profilePtr;
    void* callRefPtr;
    le_result_t result;

    CurrentRat = LE_MRC_RAT_GSM;

    /* TEST 4.1 */
    /* Allocate profile */
    profile = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);

    /* start the session associated to the profile */
    result = le_mdc_StartSession(profile);
    LE_ASSERT(result == LE_OK);

    /* check internal handler */
    profilePtr = le_ref_Lookup(DataProfileRefMap, profile);
    LE_ASSERT(profilePtr->callRef != NULL);
    callRefPtr = profilePtr->callRef;

    /* TEST 4.2 */
    /* start again the profile */
    result = le_mdc_StartSession(profile);
    LE_ASSERT(result == LE_FAULT);
    profilePtr = le_ref_Lookup(DataProfileRefMap, profile);

    /* check that the callRef is not modified */
    LE_ASSERT(profilePtr->callRef == callRefPtr);

    /* TEST 4.3 */
    /* stop the session */
    result = le_mdc_StopSession(profile);
    LE_ASSERT(result == LE_OK);

    LE_INFO("Test 4 passed");
}





/* main() of the tests */
COMPONENT_INIT
{
    le_log_SetFilterLevel(LE_LOG_DEBUG);

    /* init the service */
    le_mdc_Init();

    /* TEST 1 */
    test1();

    /* TEST 2 */
    test2();

    /* TEST 3 */
    test3();

    /* TEST 4 */
    test4();

    exit(EXIT_SUCCESS);
}
