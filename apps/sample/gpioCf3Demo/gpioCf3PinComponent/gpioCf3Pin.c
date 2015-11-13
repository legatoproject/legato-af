/**
 * @file gpioPin.c
 *
 * This is sample Legato CF3 GPIO app by using le_gpio.api.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

/* Legato Framework */
#include "legato.h"
#include "interfaces.h"

static int Pin22 = 22;

static void Pin22ChangeCallback(bool state, void *ctx){
    LE_INFO("State change %s", state?"TRUE":"FALSE");

    LE_INFO("Context pointer came back as %d", *(int *)ctx);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Pin-per-service GPIO pin21 as example
 */
// -------------------------------------------------------------------------------------------------
static void Pin21GpioSignal()
{
    bool active;

    le_gpioPin21_Activate();
    le_gpioPin21_EnablePullUp();
    le_gpioPin21_Deactivate();

    le_gpioPin21_SetInput(1);
    active = le_gpioPin21_Read();
    LE_INFO("Pin21 read active: %d", active);
}

//-------------------------------------------------------------------------------------------------
/**
 *  Pin-per-service GPIO pin22 as example
 */
// -------------------------------------------------------------------------------------------------
static void Pin22GpioSignal()
{
    bool value = false;
    le_gpioPin22_SetInput(LE_GPIOPIN22_ACTIVE_LOW);
    value = le_gpioPin21_Read();
    LE_INFO("Pin22 read active: %d", value);

    le_gpioPin22_AddChangeEventHandler(LE_GPIOPIN22_EDGE_FALLING, Pin22ChangeCallback, &Pin22, 0);
}

COMPONENT_INIT
{
    LE_INFO("This is sample gpioctl Legato CF3 GPIO app by using le_gpio.api\n");

    Pin21GpioSignal();
    Pin22GpioSignal();

    return;
}

