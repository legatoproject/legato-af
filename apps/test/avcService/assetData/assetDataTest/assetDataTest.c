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
        LE_INFO("Create happened on '%s', %i, %i", appName, assetId, instanceId);
    else
        LE_INFO("Delete happened on '%s', %i, %i", appName, assetId, instanceId);

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

    int instanceId;
    LE_TEST ( assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK );

    LE_INFO("Write happened on '%s', %i, %i, %i", appName, assetId, instanceId, fieldId);

    assetData_client_GetInt(instanceRef, fieldId, &value);
    LE_INFO("New value is %i", value);

    if ( instanceId == 0 )
        LE_TEST( value == 399 )
    else if ( instanceId == 1 )
        LE_TEST( value == 512 )
    else
        LE_TEST( ! "valid instance id" );

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

    int instanceId;
    LE_TEST ( assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK );

    LE_TEST( instanceId == 0 );

    LE_INFO("Execute happened on '%s', %i, %i, %i", appName, assetId, instanceId, fieldId);

    LE_TEST ( contextPtr == SemExecOne )
    le_sem_Post(SemExecOne);
}


void RunTest(void)
{
    banner("Test Asset list before creating instances");

    banner("Get Asset Refs before creating instances");
    assetData_AssetDataRef_t lwm2mAssetRef;

    LE_TEST( assetData_GetAssetRefById("lwm2m", 9, &lwm2mAssetRef) == LE_OK );


    banner("Instance creation handlers");
    LE_TEST( assetData_client_AddAssetActionHandler(lwm2mAssetRef, AssetCreateHandler, "test") != NULL);


    banner("Create asset instances");
    assetData_InstanceDataRef_t testOneRefZero = NULL;
    assetData_InstanceDataRef_t testOneRefOne = NULL;
    assetData_InstanceDataRef_t lwm2mRefZero = NULL;
    assetData_InstanceDataRef_t lwm2mRefOne = NULL;
    assetData_InstanceDataRef_t lwm2mRefOneB = NULL;
    int instanceId;

    LE_TEST( assetData_CreateInstanceById("testOne", 1000, -1, &testOneRefZero) == LE_OK );
    LE_TEST( testOneRefZero != NULL );
    LE_TEST( assetData_GetInstanceId(testOneRefZero, &instanceId) == LE_OK );
    LE_TEST( instanceId == 0 );

    LE_TEST( assetData_CreateInstanceById("testOne", 1000, -1, &testOneRefOne) == LE_OK );
    LE_TEST( testOneRefOne != NULL );
    LE_TEST( assetData_GetInstanceId(testOneRefOne, &instanceId) == LE_OK );
    LE_TEST( instanceId == 1 );

    LE_TEST( assetData_CreateInstanceById("lwm2m", 9, 3, &lwm2mRefZero) == LE_OK );
    LE_TEST( lwm2mRefZero != NULL );
    LE_TEST( assetData_GetInstanceId(lwm2mRefZero, &instanceId) == LE_OK );
    LE_TEST( instanceId == 3 );
    le_sem_Wait(SemCreateOne);

    LE_TEST( assetData_CreateInstanceById("lwm2m", 9, -1, &lwm2mRefOne) == LE_OK );
    LE_TEST( lwm2mRefOne != NULL );
    LE_TEST( assetData_GetInstanceId(lwm2mRefOne, &instanceId) == LE_OK );
    LE_TEST( instanceId == 4 );
    le_sem_Wait(SemCreateTwo);

    // Try creating the same instance again
    LE_TEST( assetData_CreateInstanceById("lwm2m", 9, 4, &lwm2mRefOneB) == LE_DUPLICATE );
    LE_TEST( lwm2mRefOneB == NULL );

    banner("Get Asset Refs");
    assetData_AssetDataRef_t testOneAssetRef;

    LE_TEST( assetData_GetAssetRefById("testOne", 1000, &testOneAssetRef) == LE_OK );


    banner("Read/Write integer fields");
    int value;

    LE_TEST( assetData_client_GetInt(testOneRefZero, 4, &value) == LE_OK );
    LE_TEST( value == 18 );

    LE_TEST( assetData_client_SetInt(testOneRefZero, 4, 199) == LE_OK );
    LE_TEST( assetData_client_GetInt(testOneRefZero, 4, &value) == LE_OK );
    LE_TEST( value == 199 );

    LE_TEST( assetData_client_GetInt(testOneRefOne, 4, &value) == LE_OK );
    LE_TEST( value == 18 );

    LE_TEST( assetData_client_GetInt(testOneRefZero, 50, &value) == LE_NOT_FOUND );


    banner("Read/Write integer fields as values");
    char valueStr[100];

    LE_TEST( assetData_server_GetValue(NULL, testOneRefZero, 4, valueStr, sizeof(valueStr)) == LE_OK );
    LE_TEST( strcmp( valueStr, "199" ) == 0 );

    LE_TEST( assetData_server_SetValue(testOneRefZero, 4, "123") == LE_OK );
    LE_TEST( assetData_client_GetInt(testOneRefZero, 4, &value) == LE_OK );
    LE_TEST( value == 123 );

    banner("Read/Write float fields");
    double float_value;

    LE_TEST( assetData_client_GetFloat(testOneRefZero, 12, &float_value) == LE_OK );
    LE_PRINT_VALUE("%lf",float_value);
    LE_TEST( float_value == 123.456 );

    LE_TEST( assetData_client_SetFloat(testOneRefZero, 12, 789.012) == LE_OK );
    LE_TEST( assetData_client_GetFloat(testOneRefZero, 12, &float_value) == LE_OK );
    LE_TEST( float_value == 789.012 );

    LE_TEST( assetData_client_GetFloat(testOneRefOne, 12, &float_value) == LE_OK );
    LE_TEST( float_value == 123.456 );

    LE_TEST( assetData_client_GetFloat(testOneRefZero, 50, &float_value) == LE_NOT_FOUND );


    banner("Read/Write float as values");

    LE_TEST( assetData_server_GetValue(NULL, testOneRefZero, 12, valueStr, sizeof(valueStr)) == LE_OK );
    LE_TEST( strcmp( valueStr, "789.012000" ) == 0 );

    LE_TEST( assetData_server_SetValue(testOneRefZero, 12, "345.678") == LE_OK );
    LE_TEST( assetData_client_GetFloat(testOneRefZero, 12, &float_value) == LE_OK );
    LE_TEST( float_value == 345.678 );

    banner("Read/Write string fields");
    char strBuf[100];

    LE_TEST( assetData_client_SetString(lwm2mRefZero, 0, "new value") == LE_OK );
    LE_TEST( assetData_client_GetString(lwm2mRefZero, 0, strBuf, sizeof(strBuf)) == LE_OK );
    LE_TEST( strcmp(strBuf, "new value") == 0 );

    LE_TEST( assetData_client_SetString(lwm2mRefZero, 0, "a different value") == LE_OK );
    LE_TEST( assetData_client_GetString(lwm2mRefZero, 0, strBuf, sizeof(strBuf)) == LE_OK );
    LE_TEST( strcmp(strBuf, "a different value") == 0 );


    banner("Read/Write string fields as values");

    LE_TEST( assetData_server_GetValue(NULL, lwm2mRefZero, 0, valueStr, sizeof(valueStr)) == LE_OK );
    LE_TEST( strcmp( valueStr, "a different value" ) == 0 );

    LE_TEST( assetData_server_SetValue(lwm2mRefZero, 0, "123") == LE_OK );
    LE_TEST( assetData_client_GetString(lwm2mRefZero, 0, valueStr, sizeof(valueStr)) == LE_OK );
    LE_TEST( strcmp( valueStr, "123" ) == 0 );


    banner("Read/Write incompatible fields");

    LE_TEST( assetData_client_SetInt(lwm2mRefZero, 0, 256) == LE_FAULT );
    LE_TEST( assetData_client_GetInt(lwm2mRefZero, 0, &value) == LE_FAULT );

    LE_TEST( assetData_client_SetString(testOneRefZero, 4, "new value") == LE_FAULT );
    LE_TEST( assetData_client_GetString(testOneRefZero, 4, strBuf, sizeof(strBuf)) == LE_FAULT );


    banner("Field write int handlers");

    LE_TEST( assetData_server_AddFieldActionHandler(testOneAssetRef, 4, FieldWriteIntHandler, SemWriteOne) != NULL);
    LE_TEST( assetData_server_AddFieldActionHandler(testOneAssetRef, 4, FieldWriteIntHandler, SemWriteTwo) != NULL);

    LE_TEST( assetData_client_SetInt(testOneRefZero, 4, 399) == LE_OK );
    LE_TEST( assetData_client_GetInt(testOneRefZero, 4, &value) == LE_OK );
    LE_TEST( value == 399 );
    le_sem_Wait(SemWriteOne);
    le_sem_Wait(SemWriteTwo);

    LE_TEST( assetData_client_SetInt(testOneRefOne, 4, 512) == LE_OK );
    LE_TEST( assetData_client_GetInt(testOneRefOne, 4, &value) == LE_OK );
    LE_TEST( value == 512 );
    le_sem_Wait(SemWriteOne);
    le_sem_Wait(SemWriteTwo);


    banner("Field execute handlers");

    LE_TEST( assetData_client_AddFieldActionHandler(testOneAssetRef, 2, FieldExecHandler, SemExecOne) != NULL);
    LE_TEST( assetData_server_Execute(testOneRefZero, 2) == LE_OK );
    LE_TEST( assetData_server_Execute(testOneRefZero, 1) == LE_FAULT );
    le_sem_Wait(SemExecOne);


    banner("Create Framework object instances");
    assetData_InstanceDataRef_t frameworkRefZero = NULL;

    LE_TEST( assetData_CreateInstanceById("legato", 0, -1, &frameworkRefZero) == LE_OK );
    LE_TEST( frameworkRefZero != NULL );
    LE_TEST( assetData_GetInstanceId(frameworkRefZero, &instanceId) == LE_OK );
    LE_TEST( instanceId == 0 );
    LE_TEST( assetData_client_GetString(frameworkRefZero, 0, strBuf, sizeof(strBuf)) == LE_OK );
    LE_TEST( strcmp(strBuf, "1.0") == 0 );


    banner("Write Object to TLV Testing");
    uint8_t tlvBuffer[256];
    size_t bytesWritten;

    // Set the package names for each instance ...
    LE_TEST( assetData_client_SetString(lwm2mRefZero, 0, "instance zero") == LE_OK );
    LE_TEST( assetData_client_SetString(lwm2mRefOne, 0, "instance one") == LE_OK );

    LE_TEST( assetData_WriteObjectToTLV(lwm2mAssetRef, 0, tlvBuffer, sizeof(tlvBuffer), &bytesWritten) == LE_OK );
    WriteDataToLog(tlvBuffer, bytesWritten);


    banner("Write To / Read From TLV Testing");
    uint8_t tlvBufferOne[256];
    uint8_t tlvBufferTwo[256];
    size_t bytesWrittenOne;
    size_t bytesWrittenTwo;

    // Set some other resource values, such as "Update Result" which is 9.
    LE_TEST( assetData_client_SetInt(lwm2mRefZero, 9, 0x123456) == LE_OK );

    // Write assetData to TLV
    LE_TEST( assetData_WriteFieldListToTLV(lwm2mRefZero, tlvBufferOne, sizeof(tlvBufferOne), &bytesWrittenOne) == LE_OK );
    WriteDataToLog(tlvBufferOne, bytesWrittenOne);

    // Read from the TLV and write back to assetData
    LE_TEST( assetData_ReadFieldListFromTLV(tlvBufferOne, bytesWrittenOne, lwm2mRefZero, false) == LE_OK );

    // Write assetData to different TLV and compare
    LE_TEST( assetData_WriteFieldListToTLV(lwm2mRefZero, tlvBufferTwo, sizeof(tlvBufferTwo), &bytesWrittenTwo) == LE_OK );
    WriteDataToLog(tlvBufferTwo, bytesWrittenTwo);

    LE_TEST( bytesWrittenOne == bytesWrittenTwo );
    LE_TEST( memcmp(tlvBufferOne, tlvBufferTwo, bytesWrittenOne) == 0 );
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


