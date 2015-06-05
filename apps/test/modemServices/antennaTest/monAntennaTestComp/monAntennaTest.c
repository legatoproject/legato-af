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
 *
 * The status handler counts the number of changing state: after a configured state changement, the
 * handler is unregistered.
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "interfaces.h"


#define NB_CHANGING_STATE_BEFORE_RELEASING 3
#define SHORT_LIMIT 950//963
#define OPEN_LIMIT 1000//964

/*
 * context used for antenna diagnostics test
 */
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
    LE_INFO("Init");
    uint32_t openLimit=0, shortLimit = 0;
    le_antenna_Status_t status;

    /*
     * Request cellular antenna diagnostic
     */
    AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef =
                                                    le_antenna_Request(LE_ANTENNA_PRIMARY_CELLULAR);
    LE_DEBUG("%p", AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef);
    LE_ASSERT(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef != 0);

    /*
     * Set the short limit: this limit is can be used to detect a close circuit using a 10kohms
     * resistance to simulate the antenna
     */
    le_result_t result = le_antenna_SetShortLimit(
                                            AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                            SHORT_LIMIT);
    LE_ASSERT(result == LE_OK);

    /*
     * Set the open limit: this limit is can be used to detect an open circuit
     */
    result = le_antenna_SetOpenLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                                                                    OPEN_LIMIT);
    LE_ASSERT(result == LE_OK);

    /*
     * Get the short and open limit, and check with the limits set before
     */
    result = le_antenna_GetOpenLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                     &openLimit);
    LE_ASSERT(result == LE_OK);
    LE_ASSERT(openLimit == OPEN_LIMIT);
    LE_INFO("openLimit %d", openLimit);

    result = le_antenna_GetShortLimit(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef,
                                      &shortLimit);
    LE_ASSERT(result == LE_OK);
    LE_ASSERT(shortLimit == SHORT_LIMIT);
    LE_INFO("shortLimit %d", shortLimit);

    /*
     * Get the current state (the result depends on presence/absence of the 10kohms resistance)
     */
    result = le_antenna_GetStatus(AntennaCtxtText[LE_ANTENNA_PRIMARY_CELLULAR].antennaRef, &status);
    LE_ASSERT(result == LE_OK);
    LE_INFO("status antenna cellular %d", status);

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
                                      SHORT_LIMIT);
    LE_ASSERT(result == LE_OK);

    /*
     * Set the open limit: this limit is can be used to detect an open circuit
     */
    result = le_antenna_SetOpenLimit(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                     OPEN_LIMIT);
    LE_ASSERT(result == LE_OK);

    /*
     * Get the current state (the result depends on presence/absence of the 10kohms resistance)
     */
    result = le_antenna_GetStatus(AntennaCtxtText[LE_ANTENNA_DIVERSITY_CELLULAR].antennaRef,
                                  &status);
    LE_ASSERT(result == LE_OK);
    LE_INFO("status diversity cellular %d", status);

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

    /*
     * Request GNSS antenna diagnostic
     */
    AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef = le_antenna_Request(LE_ANTENNA_GNSS);
    LE_ASSERT(AntennaCtxtText[LE_ANTENNA_GNSS].antennaRef != 0);

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
}
