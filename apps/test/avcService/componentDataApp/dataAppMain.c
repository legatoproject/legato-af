//--------------------------------------------------------------------------------------------------
/**
 * @file dataAppMain.c
 *
 * This component is used for testing the AirVantage Data API.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

#include "le_print.h"


// Uncomment for overflow testing
// #define OVERFLOW_TESTING

// Uncomment for testing apn configuration apis
// #define VERIFY_APN_CONFIG

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for server reads of variableStringOne variable field
 */
//--------------------------------------------------------------------------------------------------
static void Field_variableStringOne_Handler
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    char strBuf[LE_AVDATA_STRING_VALUE_LEN];
    static int32_t simpleCount = 0;

    LE_INFO("Registered handler called for %s", fieldName);

    simpleCount++;

    sprintf(strBuf, "%d", simpleCount);
    LE_WARN("Asset Value =  %s", strBuf);

    // Write the result to the assetData. The assetData handler will send this response back to
    // the server
    le_avdata_SetString(instRef, "variableStringOne", strBuf);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for server writes of settingStringOne setting field
 */
//--------------------------------------------------------------------------------------------------
static void Field_settingStringOne_Handler
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    char strBuf[LE_AVDATA_STRING_VALUE_LEN];

    LE_INFO("Registered handler called for %s", fieldName);

    // Log the value written by the server
    le_avdata_GetString(instRef, "settingStringOne", strBuf, sizeof(strBuf));
    LE_PRINT_VALUE("%s", strBuf);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for server Reads of variableFloatOne
 * ToDO: This will only work on the second read.
 *       Verify this after #3079 Implement server read callback for AV data, is merged
 */
//--------------------------------------------------------------------------------------------------
static void Field_variableFloatOne_Handler
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    LE_INFO("Registered handler called for %s", fieldName);
    le_avdata_SetFloat(instRef, "variableFloatOne", 532.212);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for server execute on commandOne field
 */
//--------------------------------------------------------------------------------------------------
static void Field_executeCommandOne_Handler
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    LE_INFO("Registered handler called for %s", fieldName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for server execute on commandTwo field
 */
//--------------------------------------------------------------------------------------------------
static void Field_executeCommandTwo_Handler
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    LE_INFO("Registered handler called for %s", fieldName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Status handler
 */
//--------------------------------------------------------------------------------------------------
static void StatusHandler
(
    le_avc_Status_t updateStatus,
    int32_t totalNumBytes,
    int32_t downloadProgress,
    void* contextPtr
)
{
    LE_ERROR("Got status %i", updateStatus);
    switch ( updateStatus )
    {
        case LE_AVC_DOWNLOAD_PENDING:
               LE_WARN("Accept download");
               LE_ASSERT( le_avc_AcceptDownload() == LE_OK );
               break;

           case LE_AVC_INSTALL_PENDING:
               LE_WARN("Accept install");
               LE_ASSERT( le_avc_AcceptInstall() == LE_OK );
               break;

           case LE_AVC_UNINSTALL_PENDING:
               LE_WARN("Accept uninstall");
               LE_ASSERT( le_avc_AcceptUninstall() == LE_OK );
               break;

           default:
               LE_WARN("Update status %i not handled", updateStatus);
    }
}


#ifdef VERIFY_APN_CONFIG

//--------------------------------------------------------------------------------------------------
/**
 * Verify Retry timer API
 */
//--------------------------------------------------------------------------------------------------
static void VerifyRetryTimer
(
    uint16_t* testRetryTimerPtr,
    size_t testNumTimers
)
{
    int i;
    le_result_t rc;
    size_t numTimers;
    uint16_t retryTimer[testNumTimers];

    rc = le_avc_SetRetryTimers(testRetryTimerPtr, testNumTimers);
    if (rc != LE_OK)
    {
        LE_ERROR("Failed to write the retry timers.");
    }

    numTimers = testNumTimers;
    rc = le_avc_GetRetryTimers(retryTimer, &numTimers);

    if (rc != LE_OK
        || numTimers != testNumTimers)
    {
        LE_ERROR("Failed reading retry timer.");
    }

    for (i = 0; i < testNumTimers; i++)
    {
        LE_DEBUG("retryTimer[%d] = %d", i, retryTimer[i]);

        if(testRetryTimerPtr[i] != retryTimer[i])
        {
            LE_ERROR("Retry Timer test failed.");
            return;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Verify APN config API
 */
//--------------------------------------------------------------------------------------------------
static void VerifyApnConfig
(
    const char* testApnNamePtr,
    const char* testUserNamePtr,
    const char* testUserPasswordPtr
)
{
    char apnName[LE_AVC_APN_NAME_MAX_LEN_BYTES];
    char userName[LE_AVC_USERNAME_MAX_LEN_BYTES];
    char password[LE_AVC_PASSWORD_MAX_LEN_BYTES];

    le_result_t rc;

    rc = le_avc_SetApnConfig(testApnNamePtr, testUserNamePtr, testUserPasswordPtr);
    if (rc != LE_OK)
    {
        LE_ERROR("Failed to write APN Config.");
    }

    rc = le_avc_GetApnConfig(apnName, sizeof(apnName),
                             userName, sizeof(userName),
                             password, sizeof(password));
    if (rc != LE_OK)
    {
        LE_ERROR("Failed to read APN config.");
    }

    if (strcmp(apnName, testApnNamePtr) != 0
           || strcmp(userName, testUserNamePtr) != 0
           || strcmp(password, testUserPasswordPtr) != 0)
    {
        LE_ERROR("APN Config test failed.");
        LE_DEBUG("APN Name : %s", apnName);
        LE_DEBUG("User Name %s: ", userName);
        LE_DEBUG("Password : %s", password);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Verify polling timer API
 */
//--------------------------------------------------------------------------------------------------
static void VerifyPollingTimer
(
    uint32_t testValue
)
{
    le_result_t rc;
    uint32_t pollingTimer;

    rc = le_avc_SetPollingTimer(testValue);
    if (rc != LE_OK)
    {
        LE_ERROR("Failed reading polling timer.");
    }

    rc = le_avc_GetPollingTimer(&pollingTimer);
    if (rc != LE_OK)
    {
        LE_ERROR("Failed reading polling timer.");
    }
    LE_PRINT_VALUE("%d", pollingTimer);

    if (pollingTimer != testValue)
    {
        LE_ERROR("Polling Timer test failed.");
    }
}

#endif

//--------------------------------------------------------------------------------------------------
/**
 * Writes periodically to variable fields of asset data.
 */
//--------------------------------------------------------------------------------------------------
static void SampleTimer
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    static int variableIntOneCount = 0;
    le_avdata_AssetInstanceRef_t instOneRef;

    instOneRef = (le_avdata_AssetInstanceRef_t) le_timer_GetContextPtr(timerRef);


    le_avdata_SetInt(instOneRef, "variableIntOne", variableIntOneCount++);
}


//--------------------------------------------------------------------------------------------------
/**
 * Init the component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_avdata_AssetInstanceRef_t instOneRef;
    le_avdata_AssetInstanceRef_t instZeroRef;
    char strBuf[LE_AVDATA_STRING_VALUE_LEN];

    le_timer_Ref_t sampleTimerRef;

    int value;
    bool bValue;

    instZeroRef = le_avdata_Create("myData");
    instOneRef = le_avdata_Create("myData");
    LE_PRINT_VALUE("%p", instZeroRef);
    LE_PRINT_VALUE("%p", instOneRef);

    /*
     * Test variable fields
     */

    // Register handlers that will actually set the value of the 'variable' field on read
    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "variableStringOne",
                                   Field_variableStringOne_Handler,
                                   NULL);

    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "variableFloatOne",
                                   Field_variableFloatOne_Handler,
                                   NULL);

    // Set initial values.
    le_avdata_SetString(instZeroRef, "variableStringOne", "field value for instance zero");
    le_avdata_SetInt(instZeroRef, "variableIntOne", 123);
    le_avdata_SetBool(instZeroRef, "variableBoolOne", true);

    le_avdata_SetString(instOneRef, "variableStringOne", "field value for instance one");
    le_avdata_SetInt(instOneRef, "variableIntOne", 456);

#ifdef OVERFLOW_TESTING
    // For testing response payload buffer overflow
    le_avdata_AssetInstanceRef_t instTwoRef = le_avdata_Create("myData");

    le_avdata_SetString(instTwoRef,
                        "variableStringOne",
                        "a very long field value for StringOne of instance two"
                        "a very long field value for StringOne of instance two"
                        "a very long field value for StringOne of instance two"
                        "a very long field value for StringOne of instance two");

    le_avdata_SetString(instTwoRef,
                        "variableStringTwo",
                        "a very long field value for StringTwo of instance two"
                        "a very long field value for StringTwo of instance two"
                        "a very long field value for StringTwo of instance two"
                        "a very long field value for StringTwo of instance two");
#endif

    /*
     * Test settings fields
     */

    // Get the initial values of the setting fields
    le_avdata_GetString(instZeroRef, "settingStringOne", strBuf, sizeof(strBuf));
    LE_PRINT_VALUE("%s", strBuf);
    le_avdata_GetInt(instOneRef, "settingIntTwo", &value);
    LE_PRINT_VALUE("%i", value);
    le_avdata_GetBool(instOneRef, "settingBoolOne", &bValue);
    LE_PRINT_VALUE("%i", bValue);

    // Register a handler that will actually get the value of the 'setting' field on write
    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "settingStringOne",
                                   Field_settingStringOne_Handler,
                                   NULL);

    // Register a handler which will be called when there is a execute request for the field
    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "commandOne",
                                   Field_executeCommandOne_Handler,
                                   NULL);

    // Register a handler which will be called when there is a execute request for the field
    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "commandTwo",
                                   Field_executeCommandTwo_Handler,
                                   NULL);

#ifdef VERIFY_APN_CONFIG
    le_avc_SessionType_t sessionType;
    uint16_t httpStatus;

    uint16_t testRetryTimer[LE_AVC_NUM_RETRY_TIMERS];
    int i;

    // Test the config APIs.

    // We can only read the http status and session type.
    httpStatus = le_avc_GetHttpStatus();
    sessionType = le_avc_GetSessionType();

    LE_DEBUG("sessionType = %d", sessionType);
    LE_DEBUG("httpStatus = %d", httpStatus);

    // Verify polling timer by writing two different values.

    VerifyPollingTimer(1234);
    VerifyPollingTimer(4321);

    // Verify retry timer by writing two different values.
    for (i = 0; i < LE_AVC_NUM_RETRY_TIMERS; i++)
    {
        testRetryTimer[i] = i;
    }

    VerifyRetryTimer(testRetryTimer, NUM_ARRAY_MEMBERS(testRetryTimer));

    for (i = 0; i < LE_AVC_NUM_RETRY_TIMERS; i++)
    {
        testRetryTimer[i] = 100 + i;
    }

    VerifyRetryTimer(testRetryTimer, NUM_ARRAY_MEMBERS(testRetryTimer));

    // Verify APN config by writing two different values.
    VerifyApnConfig("internet.com", "NewUser", "NewPassword");
    VerifyApnConfig("sierra.com", "NewUser1", "NewPassword1");
#endif

    le_avc_AddStatusEventHandler(StatusHandler, NULL);

    // Initialize a timer that periodically increments a variable.
    le_clk_Time_t timerInterval = { .sec=15, .usec=0 };

    sampleTimerRef = le_timer_Create("SampleTimer");
    le_timer_SetInterval(sampleTimerRef, timerInterval);
    le_timer_SetContextPtr(sampleTimerRef, (void*)instOneRef);
    le_timer_SetRepeat(sampleTimerRef, 0);
    le_timer_SetHandler(sampleTimerRef, SampleTimer);
    le_timer_Start(sampleTimerRef);
}
