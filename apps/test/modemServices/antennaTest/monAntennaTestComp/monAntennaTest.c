/**
 * This module is for unit tests of the antenna diagnostics
 *
 * The antenna diagnostics APIs are called sequentially, and thresholds are set to detect the
 * presence of an antenna (simulate by a 10kohms resistance).
 *
 * The different states can be simulate:
 * - close circuit: replace the antenna with a 10kohms resistance
 * - open circuit: nothing plug
 * - short circuit: short circuit the RF path
 * - over current: antenna is shorted and current HW protection circuitry has tripped.
 * @warning Ensure to check the supported antenna diagnosis for your specific platform.
 *
 * The status handler counts the number of changing state: after a configured state changement, the
 * handler is unregistered.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

#define NB_CHANGING_STATE_BEFORE_RELEASING 3
#define PRIMARY_ANTENNA_SHORT_LIMIT 839
#define PRIMARY_ANTENNA_OPEN_LIMIT 1088

//--------------------------------------------------------------------------------------------------
/**
 * Device model type
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    DEVICEMODEL_UNKNOWN = 0,
    DEVICEMODEL_AR7_FAMILY,
    DEVICEMODEL_AR8_FAMILY
}DeviceModelFamily_t;

//--------------------------------------------------------------------------------------------------
/**
 * The device model.
 */
//--------------------------------------------------------------------------------------------------
static DeviceModelFamily_t DeviceModelFamily = DEVICEMODEL_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/*
 * context used for antenna diagnostics test
 */
//--------------------------------------------------------------------------------------------------
static struct
{
    uint8_t                         count;
    le_antenna_StatusEventHandlerRef_t   handlerRef;
    le_antenna_ObjRef_t             antennaRef;
}   AntennaCtxtText[LE_ANTENNA_MAX] = {{0}};


/*
 * Antenna state handler: after NB_CHANGING_STATE_BEFORE_RELEASING changing state, the handler is
 * released
 *
 */
static void AntennaHandler
(
    le_antenna_ObjRef_t antennaRef,
    le_antenna_Status_t status,
    void* contextPtr
)
{
    le_antenna_Type_t antennaType;
    le_result_t result = le_antenna_GetType( antennaRef, &antennaType );
    LE_ASSERT(result == LE_OK);
    LE_ASSERT(antennaType < LE_ANTENNA_MAX);

    AntennaCtxtText[antennaType].count++;

    /*
     * If the changing state thresholds is reached, remove the handler, and release the antenna
     * diagnotics
     */
    if (AntennaCtxtText[antennaType].count == NB_CHANGING_STATE_BEFORE_RELEASING)
    {
        LE_INFO("Remove the handler");
        le_antenna_RemoveStatusEventHandler(AntennaCtxtText[antennaType].handlerRef);
    }
    else
    {
        LE_INFO("Antenna %d status %d", antennaType, status);
    }
}


/*
 * Start test:
 * 'app start antennaTest'
 * 'execInApp antennaTest monAntennaTest'
 *
 */
COMPONENT_INIT
{
    LE_INFO("======== Antenna diagnostic Test started  ========");

    uint32_t openLimit=0, shortLimit = 0;
    le_antenna_Status_t status;
    le_result_t result = LE_FAULT;
    int8_t antennaAdc;
    char modelDevice[256];

    // Get the device model family (AR7, AR8, ...)
    result = le_info_GetDeviceModel(modelDevice, sizeof(modelDevice));
    LE_ASSERT(result == LE_OK);
    LE_INFO("le_info_GetDeviceModel get => %s", modelDevice);
    if(strncmp(modelDevice, "AR7", strlen("AR7")) == 0)
    {
        DeviceModelFamily = DEVICEMODEL_AR7_FAMILY;
    }
    else if(strncmp(modelDevice, "AR8", strlen("AR8")) == 0)
    {
        DeviceModelFamily = DEVICEMODEL_AR8_FAMILY;
    }
    else
    {
        DeviceModelFamily = DEVICEMODEL_UNKNOWN;
    }

    /*
     * Request cellular antenna diagnostic
     */
    LE_INFO("Cellular antenna diagnostic:");

    AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef =
                                                    le_antenna_Request(LE_ANTENNA_PRIMARY_CELLULAR);
    LE_DEBUG("%p", AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef);
    LE_ASSERT(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef != 0);

    if(DeviceModelFamily == DEVICEMODEL_AR8_FAMILY)
    {
        LE_INFO("External ADC selected");

        // Test external ADC index 0
        result = le_antenna_SetExternalAdc(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef
                                  , 0);
        LE_ASSERT(result == LE_UNSUPPORTED);

        result = le_antenna_GetExternalAdc(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef
                                  , &antennaAdc);
        //LE_ASSERT((result == LE_OK)&&(antennaAdc == 0));
        LE_ASSERT(result == LE_UNSUPPORTED);

        // // Test external ADC index 1
        result = le_antenna_SetExternalAdc(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef
                                  , 1);
        LE_ASSERT(result == LE_OK);

        result = le_antenna_GetExternalAdc(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef
                                  , &antennaAdc);
        //LE_ASSERT((result == LE_OK)&&(antennaAdc == 1));
        LE_ASSERT(result == LE_UNSUPPORTED);
    }
    else
    {
        LE_INFO("Internal ADC selected by default");
        result = le_antenna_GetExternalAdc(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef
                                  , &antennaAdc);
        //LE_ASSERT((result == LE_OK)&&(antennaAdc == -1));
        LE_ASSERT(result == LE_UNSUPPORTED);
    }

    /*
     * Set the short limit: this limit is can be used to detect a close circuit using a 10kohms
     * resistance to simulate the antenna
     */
    result = le_antenna_SetShortLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                      PRIMARY_ANTENNA_SHORT_LIMIT);
    LE_ASSERT(result == LE_OK);

    /*
     * Set the open limit: this limit is can be used to detect an open circuit
     */
    result = le_antenna_SetOpenLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                     PRIMARY_ANTENNA_OPEN_LIMIT);
    LE_ASSERT(result == LE_OK);

    /*
     * Get the short and open limit, and check with the limits set before
     */
    result = le_antenna_GetOpenLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                     &openLimit);
    LE_ASSERT(result == LE_OK);
    LE_ASSERT(openLimit == PRIMARY_ANTENNA_OPEN_LIMIT);
    LE_INFO("openLimit %d", openLimit);

    result = le_antenna_GetShortLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                      &shortLimit);
    LE_ASSERT(result == LE_OK);
    LE_ASSERT(shortLimit == PRIMARY_ANTENNA_SHORT_LIMIT);
    LE_INFO("shortLimit %d", shortLimit);

    /*
     * Get the current state (the result depends on presence/absence of the 10kohms resistance)
     */
    result = le_antenna_GetStatus(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef, &status);
    LE_ASSERT(result == LE_OK);
    LE_INFO("cellular antenna status %d", status);

    /*
     * Subcribes to the status handler
     */
    AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].handlerRef = le_antenna_AddStatusEventHandler
                                        (
                                            AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                            AntennaHandler,
                                            NULL
                                        );

    LE_INFO("AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].handlerRef %p",
                                    AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].handlerRef);
    LE_ASSERT(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].handlerRef != 0);

    /*
     * Request diversity antenna diagnostic
     */
    if(DeviceModelFamily == DEVICEMODEL_AR7_FAMILY)
    {
        LE_INFO("Diversity antenna diagnostic:");
        AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef = le_antenna_Request(
                                                                        LE_ANTENNA_DIVERSITY_CELLULAR);
        LE_ASSERT(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef != 0);
        /*
         * Get the current limits
         */
        result = le_antenna_GetOpenLimit(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                                                                            &openLimit);
        LE_ASSERT(result == LE_OK);
        LE_INFO("openLimit %d", openLimit);

        result = le_antenna_GetShortLimit(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                        &shortLimit);
        LE_ASSERT(result == LE_OK);
        LE_INFO("shortLimit %d", shortLimit);

        /*
         * Set the short limit: this limit is can be used to detect a close circuit using a 10kohms
         * resistance to simulate the antenna
         */
        result = le_antenna_SetShortLimit(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                          PRIMARY_ANTENNA_SHORT_LIMIT);
        LE_ASSERT(result == LE_OK);

        /*
         * Set the open limit: this limit is can be used to detect an open circuit
         */
        result = le_antenna_SetOpenLimit(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                         PRIMARY_ANTENNA_OPEN_LIMIT);
        LE_ASSERT(result == LE_OK);

        /*
         * Get the current state (the result depends on presence/absence of the 10kohms resistance)
         */
        result = le_antenna_GetStatus(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                      &status);
        LE_ASSERT(result == LE_OK);
        LE_INFO("diversity antenna status %d", status);

        /*
         * Subscribes to the status handler
         */
        AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].handlerRef = le_antenna_AddStatusEventHandler (
                                            AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                            AntennaHandler,
                                            NULL
                                            );

        LE_INFO("handlerRef %p", AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].handlerRef);
        LE_ASSERT(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].handlerRef != 0);
    }
    else
    {
        LE_INFO("Diversity antenna diagnostic not tested for that platform");
    }

    /*
     * Request GNSS antenna diagnostic
     */
    LE_INFO("GNSS antenna diagnostic:");
    AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef = le_antenna_Request(LE_ANTENNA_GNSS);
    LE_ASSERT(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef != 0);

    if(DeviceModelFamily == DEVICEMODEL_AR8_FAMILY)
    {
        LE_INFO("External ADC selected");

        // Test external ADC index 0
        result = le_antenna_SetExternalAdc(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef
                                  , 0);
        LE_ASSERT((result == LE_OK)||(result == LE_UNSUPPORTED));

        // Test external ADC index 1 (already used for cellular diagnostic antenna)
        result = le_antenna_SetExternalAdc(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef
                                  , 1);
        LE_ASSERT(result == LE_FAULT);
        result = le_antenna_GetExternalAdc(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef
                                  , &antennaAdc);
        //LE_ASSERT((result == LE_OK)&&(antennaAdc == LE_ANTENNA_DIAG_NO_ADC));
        LE_ASSERT(result == LE_UNSUPPORTED);

        // Test external ADC index 2
        result = le_antenna_SetExternalAdc(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef
                                  , 2);
        LE_ASSERT(result == LE_OK);

        result = le_antenna_GetExternalAdc(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef
                                  , &antennaAdc);
        //LE_ASSERT((result == LE_OK)&&(antennaAdc == 2));
        LE_ASSERT(result == LE_UNSUPPORTED);
    }
    else
    {
        LE_INFO("Internal ADC selected");
        result = le_antenna_GetExternalAdc(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef
                                  , &antennaAdc);
        LE_ASSERT(result == LE_UNSUPPORTED);
    }

    /*
     * Get the current limits
     */
    result = le_antenna_GetOpenLimit(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, &openLimit);
    LE_ASSERT(result == LE_OK);
    LE_INFO("GNSS antenna openLimit %d", openLimit);

    result = le_antenna_GetShortLimit(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, &shortLimit);
    LE_ASSERT(result == LE_OK);
    LE_INFO("GNSS antenna shortLimit %d", shortLimit);

    /*
     * Set the short limit: this limit is can be used to detect a close circuit using a 10kohms
     * resistance to simulate the antenna
     */
    result = le_antenna_SetShortLimit(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, shortLimit);
    LE_ASSERT(result == LE_OK);

    /*
     * Set the open limit: this limit is can be used to detect an open circuit
     */
    result = le_antenna_SetOpenLimit(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, openLimit);
    LE_ASSERT(result == LE_OK);

    /*
     * Get the current state (the result depends on presence/absence of the 10kohms resistance)
     */
    result = le_antenna_GetStatus(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef, &status);
    LE_ASSERT(result == LE_OK);
    LE_INFO("GNSS antenna status %d", status);

    /*
     * Subscribe to the status handler
     */
    AntennaCtxtText[LE_ANTENNA_GNSS].handlerRef = le_antenna_AddStatusEventHandler (
                                        AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef,
                                        AntennaHandler,
                                        NULL
                                        );

    LE_INFO("GNSS antenna handlerRef %p", AntennaCtxtText[LE_ANTENNA_GNSS].handlerRef);
    LE_ASSERT(AntennaCtxtText[LE_ANTENNA_GNSS].handlerRef != 0);

    LE_INFO("======== Antenna diagnostic Test finished  ========");
}
