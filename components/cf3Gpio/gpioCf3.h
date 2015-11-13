//--------------------------------------------------------------------------------------------------
/**
 * CF3 implementation of the GPIO service
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef GPIOCF3_INTERFACE_H_INCLUDE_GUARD
#define GPIOCF3_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a GPIO object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct gpioCf3_Gpio* gpioCf3_GpioRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * When using GPIO pins we first need to specify in which mode we'd like to use it.
 * There are three modes into which a pin can be set.
 *
 * The type of GPIO pin mode,  Input, Output.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CF3_PIN_MODE_OUTPUT,
        ///< GPIO direction output mode, Assign a pin its value

    CF3_PIN_MODE_INPUT
        ///< GPIO direction input mode, Poll a pin to get its value
}
gpioCf3_PinMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * The polarity of GPIO level low or high.
 * Translates to the setting of "active_low" in sysfs
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CF3_ACTIVE_TYPE_HIGH,
        ///< GPIO Active-High, output signal is 1

    CF3_ACTIVE_TYPE_LOW
        ///< GPIO Active-Low
}
gpioCf3_ActiveType_t;

//--------------------------------------------------------------------------------------------------
/**
 * The value of GPIO low or high.
 * Translates to the setting of "value" in sysfs
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CF3_VALUE_LOW,
        ///< GPIO Low Value

    CF3_VALUE_HIGH
        ///< GPIO High Value
}
gpioCf3_Value_t;


//--------------------------------------------------------------------------------------------------
/**
 * The type of GPIO pullup, pulldown.
 *
 * PULLUPDOWN_TYPE_OFF:  pullup disable and pulldown disable
 * PULLUPDOWN_TYPE_DOWN: pullup disable and pulldown enable
 * PULLUPDOWN_TYPE_UP:   pullup enable and pulldown disable
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CF3_PULLUPDOWN_TYPE_OFF,
        ///< GPIO both pullup and pulldown disable

    CF3_PULLUPDOWN_TYPE_DOWN,
        ///< GPIO pulldown

    CF3_PULLUPDOWN_TYPE_UP
        ///< GPIO pullup
}
gpioCf3_PullUpDownType_t;


//--------------------------------------------------------------------------------------------------
/**
 * The mode of GPIO Edge Sensivity.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CF3_EDGE_SENSE_NONE,
        ///< None

    CF3_EDGE_SENSE_RISING,
        ///< Rising

    CF3_EDGE_SENSE_FALLING,
        ///< Falling

    CF3_EDGE_SENSE_BOTH
        ///< Both
}
gpioCf3_EdgeSensivityMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * The operation of GPIO open drain
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CF3_PUSH_PULL_OP,
        ///< Regular push-pull operation.

    CF3_OPEN_DRAIN_OP
        ///< Open drain operation.
}
gpioCf3_OpenDrainOperation_t;

//--------------------------------------------------------------------------------------------------
/**
 * Setup GPIO Direction INPUT or OUTPUT mode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetDirectionMode
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference.

    gpioCf3_PinMode_t mode
        ///< [IN]
        ///< The type of GPIO pin mode,  Input, Output.
);

//--------------------------------------------------------------------------------------------------
/**
 * Setup GPIO pullup/pulldown.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetPullUpDown
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_PullUpDownType_t pud
        ///< [IN]
        ///< The type of pullup, pulldown, off
);

//--------------------------------------------------------------------------------------------------
/**
 * Setup GPIO OpenDrain.
 *
 * Enables open drain operation for each output-configured IO.
 *
 * Output pins can be driven in two different modes:
 * - Regular push-pull operation: A transistor connects to high, and a transistor connects to low
 *   (only one is operated at a time)
 * - Open drain operation:  A transistor connects to low and nothing else
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetOpenDrain
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_OpenDrainOperation_t drain
        ///< [IN]
        ///< The operation of GPIO open drain
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetTriState
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_ActiveType_t polarity
        ///< [IN]
        ///< Active-high or active-low
);

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetHighZ
(
    gpioCf3_GpioRef_t gpioRef       ///< [IN] GPIO module object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Setup GPIO Polarity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetPolarity
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_ActiveType_t polarity
        ///< [IN]
        ///< Active-high or active-low
);

//--------------------------------------------------------------------------------------------------
/**
 * Setup PushPull Output.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetPushPullOutput
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_ActiveType_t polarity,
        ///< [IN]
        ///< Active-high or active-low

    bool value
        ///< [IN]
        ///< The initial state
);

//--------------------------------------------------------------------------------------------------
/**
 * Rising or Falling of Edge sensitivity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetEdgeSense
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_EdgeSensivityMode_t edge
        ///< [IN]
        ///< The mode of GPIO Edge Sensivity.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write value to GPIO output mode, low or high.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetOutput
(
    gpioCf3_GpioRef_t gpioRef,
        ///< [IN]
        ///< GPIO module object reference

    gpioCf3_Value_t value
        ///< [IN]
        ///< Low or High
);

//--------------------------------------------------------------------------------------------------
/**
 * Read value from GPIO input mode.
 *
 * @return This will return whether the pin is HIGH or LOW.
 */
//--------------------------------------------------------------------------------------------------
gpioCf3_Value_t gpioCf3_ReadInput
(
    gpioCf3_GpioRef_t gpioRef
        ///< [IN]
        ///< GPIO module object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a change callback on a particular pin. This only supports one
 * handler per pin
 *
 * @return This will return a reference.
 */
//--------------------------------------------------------------------------------------------------
void* gpioCf3_SetChangeCallback
(
    gpioCf3_GpioRef_t gpioRef,                    ///< [IN] GPIO object reference
    le_fdMonitor_HandlerFunc_t fdMonFunc,         ///< [IN] The fd monitor function
    gpioCf3_EdgeSensivityMode_t edge,             ///< [IN] Change(s) that should trigger the callback
    le_gpioPin2_ChangeCallbackFunc_t handlerPtr,  ///< [IN] The change callback
    void* contextPtr,                             ///< [IN] A context pointer
    int32_t sampleMs                              ///< [IN] If not interrupt capable, sample this often (ms).
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove a change callback on a particular pin
 *
 * @return This will return a reference.
 */
//--------------------------------------------------------------------------------------------------
void gpioCf3_RemoveChangeCallback
(
    gpioCf3_GpioRef_t gpioRef,         ///< [IN] GPIO object reference
    void * addHandlerRef               ///< [IN] The reference from when the handler was added
);

#endif // CF3_INTERFACE_H_INCLUDE_GUARD

