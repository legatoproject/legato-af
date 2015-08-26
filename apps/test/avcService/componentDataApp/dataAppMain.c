//--------------------------------------------------------------------------------------------------
/**
 * @file dataAppMain.c
 *
 * This component is used for testing the AirVantage Data API.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

#include "le_print.h"


// Uncomment for overflow testing
// #define OVERFLOW_TESTING


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

    LE_INFO("Registered handler called for %s", fieldName);

    // Return the value from one of the settings
    le_avdata_GetString(instRef, "settingStringOne", strBuf, sizeof(strBuf));
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
 * Init the component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start");

    le_avdata_AssetInstanceRef_t instZeroRef;
    le_avdata_AssetInstanceRef_t instOneRef;
    char strBuf[LE_AVDATA_STRING_VALUE_LEN];
    int value;
    bool bValue;

    instZeroRef = le_avdata_Create("myData");
    instOneRef = le_avdata_Create("myData");
    LE_PRINT_VALUE("%p", instZeroRef);
    LE_PRINT_VALUE("%p", instOneRef);

    /*
     * Test variable fields
     */

    // Register a handler that will actually set the value of the 'variable' field on read
    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "variableStringOne",
                                   Field_variableStringOne_Handler,
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
}
