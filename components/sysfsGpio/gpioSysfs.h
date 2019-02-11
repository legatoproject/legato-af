//--------------------------------------------------------------------------------------------------
/**
 * Definitions of the generic functions for manipulating the Sysfs GPIO
 * interface presented by the Linux kernel
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef GPIOSYSFS_INTERFACE_H_INCLUDE_GUARD
#define GPIOSYSFS_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a GPIO object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct gpioSysfs_Gpio* gpioSysfs_GpioRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * State change event handler (callback).
 *
 * @param state
 *        New state of pin (true = active, false = inactive).
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*gpioSysfs_ChangeCallbackFunc_t)
(
    bool state,
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * The direction of the GPIO pin: Input, Output.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYSFS_PIN_MODE_OUTPUT,  ///< GPIO direction output
    SYSFS_PIN_MODE_INPUT    ///< GPIO direction input
}
gpioSysfs_PinMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * The polarity of GPIO level low or high.
 * Translates to the setting of "active_low" in sysfs
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYSFS_ACTIVE_TYPE_HIGH,  ///< GPIO Active-High, output signal is 1
    SYSFS_ACTIVE_TYPE_LOW    ///< GPIO Active-Low, output signal is 0
}
gpioSysfs_ActiveType_t;

//--------------------------------------------------------------------------------------------------
/**
 * The value of GPIO low or high.
 * Translates to the setting of "value" in sysfs
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYSFS_VALUE_LOW,   ///< GPIO Low Value
    SYSFS_VALUE_HIGH   ///< GPIO High Value
}
gpioSysfs_Value_t;


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
    SYSFS_PULLUPDOWN_TYPE_OFF,
    SYSFS_PULLUPDOWN_TYPE_DOWN,
    SYSFS_PULLUPDOWN_TYPE_UP
}
gpioSysfs_PullUpDownType_t;


//--------------------------------------------------------------------------------------------------
/**
 * The mode of GPIO Edge Sensivity.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYSFS_EDGE_SENSE_NONE,
    SYSFS_EDGE_SENSE_RISING,
    SYSFS_EDGE_SENSE_FALLING,
    SYSFS_EDGE_SENSE_BOTH
}
gpioSysfs_EdgeSensivityMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * The operation of GPIO open drain
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYSFS_PUSH_PULL_OP,   ///< Regular push-pull operation.
    SYSFS_OPEN_DRAIN_OP   ///< Open drain operation.
}
gpioSysfs_OpenDrainOperation_t;


//--------------------------------------------------------------------------------------------------
/**
 * The GPIO design currently found on the platform
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYSFS_GPIO_DESIGN_V1,  ///< Legacy V1 GPIO design in sysfs
    SYSFS_GPIO_DESIGN_V2,  ///< GPIO design V2 in sysfs (/sys/class/gpio/v2)
}
gpioSysfs_Design_t;

//--------------------------------------------------------------------------------------------------
/**
 * Setup GPIO pullup/pulldown.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetPullUpDown
(
    gpioSysfs_GpioRef_t gpioRef,     ///< [IN] GPIO module object reference
    gpioSysfs_PullUpDownType_t pud   ///< [IN] The type of pullup, pulldown, off
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
le_result_t gpioSysfs_SetOpenDrain
(
    gpioSysfs_GpioRef_t gpioRef,          ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity,      ///< [IN] Active-high or active-low
    bool value                            ///< [IN] Initial value to drive
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetTriState
(
    gpioSysfs_GpioRef_t gpioRef,      ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity   ///< [IN] Active-high or active-low
);

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetHighZ
(
    gpioSysfs_GpioRef_t gpioRef       ///< [IN] GPIO module object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Setup GPIO Polarity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetPolarity
(
    gpioSysfs_GpioRef_t gpioRef,      ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity   ///< [IN] Active-high or active-low
);

//--------------------------------------------------------------------------------------------------
/**
 * Setup PushPull Output.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetPushPullOutput
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity,   ///< [IN] Active-high or active-low
    bool value                         ///< [IN] The initial state
);

//--------------------------------------------------------------------------------------------------
/**
 * Rising or Falling of Edge sensitivity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetEdgeSense
(
    gpioSysfs_GpioRef_t gpioRef,         ///< [IN] GPIO module object reference
    gpioSysfs_EdgeSensivityMode_t edge   ///< [IN] The mode of GPIO Edge Sensivity.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read value from GPIO input mode.
 *
 * @return This will return whether the pin is HIGH or LOW.
 */
//--------------------------------------------------------------------------------------------------
gpioSysfs_Value_t gpioSysfs_ReadValue
(
    gpioSysfs_GpioRef_t gpioRef    ///< [IN] GPIO module object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetInput
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity    ///< [IN] Active-high or active-low.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set an output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_Activate
(
    gpioSysfs_GpioRef_t gpioRefPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_Deactivate
(
    gpioSysfs_GpioRef_t gpioRefPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a change callback on a particular pin. This only supports one
 * handler per pin
 *
 * @return This will return a reference.
 */
//--------------------------------------------------------------------------------------------------
void* gpioSysfs_SetChangeCallback
(
    gpioSysfs_GpioRef_t gpioRef,                  ///< [IN] GPIO object reference
    le_fdMonitor_HandlerFunc_t fdMonFunc,         ///< [IN] The fd monitor function
    gpioSysfs_EdgeSensivityMode_t edge,           ///< [IN] Change(s) that should trigger the callback
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
void gpioSysfs_RemoveChangeCallback
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO object reference
    void * addHandlerRef               ///< [IN] The reference from when the handler was added
);


//--------------------------------------------------------------------------------------------------
/**
 * Turn off edge detection
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_DisableEdgeSense
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of pull up and down resistors.
 *
 * @return The current configured value
 */
//--------------------------------------------------------------------------------------------------
gpioSysfs_PullUpDownType_t gpioSysfs_GetPullUpDown
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the current value the pin polarity.
 *
 * @return The current configured value
 */
//--------------------------------------------------------------------------------------------------
gpioSysfs_ActiveType_t gpioSysfs_GetPolarity
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the current value of edge sensing.
 *
 * @return The current configured value
 *
 * @note it is invalid to read the edge sense of an output
 */
//--------------------------------------------------------------------------------------------------
gpioSysfs_EdgeSensivityMode_t gpioSysfs_GetEdgeSense
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if the pin is currently active.
 *
 * @return true = active, false = inactive.
 *
 * @note this function can only be used on output pins
 */
//--------------------------------------------------------------------------------------------------
bool gpioSysfs_IsActive
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if the pin is configured as an output.
 *
 * @return true = output, false = input.
 */
//--------------------------------------------------------------------------------------------------
bool gpioSysfs_IsOutput
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if the pin is configured as an input.
 *
 * @return true = input, false = output.
 */
//--------------------------------------------------------------------------------------------------
bool gpioSysfs_IsInput
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
);

//--------------------------------------------------------------------------------------------------
/**
 * This function will be called when there is a state change on a GPIO
 *
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_InputMonitorHandlerFunc
(
    const gpioSysfs_GpioRef_t gpioRefPtr,
    int fd,
    short events
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to be called when the client-server session opens. This allows the relationship
 * between the session and the GPIO reference to be created.
 * A service using this module to interact with the sysfs should register this function
 * with the low-level messaging API using le_msg_AddServiceOpenHandler.
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_SessionOpenHandlerFunc
(
    le_msg_SessionRef_t  sessionRef,  ///<[IN] Client session reference.
    void*                contextPtr   ///<[IN] Client context pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to be called when the client-server session closes.
 * A service using this module to interact with the sysfs should register this function
 * with the low-level messaging API using le_msg_AddServiceCloseHandler.
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_SessionCloseHandlerFunc
(
    le_msg_SessionRef_t  sessionRef,  ///<[IN] Client session reference.
    void*                contextPtr   ///<[IN] Client context pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Determine if a GPIO PIn is available for use. This is done by reading the value of
 * /sys/class/gpio/gpiochip1 (on our platforms). The sysfs doc describes this file as follows...
 *
 * GPIO controllers have paths like /sys/class/gpio/gpiochip42/ (for the
 * controller implementing GPIOs starting at #42) and have the following
 * read-only attributes:
 *
 *  /sys/class/gpio/gpiochipN/
 */
//--------------------------------------------------------------------------------------------------
bool gpioSysfs_IsPinAvailable
(
    int pinNum         ///< [IN] GPIO pin number (starting at 1)
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the GPIO sys fs and return the GPIO design found:
 *   - SYSFS_GPIO_DESIGN_V2 if /sys/class/gpio/v2/alias_export exists and is writable,
 *   - SYSFS_GPIO_DESIGN_V1 else.
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_Initialize
(
    gpioSysfs_Design_t* gpioDesignPtr    ///< [OUT] Current GPIO design for sysfs
);

//--------------------------------------------------------------------------------------------------
/**
 * The struct of Sysfs object
 */
//--------------------------------------------------------------------------------------------------
struct gpioSysfs_Gpio {
    uint8_t pinNum;                               ///< GPIO Pin number
    const char* gpioName;                         ///< GPIO Signal Name
    bool inUse;                                   ///< Is the GPIO currently used?
    gpioSysfs_ChangeCallbackFunc_t handlerPtr;    ///< Change callback handler, if registered
    void *callbackContextPtr;                     ///< Client context to be passed back
    le_fdMonitor_Ref_t fdMonitor;                 ///< fdMonitor Object associated to this GPIO
    le_msg_SessionRef_t currentSession;           ///< Current valid IPC session for this pin
};


#endif // GPIOSYSFS_INTERFACE_H_INCLUDE_GUARD

