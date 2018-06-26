/*
 * This program tests the assetData interface.
 */

#include "legato.h"

#include "assetData.h"
#include "le_print.h"



// Used for signalling between handlers and RunTest()
// Note that this works because the handlers are called directly, rather than being queued onto
// the eventLoop of the main thread.
le_sem_Ref_t SemWriteOne;
le_sem_Ref_t SemWriteTwo;
le_sem_Ref_t SemExecOne;
le_sem_Ref_t SemCreateOne;
le_sem_Ref_t SemCreateTwo;



void banner(char *testName)
{
    int i;
    char banner[41];

    for (i=0; i<sizeof(banner)-1; i++)
        banner[i]='=';
    banner[sizeof(banner)-1] = '\0';

    LE_INFO("\n%s %s %s", banner, testName, banner);
}


void WriteDataToLog
(
    uint8_t* dataPtr,
    size_t numBytes
)
{
    int i;
    char buffer[10000];  // hopefully there's no buffer overflow

    for (i=0; i<numBytes; i++)
    {
        sprintf(&buffer[i*3], "%02X ", *dataPtr++ );
    }

    LE_INFO("Data = >>%s<<", buffer);
}


void AssetCreateHandler
(
    assetData_AssetDataRef_t assetRef,
    int instanceId,
    assetData_ActionTypes_t action,
    void* contextPtr
)
{
    LE_TEST( (action == ASSET_DATA_ACTION_CREATE) || (action == ASSET_DATA_ACTION_DELETE) );
    LE_TEST ( strcmp( contextPtr, "test") == 0 );

    char appName[100];
    LE_TEST ( assetData_GetAppNameFromAsset(assetRef, appName, sizeof(appName)) == LE_OK );
    LE_TEST ( strcmp( appName, "lwm2m") == 0 );

    int assetId;
    LE_TEST ( assetData_GetAssetIdFromAsset(assetRef, &assetId) == LE_OK );
    LE_TEST ( assetId == 9 );

    LE_TEST( (instanceId==3) || (instanceId==4) );

    if ( action == ASSET_DATA_ACTION_CREATE )
    {
        LE_INFO("Create happened on '%s', %i, %i", appName, assetId, instanceId);
    }
    else
    {
        LE_INFO("Delete happened on '%s', %i, %i", appName, assetId, instanceId);
    }

    if ( instanceId == 3 )
    {
        LE_INFO("Got instance 3");
        le_sem_Post(SemCreateOne);
    }
    else if ( instanceId == 4 )
    {
        LE_INFO("Got instance 4");
        le_sem_Post(SemCreateTwo);
    }
}


void FieldWriteIntHandler
(
    assetData_InstanceDataRef_t instanceRef,
    int fieldId,
    assetData_ActionTypes_t action,
    void* contextPtr
)
{
    int value;

    if ( action == ASSET_DATA_ACTION_READ )
    {
        LE_INFO("Ignore read action");
        return;
    }

    LE_TEST( action == ASSET_DATA_ACTION_WRITE );

    char appName[100];
    LE_TEST ( assetData_GetAppNameFromInstance(instanceRef, appName, sizeof(appName)) == LE_OK );

    int assetId;
    LE_TEST ( assetData_GetAssetIdFromInstance(instanceRef, &assetId) == LE_OK );

    int instanceId = -1;
    LE_TEST ( assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK );

    LE_INFO("Write happened on '%s', %i, %i, %i", appName, assetId, instanceId, fieldId);

    assetData_client_GetInt(instanceRef, fieldId, &value);
    LE_INFO("New value is %i", value);

    if ( instanceId == 0 )
    {
        LE_TEST( value == 399 );
    }
    else if ( instanceId == 1 )
    {
        LE_TEST( value == 512 );
    }
    else
    {
        LE_TEST( ! "valid instance id" );
    }

    if ( contextPtr == SemWriteOne )
    {
        LE_INFO("Got SemWriteOne");
        le_sem_Post(SemWriteOne);
    }
    else if ( contextPtr == SemWriteTwo )
    {
        LE_INFO("Got SemWriteTwo");
        le_sem_Post(SemWriteTwo);
    }
    else
    {
        LE_PRINT_VALUE("%p", SemWriteOne);
        LE_PRINT_VALUE("%p", SemWriteTwo);
        LE_FATAL("Bad contextPtr=%p", contextPtr);
    }
}


void FieldExecHandler
(
    assetData_InstanceDataRef_t instanceRef,
    int fieldId,
    assetData_ActionTypes_t action,
    void* contextPtr
)
{
    LE_TEST( action == ASSET_DATA_ACTION_EXEC );

    char appName[100];
    LE_TEST ( assetData_GetAppNameFromInstance(instanceRef, appName, sizeof(appName)) == LE_OK );

    int assetId;
    LE_TEST ( assetData_GetAssetIdFromInstance(instanceRef, &assetId) == LE_OK );

    int instanceId = -1;
    LE_TEST ( assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK );

    LE_TEST( instanceId == 0 );

    LE_INFO("Execute happened on '%s', %i, %i, %i", appName, assetId, instanceId, fieldId);

    LE_TEST ( contextPtr == SemExecOne );
    le_sem_Post(SemExecOne);
}


void RunTest(void)
{
    banner("Test Asset list before creating instances");

    banner("Get Asset Refs before creating instances");
    assetData_AssetDataRef_t lwm2mAssetRef;

    LE_TEST(LE_OK == assetData_GetAssetRefById("lwm2m", 9, &lwm2mAssetRef));


    banner("Instance creation handlers");
    LE_TEST(NULL != assetData_client_AddAssetActionHandler(lwm2mAssetRef,
                                               AssetCreateHandler, "test"));


    banner("Create asset instances");
    assetData_InstanceDataRef_t testOneRefZero = NULL;
    assetData_InstanceDataRef_t testOneRefOne = NULL;
    assetData_InstanceDataRef_t lwm2mRefZero = NULL;
    assetData_InstanceDataRef_t lwm2mRefOne = NULL;
    assetData_InstanceDataRef_t lwm2mRefOneB = NULL;
    int instanceId;

    LE_TEST(LE_OK == assetData_CreateInstanceById("testOne", 1000, -1, &testOneRefZero));
    LE_TEST(NULL != testOneRefZero);
    LE_TEST(LE_OK == assetData_GetInstanceId(testOneRefZero, &instanceId));
    LE_TEST(0 == instanceId);

    LE_TEST(LE_OK == assetData_CreateInstanceById("testOne", 1000, -1, &testOneRefOne));
    LE_TEST(NULL != testOneRefOne);
    LE_TEST(LE_OK == assetData_GetInstanceId(testOneRefOne, &instanceId));
    LE_TEST(1 == instanceId);

    LE_TEST(LE_OK == assetData_CreateInstanceById("lwm2m", 9, 3, &lwm2mRefZero));
    LE_TEST(NULL != lwm2mRefZero);
    LE_TEST(LE_OK == assetData_GetInstanceId(lwm2mRefZero, &instanceId));
    LE_TEST(3 == instanceId);
    le_sem_Wait(SemCreateOne);

    LE_TEST(LE_OK == assetData_CreateInstanceById("lwm2m", 9, -1, &lwm2mRefOne));
    LE_TEST(NULL != lwm2mRefOne);
    LE_TEST(LE_OK == assetData_GetInstanceId(lwm2mRefOne, &instanceId));
    LE_TEST(4 == instanceId);
    le_sem_Wait(SemCreateTwo);

    // Try creating the same instance again
    LE_TEST(LE_DUPLICATE == assetData_CreateInstanceById("lwm2m", 9, 4, &lwm2mRefOneB));
    LE_TEST(NULL == lwm2mRefOneB);

    banner("Get Asset Refs");
    assetData_AssetDataRef_t testOneAssetRef;

    LE_TEST(LE_OK == assetData_GetAssetRefById("testOne", 1000, &testOneAssetRef));


    banner("Read/Write integer fields");
    int value;

    LE_TEST(LE_OK == assetData_client_GetInt(testOneRefZero, 4, &value));
    LE_TEST(18 == value);

    LE_TEST(LE_OK == assetData_client_SetInt(testOneRefZero, 4, 199));
    LE_TEST(LE_OK == assetData_client_GetInt(testOneRefZero, 4, &value));
    LE_TEST(199 == value);

    LE_TEST(LE_OK == assetData_client_GetInt(testOneRefOne, 4, &value));
    LE_TEST(18 == value);

    LE_TEST(LE_NOT_FOUND == assetData_client_GetInt(testOneRefZero, 50, &value));


    banner("Read/Write integer fields as values");
    char valueStr[100];

    LE_TEST(LE_OK == assetData_server_GetValue(NULL, testOneRefZero,
                                               4, valueStr, sizeof(valueStr)));
    LE_TEST(0 == strcmp( valueStr, "199" ));

    LE_TEST(LE_OK == assetData_server_SetValue(testOneRefZero, 4, "123"));
    LE_TEST(LE_OK == assetData_client_GetInt(testOneRefZero, 4, &value));
    LE_TEST(123 == value);

    banner("Read/Write float fields");
    double float_value;

    LE_TEST(LE_OK == assetData_client_GetFloat(testOneRefZero, 12, &float_value));
    LE_PRINT_VALUE("%lf",float_value);
    LE_TEST(123.456 == float_value);

    LE_TEST(LE_OK == assetData_client_SetFloat(testOneRefZero, 12, 789.012));
    LE_TEST(LE_OK == assetData_client_GetFloat(testOneRefZero, 12, &float_value));
    LE_TEST(789.012 == float_value);

    LE_TEST(LE_OK == assetData_client_GetFloat(testOneRefOne, 12, &float_value));
    LE_TEST(123.456 == float_value);

    LE_TEST(LE_NOT_FOUND == assetData_client_GetFloat(testOneRefZero, 50, &float_value));


    banner("Read/Write float as values");

    LE_TEST(LE_OK == assetData_server_GetValue(NULL, testOneRefZero,
                                               12, valueStr, sizeof(valueStr)));
    LE_TEST(0 == strcmp( valueStr, "789.012000" ));

    LE_TEST(LE_OK == assetData_server_SetValue(testOneRefZero, 12, "345.678"));
    LE_TEST(LE_OK == assetData_client_GetFloat(testOneRefZero, 12, &float_value));
    LE_TEST(345.678 == float_value);

    banner("Read/Write string fields");
    char strBuf[100];

    LE_TEST(LE_OK == assetData_client_SetString(lwm2mRefZero, 0, "new value"));
    LE_TEST(LE_OK == assetData_client_GetString(lwm2mRefZero, 0, strBuf, sizeof(strBuf)));
    LE_TEST(0 == strcmp(strBuf, "new value"));

    LE_TEST(LE_OK == assetData_client_SetString(lwm2mRefZero, 0, "a different value"));
    LE_TEST(LE_OK == assetData_client_GetString(lwm2mRefZero, 0, strBuf, sizeof(strBuf)));
    LE_TEST(0 == strcmp(strBuf, "a different value"));


    banner("Read/Write string fields as values");

    LE_TEST(LE_OK == assetData_server_GetValue(NULL, lwm2mRefZero, 0, valueStr, sizeof(valueStr)));
    LE_TEST(0 == strcmp( valueStr, "a different value" ));

    LE_TEST(LE_OK == assetData_server_SetValue(lwm2mRefZero, 0, "123"));
    LE_TEST(LE_OK == assetData_client_GetString(lwm2mRefZero, 0, valueStr, sizeof(valueStr)));
    LE_TEST(0 == strcmp( valueStr, "123" ));


    banner("Read/Write incompatible fields");

    LE_TEST(LE_FAULT == assetData_client_SetInt(lwm2mRefZero, 0, 256));
    LE_TEST(LE_FAULT == assetData_client_GetInt(lwm2mRefZero, 0, &value));

    LE_TEST(LE_FAULT == assetData_client_SetString(testOneRefZero, 4, "new value"));
    LE_TEST(LE_FAULT == assetData_client_GetString(testOneRefZero, 4, strBuf, sizeof(strBuf)));


    banner("Field write int handlers");

    LE_TEST(NULL != assetData_server_AddFieldActionHandler(testOneAssetRef, 4,
                                                   FieldWriteIntHandler, SemWriteOne));
    LE_TEST(NULL != assetData_server_AddFieldActionHandler(testOneAssetRef, 4,
                                                   FieldWriteIntHandler, SemWriteTwo));

    LE_TEST(LE_OK == assetData_client_SetInt(testOneRefZero, 4, 399));
    LE_TEST(LE_OK == assetData_client_GetInt(testOneRefZero, 4, &value));
    LE_TEST(399 == value);
    le_sem_Wait(SemWriteOne);
    le_sem_Wait(SemWriteTwo);

    LE_TEST(LE_OK == assetData_client_SetInt(testOneRefOne, 4, 512));
    LE_TEST(LE_OK == assetData_client_GetInt(testOneRefOne, 4, &value));
    LE_TEST(512 == value);
    le_sem_Wait(SemWriteOne);
    le_sem_Wait(SemWriteTwo);


    banner("Field execute handlers");

    LE_TEST(NULL != assetData_client_AddFieldActionHandler(testOneAssetRef, 2,
                                                           FieldExecHandler, SemExecOne));
    LE_TEST(LE_OK == assetData_server_Execute(testOneRefZero, 2));
    LE_TEST(LE_FAULT == assetData_server_Execute(testOneRefZero, 1));
    le_sem_Wait(SemExecOne);


    banner("Create Framework object instances");
    assetData_InstanceDataRef_t frameworkRefZero = NULL;

    LE_TEST(LE_OK == assetData_CreateInstanceById("legato", 0, -1, &frameworkRefZero));
    LE_TEST(NULL != frameworkRefZero);
    LE_TEST(LE_OK == assetData_GetInstanceId(frameworkRefZero, &instanceId));
    LE_TEST(0 == instanceId);
    LE_TEST(LE_OK == assetData_client_GetString(frameworkRefZero, 0, strBuf, sizeof(strBuf)));
    LE_TEST(0 == strcmp(strBuf, "1.0"));


    banner("Write Object to TLV Testing");
    uint8_t tlvBuffer[256];
    size_t bytesWritten = 0;

    // Set the package names for each instance ...
    LE_TEST(LE_OK == assetData_client_SetString(lwm2mRefZero, 0, "instance zero"));
    LE_TEST(LE_OK == assetData_client_SetString(lwm2mRefOne, 0, "instance one"));

    LE_TEST(LE_OK == assetData_WriteObjectToTLV(lwm2mAssetRef, 0,
                                                tlvBuffer, sizeof(tlvBuffer), &bytesWritten));
    WriteDataToLog(tlvBuffer, bytesWritten);


    banner("Write To / Read From TLV Testing");
    uint8_t tlvBufferOne[256];
    uint8_t tlvBufferTwo[256];
    size_t bytesWrittenOne = 0;
    size_t bytesWrittenTwo = 0;

    // Set some other resource values, such as "Update Result" which is 9.
    LE_TEST(LE_OK == assetData_client_SetInt(lwm2mRefZero, 9, 0x123456));

    // Write assetData to TLV
    LE_TEST(LE_OK == assetData_WriteFieldListToTLV(lwm2mRefZero, tlvBufferOne,
                                                   sizeof(tlvBufferOne), &bytesWrittenOne));
    WriteDataToLog(tlvBufferOne, bytesWrittenOne);

    // Read from the TLV and write back to assetData
    LE_TEST(LE_OK == assetData_ReadFieldListFromTLV(tlvBufferOne, bytesWrittenOne,
                                                    lwm2mRefZero, false));

    // Write assetData to different TLV and compare
    LE_TEST(LE_OK == assetData_WriteFieldListToTLV(lwm2mRefZero, tlvBufferTwo,
                                                   sizeof(tlvBufferTwo), &bytesWrittenTwo));
    WriteDataToLog(tlvBufferTwo, bytesWrittenTwo);

    LE_TEST( bytesWrittenOne == bytesWrittenTwo );
    LE_TEST(0 == memcmp(tlvBufferOne, tlvBufferTwo, bytesWrittenOne));
}


COMPONENT_INIT
{
    LE_TEST_INIT;

    // todo: this should eventually be done in avcServer.c
    assetData_Init();

    // Create semaphores for signalling between handler functions and RunTest()
    SemWriteOne = le_sem_Create("SemWriteOne", 0);
    SemWriteTwo = le_sem_Create("SemWriteTwo", 0);
    SemExecOne = le_sem_Create("SemExecOne", 0);
    SemCreateOne = le_sem_Create("SemCreateOne", 0);
    SemCreateTwo = le_sem_Create("SemCreateTwo", 0);

    RunTest();

    LE_TEST_EXIT;
}


