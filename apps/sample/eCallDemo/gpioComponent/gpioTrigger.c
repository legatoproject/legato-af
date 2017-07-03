/**
 * @file gpioTrigger.c
 *
 * This module sets up a GPIO trigger (pin 2) for starting an eCall
 * and outputs its state to pin 13.
 *
 * You can activate the trigger and monitor by issuing the command:
 * @verbatim
   $ app runProc eCallDemo --exe=gpio
   @endverbatim
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 *  The number of passengers to transmit.
 */
//--------------------------------------------------------------------------------------------------
#define NUM_PASSENGERS 4


//--------------------------------------------------------------------------------------------------
/**
 *  Callback fired when pin 2 is grounded.
 */
//--------------------------------------------------------------------------------------------------
static void Pin2ChangeCallback
(
    bool   state,        ///< [IN] State of pin
    void*  contextPtr    ///< [IN] Context pointer
)
{
    LE_INFO("GPIO triggered. Starting eCallDemo with.%d passengers", NUM_PASSENGERS);

    // Starts the ecall Session
    ecallApp_StartSession(NUM_PASSENGERS);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Registering the handler for pin 2 to trigger callback for any state change of pin 2.
 */
//--------------------------------------------------------------------------------------------------
static void RegisterGPIOHandler
(
    void
)
{
    // Enable the pull-up resistor (disables pull-down if previously enabled)
    le_gpioPin2_EnablePullUp();

    // Configure the pin as an input pin
    le_gpioPin2_SetInput(LE_GPIOPIN2_ACTIVE_LOW);

    // Register a callback function to be called when an input pin changes state
    le_gpioPin2_AddChangeEventHandler(LE_GPIOPIN2_EDGE_RISING, Pin2ChangeCallback, NULL, 0);

    // Set the edge detection mode
    le_gpioPin2_SetEdgeSense(LE_GPIOPIN2_EDGE_RISING);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Callback fired when eCall state changes.
 */
//--------------------------------------------------------------------------------------------------
static void ECallStateChangeHandler
(
    le_ecall_CallRef_t  eCallRef,     ///< [IN] eCall reference
    le_ecall_State_t    state,        ///< [IN] eCall state
    void*               contextPtr    ///< [IN] Context pointer
)
{
    if (LE_ECALL_STATE_CONNECTED == state)
    {
        // Set output pin to active state
        le_gpioPin13_Activate();
    }
    else if (LE_ECALL_STATE_DISCONNECTED == state)
    {
        // Set output pin to inactive state
        le_gpioPin13_Deactivate();
    }
    else
    {
        LE_INFO("Ecall state = %d", state);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Registering the handler for ECall.
 */
//--------------------------------------------------------------------------------------------------
static void RegisterECallHandler
(
    void
)
{
    LE_ERROR_IF((!le_ecall_AddStateChangeHandler(ECallStateChangeHandler, NULL)),
                "Unable to add an eCall state change handler!");
}

//--------------------------------------------------------------------------------------------------
/**
 *  Reads the status of the pins.
 */
//--------------------------------------------------------------------------------------------------
static void PinsReadConfig
(
    void
)
{
    LE_INFO("Pin 2 active = %s", le_gpioPin2_IsActive()?"true":"false");

    LE_INFO("Pin 13 active = %s", le_gpioPin13_IsActive()?"true":"false");

    // Get the current value of edge sensing
    le_gpioPin2_Edge_t edge = le_gpioPin2_GetEdgeSense();
    if (LE_GPIOPIN2_EDGE_FALLING == edge)
    {
        LE_INFO("Pin 2 edge sense = falling");
    }
    else if (LE_GPIOPIN2_EDGE_RISING == edge)
    {
        LE_INFO("Pin 2 edge sense = rising");
    }
    else if (LE_GPIOPIN2_EDGE_BOTH == edge)
    {
        LE_INFO("Pin 2 edge sense = both");
    }
    else if (LE_GPIOPIN2_EDGE_NONE == edge)
    {
        LE_INFO("Pin 2 edge sense = none");
    }

    // Get the current value the pin polarity
    le_gpioPin2_Polarity_t pol = le_gpioPin2_GetPolarity();
    if (LE_GPIOPIN2_ACTIVE_HIGH == pol)
    {
        LE_INFO("Pin 2 polarity = ACTIVE_HIGH");
    }
    else
    {
        LE_INFO("Pin 2 polarity = ACTIVE_LOW");
    }

    LE_INFO("Pin 2 is input = %s", le_gpioPin2_IsInput()?"true":"false");
    LE_INFO("Pin 2 is output = %s", le_gpioPin2_IsOutput()?"true":"false");

    // Get the current value of pull up and down resistors
    le_gpioPin2_PullUpDown_t pud = le_gpioPin2_GetPullUpDown();
    if (LE_GPIOPIN2_PULL_DOWN == pud)
    {
        LE_INFO("Pin 2 pull up/down = down");
    }
    else if (LE_GPIOPIN2_PULL_UP == pud)
    {
        LE_INFO("Pin 2 pull up/down = up");
    }
    else
    {
        LE_INFO("Pin 2  pull up/down = none");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the GPIO component.
 *
 * Execute application with 'app runProc eCallDemo --exe=gpio'
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start GPIO component");

    // Connect the current client thread to the service
    le_gpioPin13_ConnectService();
    le_gpioPin2_ConnectService();

    // Set output pin to inactive state
    le_gpioPin13_Deactivate();

    RegisterECallHandler();
    RegisterGPIOHandler();
    PinsReadConfig();
}
