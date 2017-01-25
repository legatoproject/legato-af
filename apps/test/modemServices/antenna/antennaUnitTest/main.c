/**
 * This module implements the unit tests for ANTENNA API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_antenna_simu.h"
#include "le_antenna_local.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------
#define ANTENNA_SHORT_LIMIT         839
#define ANTENNA_OPEN_LIMIT          1088


//--------------------------------------------------------------------------------------------------
/*
 * context used for antenna diagnostics test
 */
//--------------------------------------------------------------------------------------------------
static struct
{
    le_antenna_Type_t               antennaType;
    le_antenna_ObjRef_t             antennaRef;
    le_event_Id_t                   statusEventId;
    le_antenna_StatusEventHandlerRef_t   handlerRef;
}   AntennaCtxtText[LE_ANTENNA_MAX] = {{0}};


//--------------------------------------------------------------------------------------------------
/*
 * Adc Index used to set External Adc
 */
//--------------------------------------------------------------------------------------------------
static int8_t AdcIndex[LE_ANTENNA_MAX] = {0, 1, 2};

//--------------------------------------------------------------------------------------------------
/*
 *  Antenna status Handler
 */
//--------------------------------------------------------------------------------------------------
static void StatusHandler
(
    le_antenna_ObjRef_t antennaRef,
    le_antenna_Status_t status,
    void* contextPtr
)
{

    switch (status)
    {
        case LE_ANTENNA_SHORT_CIRCUIT:
            LE_INFO("Antenna Status : LE_ANTENNA_SHORT_CIRCUIT");
            break;

        case LE_ANTENNA_CLOSE_CIRCUIT:
            LE_INFO("Antenna Status : LE_ANTENNA_CLOSE_CIRCUIT");
            break;

        case LE_ANTENNA_OPEN_CIRCUIT:
            LE_INFO("Antenna Status :LE_ANTENNA_OPEN_CIRCUIT");
            break;

        case LE_ANTENNA_OVER_CURRENT:
            LE_INFO("Antenna Status : LE_ANTENNA_OVER_CURRENT");
            break;

        case LE_ANTENNA_INACTIVE:
            LE_INFO("Antenna Status : LE_ANTENNA_INACTIVE");
            break;

        default:
            LE_INFO("Antenna Status : Unknown status");
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for antenna Request
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_Request
(
    void
)
{
    le_antenna_Type_t antenna;

    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        AntennaCtxtText[antenna].antennaRef = le_antenna_Request(antenna);

        LE_DEBUG("antenna Type : %d , Ref : %p",antenna, AntennaCtxtText[antenna].antennaRef);
        LE_ASSERT(AntennaCtxtText[antenna].antennaRef != NULL);
    }

    // Test for invalid antenna type
    LE_ASSERT(le_antenna_Request(5) == NULL);
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for antenna Get Type
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_GetType
(
    void
)
{
    le_antenna_Type_t antenna;
    le_antenna_Type_t antennaType;

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_OK);

    // Test with NULL ref
    LE_ASSERT(le_antenna_GetType(NULL, &antennaType) == LE_NOT_FOUND);

    // Test for all antenna type
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        LE_ASSERT(le_antenna_GetType(
                   AntennaCtxtText[antenna].antennaRef, &antennaType) == LE_OK);
        LE_ASSERT(antennaType == antenna);
    }
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for antenna Get Status
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_GetStatus
(
    void
)
{
    le_antenna_Type_t antenna;
    le_antenna_Status_t status;

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_OK);

    // Test with Null ref
    LE_ASSERT(le_antenna_GetStatus(NULL, &status) == LE_NOT_FOUND);

    // Test for all antenna type
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        LE_ASSERT(le_antenna_GetStatus(
                   AntennaCtxtText[antenna].antennaRef, &status) == LE_OK);
    }

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_UNSUPPORTED);

    LE_ASSERT(le_antenna_GetStatus(
               AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef, &status) == LE_UNSUPPORTED);
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for Antenna status Handler
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_AddStatusEventHandler
(
    void
)
{
    le_antenna_Type_t antenna;

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_OK);

    // pass NULL for antenna ref
    LE_ASSERT(le_antenna_AddStatusEventHandler(
               NULL, StatusHandler, NULL) == NULL);

    // add handler for valid antenna ref for all antenna type
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        // Add status event handler.
        AntennaCtxtText[antenna].handlerRef =
            le_antenna_AddStatusEventHandler(AntennaCtxtText[antenna].antennaRef, StatusHandler, NULL);

        LE_ASSERT(AntennaCtxtText[antenna].handlerRef != NULL);
    }
    // add handler for antenna type which is already subscribed
    LE_ASSERT(le_antenna_AddStatusEventHandler(
               AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef, StatusHandler, NULL) == NULL);
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for set and get External Adc
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_SetAndGetExternalAdc
(
    void
)
{
    le_antenna_Type_t antenna;
    int8_t antennaAdc;

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_OK);

    // Test Set External Adc with NULL ref
    LE_ASSERT(le_antenna_SetExternalAdc(
               NULL, LE_ANTENNA_PRIMARY_CELLULAR) == LE_NOT_FOUND);

    // Test Get External Adc with NULL ref
    LE_ASSERT( le_antenna_GetExternalAdc(
               NULL, &antennaAdc ) == LE_NOT_FOUND );

    // Test with valid ref and valid adc index for all antenna type
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        // Test Set External Adc
        LE_ASSERT(le_antenna_SetExternalAdc(
                   AntennaCtxtText[antenna].antennaRef, AdcIndex[antenna]) == LE_OK);

        // Test Get External Adc
        LE_ASSERT(le_antenna_GetExternalAdc(
                   AntennaCtxtText[antenna].antennaRef, &antennaAdc) == LE_OK);
        // check the get value with set value
        LE_ASSERT(antennaAdc == AdcIndex[antenna]);
    }

    // Test le_antenna_SetExternalAdc with the index already set for other antenna type
    LE_ASSERT(le_antenna_SetExternalAdc(
               AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef, 1) == LE_FAULT);

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_UNSUPPORTED);

    LE_ASSERT(le_antenna_SetExternalAdc(
               AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, AdcIndex[LE_ANTENNA_GNSS]) == LE_UNSUPPORTED);

    LE_ASSERT(le_antenna_GetExternalAdc(
               AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, &antennaAdc) == LE_UNSUPPORTED);
    // Internal Adc selected
    LE_ASSERT(antennaAdc == -1);
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for set and get ShortLimit
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_SetAndGetShortLimit
(
    void
)
{
    le_antenna_Type_t antenna;
    uint32_t shortLimit = 0;

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_OK);

    // Set the short limit with NULL ref
    LE_ASSERT(le_antenna_SetShortLimit( NULL, ANTENNA_SHORT_LIMIT) == LE_NOT_FOUND);
    // Get the Short Limit with NULL ref
    LE_ASSERT(le_antenna_GetShortLimit(NULL, &shortLimit) == LE_NOT_FOUND);

    // Test Set And Get Short Limit for all antenna types
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        shortLimit = 0;

        // Set the short limit
        LE_ASSERT(le_antenna_SetShortLimit(
                   AntennaCtxtText[antenna].antennaRef, ANTENNA_SHORT_LIMIT) == LE_OK);
        // Get the Short Limit
        LE_ASSERT(le_antenna_GetShortLimit(
                   AntennaCtxtText[antenna].antennaRef, &shortLimit) == LE_OK);
        LE_ASSERT(shortLimit == ANTENNA_SHORT_LIMIT);
    }
    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_FAULT);

    shortLimit = 0;

    // Set the short limit
    LE_ASSERT(le_antenna_SetShortLimit(
               AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef, ANTENNA_SHORT_LIMIT) == LE_FAULT);

    // Set the short limit
    LE_ASSERT(le_antenna_SetShortLimit(
               AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, ANTENNA_SHORT_LIMIT) == LE_FAULT);

    // Get the Short Limit
    LE_ASSERT(le_antenna_GetShortLimit(
               AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, &shortLimit) == LE_FAULT);
}


//--------------------------------------------------------------------------------------------------
/*
 *  Test for set and get OpenLimit
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_SetAndGetOpenLimit
(
    void
)
{
    le_antenna_Type_t antenna;
    uint32_t openLimit = 0;

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_OK);

    // Set the Open limit with NULL ref
    LE_ASSERT(le_antenna_SetOpenLimit(NULL, ANTENNA_OPEN_LIMIT) == LE_NOT_FOUND);
    // Get the Open Limit with NULL ref
    LE_ASSERT(le_antenna_GetOpenLimit(NULL, &openLimit) == LE_NOT_FOUND);

    // Test Set And Get Open Limit for all antenna types
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        openLimit = 0;

        // Set the Open limit
        LE_ASSERT(le_antenna_SetOpenLimit(
                   AntennaCtxtText[antenna].antennaRef, ANTENNA_OPEN_LIMIT) == LE_OK);
        // Get the Open Limit
        LE_ASSERT(le_antenna_GetOpenLimit(
                   AntennaCtxtText[antenna].antennaRef, &openLimit) == LE_OK);
        LE_ASSERT(openLimit == ANTENNA_OPEN_LIMIT);
    }

    // Set Return Code
    pa_antennaSimu_SetReturnCode(LE_FAULT);

    openLimit = 0;
    // Set the Open limit
    LE_ASSERT(le_antenna_SetOpenLimit(
               AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef, ANTENNA_OPEN_LIMIT) == LE_FAULT);

    // Set the Open limit
    LE_ASSERT(le_antenna_SetOpenLimit(
               AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, ANTENNA_OPEN_LIMIT) == LE_FAULT);

    // Get the Open Limit
    LE_ASSERT(le_antenna_GetOpenLimit(
               AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, &openLimit) == LE_FAULT);
}


//--------------------------------------------------------------------------------------------------
/*
 *  Remove Antenna status Handler
 */
//--------------------------------------------------------------------------------------------------
static void Testle_antenna_RemoveStatusEventHandler
(
    void
)
{
    le_antenna_Type_t antenna;

    // Remove handler for all antenna type
    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        le_antenna_RemoveStatusEventHandler(AntennaCtxtText[antenna].handlerRef);
        LE_DEBUG("Handler removed for antenna type : %d", antenna );
    }

    // try to remove handler which is not subscribed/removed
    le_antenna_RemoveStatusEventHandler(AntennaCtxtText[LE_ANTENNA_GNSS].handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    // To reactivate for all DEBUG logs
    //le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Init pa simu
    pa_antenna_Init();
    // Initialization of the Legato Antenna Monitoring Service
    le_antenna_Init();

    LE_INFO("======== START UnitTest of ANTENNA API ========");

    LE_INFO("======== Testle_antenna_Request TEST ========");
    Testle_antenna_Request();

    LE_INFO("======== Testle_antenna_GetType ========");
    Testle_antenna_GetType();

    LE_INFO("======== Testle_antenna_AddStatusEventHandler TEST ========");
    Testle_antenna_AddStatusEventHandler();

    LE_INFO("======== Testle_antenna_SetAndGetExternalAdc TEST ========");
    Testle_antenna_SetAndGetExternalAdc();

    LE_INFO("======== Testle_antenna_SetAndGetShortLimit TEST ========");
    Testle_antenna_SetAndGetShortLimit();

    LE_INFO("======== Testle_antenna_SetAndGetOpenLimit TEST ========");
    Testle_antenna_SetAndGetOpenLimit();

    LE_INFO("======== Testle_antenna_GetStatus TEST ========");
    Testle_antenna_GetStatus();

    LE_INFO("======== Testle_antenna_RemoveStatusEventHandler TEST ========");
    Testle_antenna_RemoveStatusEventHandler();

    LE_INFO("======== UnitTest of ANTENNA API FINISHED ========");
    exit(0);
}
