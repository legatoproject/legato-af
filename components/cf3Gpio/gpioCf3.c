/**
 * @file gpioCf3.c
 *
 * Prototype GPIO Cf3 API interface.
 * The GPIO API implementation for Cf3 devices (WP85). Some of the features
 * of the generic API are not supported.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "gpioCf3.h"

//--------------------------------------------------------------------------------------------------
/**
 * GPIO signals have paths like /sys/class/gpio/gpio42/ (for GPIO #42)
 */
//--------------------------------------------------------------------------------------------------
#define SYSFS_GPIO_PATH    "/sys/class/gpio"

//--------------------------------------------------------------------------------------------------
/* From 4116440 WP8548 Product Technical Specification v6 - Draft A.pdf
 * Table 4.8: GPIO Pin Description
 * - Pin 10 : GPIO2
 * - Pin 40 : GPIO7
 * - Pin 41 : GPIO8
 * - Pin 44 : GPIO13
 * - Pin 104: GPIO32
 * - Pin 105: GPIO33
 * - Pin 109: GPIO42
 * - Pin 147: GPIO21
 * - Pin 148: GPIO22
 * - Pin 149: GPIO23
 * - Pin 150: GPIO24
 * - Pin 159: GPIO25
 *
 * The GPIO pin mapping doesn't work as above Pin Description in Specification on WP85 currently.
 * The actual GPIO pin mapping actually is gpio number, not CF3 pin number. So no definition
 *
 * If later kernel sysfs code is updated as GPIO pin Description, the definition below should be
 * uncomment.
 */
//--------------------------------------------------------------------------------------------------
//#define CF3_GPIO_PIN_MAPPING 1


//--------------------------------------------------------------------------------------------------
/**
 * Maximum GPIO Pin Number supported
 */
//--------------------------------------------------------------------------------------------------
#define MAX_GPIO_PIN_NUMBER  256

//--------------------------------------------------------------------------------------------------
/**
 * Pin mapping array as Pin Description
 */
//--------------------------------------------------------------------------------------------------
static int gpioCf3PinMap[MAX_GPIO_PIN_NUMBER];

//--------------------------------------------------------------------------------------------------
/**
 * The struct of Cf3 object
 */
//--------------------------------------------------------------------------------------------------
struct gpioCf3_Gpio {
    gpioCf3_PinMode_t mode;                       ///< Output, Input or Interrupt mode
    gpioCf3_ActiveType_t level;                   ///< Active High or Low
    gpioCf3_PullUpDownType_t pud;                 ///< Pullup or Pulldown type
    uint8_t pinNum;                               ///< GPIO Pin number
    char gpioName[16];                            ///< GPIO Signal Name
    bool inUse;                                   ///< Is the GPIO currently used?
    le_gpioPin2_ChangeCallbackFunc_t handlerPtr;  ///< Change callback handler, if registered
    void *callbackContextPtr;                     ///< Client context to be passed back
    le_fdMonitor_Ref_t fdMonitor;                 ///< fdMonitor Object associated to this GPIO
};


//--------------------------------------------------------------------------------------------------
/**
 * Initialise the static data for the pins we are advertising
 */
//--------------------------------------------------------------------------------------------------

static struct gpioCf3_Gpio Cf3GpioPin2 = {0,0,0,2,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin2 = &Cf3GpioPin2;

static struct gpioCf3_Gpio Cf3GpioPin7 = {0,0,0,7,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin7 = &Cf3GpioPin7;

static struct gpioCf3_Gpio Cf3GpioPin8 = {0,0,0,8,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin8 = &Cf3GpioPin8;

static struct gpioCf3_Gpio Cf3GpioPin13 = {0,0,0,13,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin13 = &Cf3GpioPin13;

static struct gpioCf3_Gpio Cf3GpioPin21 = {0,0,0,21,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin21 = &Cf3GpioPin21;

static struct gpioCf3_Gpio Cf3GpioPin22 = {0,0,0,22,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin22 = &Cf3GpioPin22;

static struct gpioCf3_Gpio Cf3GpioPin23 = {0,0,0,23,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin23 = &Cf3GpioPin23;

static struct gpioCf3_Gpio Cf3GpioPin24 = {0,0,0,24,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin24 = &Cf3GpioPin24;

static struct gpioCf3_Gpio Cf3GpioPin25 = {0,0,0,25,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin25 = &Cf3GpioPin25;

static struct gpioCf3_Gpio Cf3GpioPin32 = {0,0,0,32,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin32 = &Cf3GpioPin32;

static struct gpioCf3_Gpio Cf3GpioPin33 = {0,0,0,33,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin33 = &Cf3GpioPin33;

static struct gpioCf3_Gpio Cf3GpioPin42 = {0,0,0,42,"",false,NULL,NULL,NULL};
static gpioCf3_GpioRef_t gpioRefPin42 = &Cf3GpioPin42;

//--------------------------------------------------------------------------------------------------
/**
 * Check if CF3 sysfs gpio path exists.
 * @return
 * - true: gpio path exists
 * - false: gpio path does not exist
 */
//--------------------------------------------------------------------------------------------------
static bool CheckGpioPathExist
(
    const char *path
)
{
    DIR* dir;

    dir = opendir(path);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if CF3 sysfs gpio path exists.
 * @return
 * - true: gpio path exists
 * - false: gpio path does not exist
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExportGpio
(
    const gpioCf3_GpioRef_t gpioRefPtr
)
{
    char path[128];
    char export[128];
    char gpioStr[8];
    FILE *fp = NULL;

    // First check if the GPIO has already been exported
    snprintf(path, sizeof(path), "%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName);
    if (CheckGpioPathExist(path))
    {
        return LE_OK;
    }

    // Write the GPIO number to the export file
    snprintf(export, sizeof(export), "%s/%s", SYSFS_GPIO_PATH, "export");
    snprintf(gpioStr, sizeof(gpioStr), "%d", gpioRefPtr->pinNum);
    do
    {
        fp = fopen(export, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if(!fp) {
        LE_ERROR("Error opening file %s for writing.\n", export);
        return LE_IO_ERROR;
    }

    ssize_t written = fwrite(gpioStr, 1, strlen(gpioStr), fp);
    fflush(fp);

    int file_err = ferror(fp);
    fclose(fp);
    if (file_err != 0)
    {
        LE_EMERG("Failed to export GPIO %s. Error %s", gpioStr, strerror(file_err));
        return LE_IO_ERROR;
    }

    if (written < strlen(gpioStr))
    {
        LE_EMERG("Data truncated while exporting GPIO %s.", gpioStr);
        return LE_IO_ERROR;
    }

    // Now check again that it has been exported
    if (CheckGpioPathExist(path))
    {
        return LE_OK;
    }
    LE_EMERG("Failed to export GPIO %s.", gpioStr);
    return LE_IO_ERROR;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set sysfs GPIO signals attributes
 *
 * GPIO signals have paths like /sys/class/gpio/gpioN/
 * and have the following read/write attributes:
 * - "direction"
 * - "value"
 * - "edge"
 * - "active_low"
 * - "pull"
 *
 * @return
 * - LE_IO_ERROR: write sysfs gpio error
 * - LE_OK: successfully
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteSysGpioSignalAttr
(
    const char *path,        ///< [IN] path to sysfs gpio signal
    const char *attr         ///< [IN] GPIO signal write attribute
)
{
    FILE *fp = NULL;

    if (!CheckGpioPathExist(path))
    {
        LE_KILL_CLIENT("GPIO %s does not exist (probably not exported)", path);
        return LE_BAD_PARAMETER;
    }

    do
    {
        fp = fopen(path, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if(!fp) {
        LE_ERROR("Error opening file %s for writing.\n", path);
        return LE_IO_ERROR;
    }

    ssize_t written = fwrite(attr, 1, strlen(attr), fp);
    fflush(fp);

    int file_err = ferror(fp);
    if (file_err != 0)
    {
        LE_EMERG("Failed to write %s to GPIO config %s. Error %s", attr, path, strerror(file_err));
        fclose(fp);
        return LE_IO_ERROR;
    }

    if (written < strlen(attr))
    {
        LE_EMERG("Data truncated while writing %s to GPIO config %s.", path, attr);
        fclose(fp);
        return LE_IO_ERROR;
    }
    fclose(fp);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get sysfs GPIO attribute
 *
 * GPIO signals have paths like /sys/class/gpio/gpioN/
 * and have the following read/write attributes:
 * - "direction"
 * - "value"
 * - "edge"
 * - "active_low"
 * - "pull"
 *
 * @return
 * - -1: write sysfs gpio error
 * -  0: successfully
 */
//--------------------------------------------------------------------------------------------------
static int ReadSysGpioSignalAttr
(
    const char *path,        ///< [IN] path to sysfs gpio
    int attr_size,           ///< [IN] the size of attribute content
    char *attr               ///< [OUT] GPIO signal read attribute content
)
{
    int i;
    char c;
    FILE *fp = NULL;
    char *result = attr;

    if (!CheckGpioPathExist(path))
    {
        LE_KILL_CLIENT("File %s does not exist", path);
        return LE_BAD_PARAMETER;
    }

    do
    {
        fp = fopen(path, "r");
    }
    while ((fp == NULL) && (errno == EINTR));

    if(!fp) {
        LE_ERROR("Error opening file %s for reading.\n", path);
        return LE_IO_ERROR;
    }

    i = 0;
    while((c = fgetc(fp)) != EOF && i < attr_size) {
        result[i] = c;
        i++;
    }
    result[i] = '\0';
    fclose (fp);

    LE_DEBUG("Read result: %s from %s", result, path);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function will be called when there is a state change on a GPIO
 *
 */
//--------------------------------------------------------------------------------------------------
static void InputMonitorHandlerFunc
(
    const gpioCf3_GpioRef_t gpioRefPtr,
    int fd,
    short events
)
{
    //We're reading a single character
    char buf[1];

    LE_DEBUG("Input handler called for %s", gpioRefPtr->gpioName);

    // Seek to the start of the file - this is required to prevent
    // repeated triggers - see https://www.kernel.org/doc/Documentation/gpio/sysfs.txt
    LE_DEBUG("Seek to start of file %d", fd);
    lseek(fd, 0, SEEK_SET);

    if (read(fd, buf, 1) != 1)
    {
        LE_ERROR("Unable to read value for GPIO %s", gpioRefPtr->gpioName);
        return;
    }

    LE_DEBUG("Read value %c from value file for callback", buf[0]);

    // Look up the callback function
    le_gpioPin2_ChangeCallbackFunc_t handlerPtr = gpioRefPtr->handlerPtr;

    if (handlerPtr != NULL)
    {
        LE_INFO("Calling change callback for %s", gpioRefPtr->gpioName);
        handlerPtr(buf[0]=='1', gpioRefPtr->callbackContextPtr);
    }
    else
    {
        LE_WARN("No callback registered for pin %s", gpioRefPtr->gpioName);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO Direction INPUT or OUTPUT mode.
 *
 * "direction" ... reads as either "in" or "out". This value may
 *        normally be written. Writing as "out" defaults to
 *        initializing the value as low. To ensure glitch free
 *        operation, values "low" and "high" may be written to
 *        configure the GPIO as an output with that initial value
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetDirectionMode
(
    gpioCf3_GpioRef_t gpioRefPtr,    ///< [IN] gpio reference
    gpioCf3_PinMode_t mode           ///< [IN] gpio direction input/output mode
)
{
    char path[64];
    char attr[16];

    if (!gpioRefPtr || gpioRefPtr->pinNum == -1)
    {
        LE_ERROR("gpioRefPtr is NULL or object not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName, "direction");
    snprintf(attr, sizeof(attr), "%s", (mode == CF3_PIN_MODE_OUTPUT) ? "out": "in");
    LE_DEBUG("path:%s, attribute:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}


//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO pullup or pulldown disable/enable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetPullUpDown
(
    gpioCf3_GpioRef_t gpioRefPtr,    ///< [IN] gpio object reference
    gpioCf3_PullUpDownType_t pud     ///< [IN] pull up, pull down type
)
{
    char path[64];
    char attr[16];

    if (!gpioRefPtr || gpioRefPtr->pinNum == -1)
    {
        LE_ERROR("gpioRefPtr is NULL or object not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName, "pull");
    snprintf(attr, sizeof(attr), "%s", (pud == CF3_PULLUPDOWN_TYPE_DOWN) ? "down": "up");
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set up PushPull Output.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetPushPullOutput
(
    gpioCf3_GpioRef_t gpioRef,
    gpioCf3_ActiveType_t polarity,
    bool value
)
{
    LE_WARN("PushPullOUtput API not implemented in CF3 sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO OpenDrain.
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
    gpioCf3_GpioRef_t gpioRefPtr,            ///< [IN] gpio object reference
    gpioCf3_OpenDrainOperation_t drainOp     ///< [IN] The operation of GPIO open drain
)
{
    LE_WARN("Open Drain API not implemented in CF3 sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetTriState
(
    gpioCf3_GpioRef_t gpioRef,       ///< [IN] GPIO module object reference
    gpioCf3_ActiveType_t polarity    ///< [IN] Active-high or active-low
)
{
    LE_WARN("Tri-State API not implemented in CF3 sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}

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
)
{
    LE_WARN("SetHighZ API not implemented in CF3 sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO polarity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetPolarity
(
    gpioCf3_GpioRef_t gpioRefPtr,           ///< [IN] gpio object reference
    gpioCf3_ActiveType_t level              ///< [IN] Active-high or active-low
)
{
    char path[64];
    char attr[16];

    if (!gpioRefPtr || gpioRefPtr->pinNum == -1)
    {
        LE_ERROR("gpioRefPtr is NULL or gpio not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName, "active_low");
    snprintf(attr, sizeof(attr), "%d", level);
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * write value to GPIO output, low or high
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetOutput
(
    gpioCf3_GpioRef_t gpioRefPtr,           ///< [IN] gpio object reference
    gpioCf3_Value_t level                   ///< [IN] High or low
)
{
    char path[64];
    char attr[16];

    if (!gpioRefPtr || gpioRefPtr->pinNum == -1)
    {
        LE_ERROR("gpioRefPtr is NULL or gpio not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName, "value");
    snprintf(attr, sizeof(attr), "%d", level);
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a change callback on a particular pin
 *
 * @return This will return whether the pin is HIGH or LOW.
 */
//--------------------------------------------------------------------------------------------------
void* gpioCf3_SetChangeCallback
(
    gpioCf3_GpioRef_t gpioRef,         ///< [IN] GPIO object reference
    le_fdMonitor_HandlerFunc_t fdMonFunc,  ///< [IN] The fd monitor function
    gpioCf3_EdgeSensivityMode_t edge,  ///< [IN] Edge detection mode.
    le_gpioPin2_ChangeCallbackFunc_t handlerPtr,  ///< [IN]
    void* contextPtr,                  ///< [IN]
    int32_t sampleMs                   ///< [IN] If not interrupt capable, sample this often (ms).
)
{
    char monFile[128];
    int monFd = 0;

    // Only one handler is allowed here
    if (gpioRef->fdMonitor != NULL)
    {
        LE_KILL_CLIENT("Only one change handler can be registered");
    }

    // Set the edge detection mode
    if (LE_OK != gpioCf3_SetEdgeSense(gpioRef, edge))
    {
        LE_KILL_CLIENT("Unable to set edge detection correctly");
    }

    // Store the callback function and context pointer
    gpioRef->handlerPtr = handlerPtr;
    gpioRef->callbackContextPtr = contextPtr;

    // Start monitoring the fd for the correct GPIO
    snprintf(monFile, sizeof(monFile), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRef->gpioName, "value");

    do
    {
        monFd = open(monFile, O_RDONLY);
    }
    while ((monFd == 0) && (errno == EINTR));

    if (monFd == 0)
    {
        LE_KILL_CLIENT("Unable to open GPIO file for monitoring");
    }

    LE_DEBUG("Setting up file monitor for fd %d and pin %s", monFd, gpioRef->gpioName);
    gpioRef->fdMonitor = le_fdMonitor_Create (gpioRef->gpioName, monFd, fdMonFunc, POLLPRI);

    return &(gpioRef->gpioName);
}

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
)
{
    // We should check the reference here, but only one handler is allowed
    // so it isn't that important

    // If there is an fd monitor then stop it
    if (gpioRef->fdMonitor != NULL)
    {
        le_fdMonitor_Delete(gpioRef->fdMonitor);
        gpioRef->fdMonitor = NULL;
    }

    // If there is a callback registered then forget it
    gpioRef->callbackContextPtr = NULL;
    gpioRef->handlerPtr = NULL;
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * read value from GPIO input mode.
 *
 *
 * "value" ... reads as either 0 (low) or 1 (high). If the GPIO
 *        is configured as an output, this value may be written;
 *        any nonzero value is treated as high.
 *
 * @return
 *      An active type, the status of pin: HIGH or LOW
 */
//--------------------------------------------------------------------------------------------------
gpioCf3_Value_t gpioCf3_ReadInput
(
    gpioCf3_GpioRef_t gpioRefPtr            ///< [IN] gpio object reference
)
{
    char path[64];
    char result[16];
    gpioCf3_ActiveType_t type;

    if (!gpioRefPtr || gpioRefPtr->pinNum == -1)
    {
        LE_ERROR("gpioRefPtr is NULL or object not initialized");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName, "value");
    ReadSysGpioSignalAttr(path, sizeof(result), result);
    type = atoi(result);
    LE_DEBUG("result:%s Value:%s", result, (type==1) ? "high": "low");

    return type;
}

//--------------------------------------------------------------------------------------------------
/**
 * Rising or Falling of Edge sensitivity
 *
 * "edge" ... reads as either "none", "rising", "falling", or
 * "both". Write these strings to select the signal edge(s)
 * that will make poll(2) on the "value" file return.

 * This file exists only if the pin can be configured as an
 * interrupt generating input pin.

 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_SetEdgeSense
(
    gpioCf3_GpioRef_t gpioRefPtr,           ///< [IN] gpio object reference
    gpioCf3_EdgeSensivityMode_t edge        ///< [IN] The mode of GPIO Edge Sensivity.
)
{
    char path[64];
    char attr[16];

    if (!gpioRefPtr || gpioRefPtr->pinNum == -1)
    {
        LE_ERROR("gpioRefPtr is NULL or object not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, gpioRefPtr->gpioName, "edge");

    switch(edge)
    {
        case CF3_EDGE_SENSE_RISING:
            snprintf(attr, 15, "rising");
            break;
        case CF3_EDGE_SENSE_FALLING:
            snprintf(attr, 15, "falling");
            break;
        case CF3_EDGE_SENSE_BOTH:
            snprintf(attr, 15, "both");
            break;
        default:
            snprintf(attr, 15, "none");
            break;
    }
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set an output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_Activate
(
    gpioCf3_GpioRef_t gpioRefPtr
)
{
    if (LE_OK != gpioCf3_SetDirectionMode(gpioRefPtr, CF3_PIN_MODE_OUTPUT))
    {
        LE_ERROR("Failed to set Direction on GPIO %s", gpioRefPtr->gpioName);
        return LE_IO_ERROR;
    }

    if (LE_OK != gpioCf3_SetOutput(gpioRefPtr, CF3_VALUE_HIGH))
    {
        LE_ERROR("Failed to set GPIO %s to high", gpioRefPtr->gpioName);
        return LE_IO_ERROR;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioCf3_Deactivate
(
    gpioCf3_GpioRef_t gpioRefPtr
)
{
    if (LE_OK != gpioCf3_SetDirectionMode(gpioRefPtr, CF3_PIN_MODE_OUTPUT))
    {
        LE_ERROR("Failed to set Direction on GPIO %s", gpioRefPtr->gpioName);
        return LE_IO_ERROR;
    }

    if (LE_OK != gpioCf3_SetOutput(gpioRefPtr, CF3_VALUE_LOW))
    {
        LE_ERROR("Failed to set GPIO %s to low", gpioRefPtr->gpioName);
        return LE_IO_ERROR;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin2_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin2, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin7_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin7, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin8_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin8, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin13_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin13, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin21_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin21, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin22_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin22, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin23_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin23, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin24_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin24, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin25_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin25, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin32_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin32, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin33_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin33, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Callback to enable us to distinguish which pin has changed state
 */
//--------------------------------------------------------------------------------------------------
void gpioPin42_InputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    InputMonitorHandlerFunc(gpioRefPin42, fd, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_SetInput
(
    le_gpioPin2_Polarity_t polarity  ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin2, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_SetInput
(
    le_gpioPin7_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin7, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_SetInput
(
    le_gpioPin8_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin8, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_SetInput
(
    le_gpioPin13_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin13, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_SetInput
(
    le_gpioPin21_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin21, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_SetInput
(
    le_gpioPin22_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin22, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_SetInput
(
    le_gpioPin23_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin23, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_SetInput
(
    le_gpioPin24_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin24, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_SetInput
(
    le_gpioPin25_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin25, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_SetInput
(
    le_gpioPin32_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin32, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_SetInput
(
    le_gpioPin33_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin33, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_SetInput
(
    le_gpioPin42_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetDirectionMode(gpioRefPin42, CF3_PIN_MODE_INPUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_SetPushPullOutput
(
    le_gpioPin2_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin2, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_SetPushPullOutput
(
    le_gpioPin7_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin7, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_SetPushPullOutput
(
    le_gpioPin8_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin8, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_SetPushPullOutput
(
    le_gpioPin13_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin13, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_SetPushPullOutput
(
    le_gpioPin21_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin21, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_SetPushPullOutput
(
    le_gpioPin22_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin22, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_SetPushPullOutput
(
    le_gpioPin23_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin23, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_SetPushPullOutput
(
    le_gpioPin24_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin24, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_SetPushPullOutput
(
    le_gpioPin25_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin25, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_SetPushPullOutput
(
    le_gpioPin32_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin32, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_SetPushPullOutput
(
    le_gpioPin33_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin33, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a push-pull output pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_SetPushPullOutput
(
    le_gpioPin42_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetPushPullOutput(gpioRefPin42, polarity, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_SetOpenDrainOutput
(
    le_gpioPin2_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin2, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_SetOpenDrainOutput
(
    le_gpioPin7_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin7, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_SetOpenDrainOutput
(
    le_gpioPin8_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin8, polarity);

}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_SetOpenDrainOutput
(
    le_gpioPin13_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin13, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_SetOpenDrainOutput
(
    le_gpioPin21_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin21, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_SetOpenDrainOutput
(
    le_gpioPin22_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin22, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_SetOpenDrainOutput
(
    le_gpioPin23_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin23, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_SetOpenDrainOutput
(
    le_gpioPin24_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin24, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_SetOpenDrainOutput
(
    le_gpioPin25_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin25, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_SetOpenDrainOutput
(
    le_gpioPin32_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin32, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_SetOpenDrainOutput
(
    le_gpioPin33_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin33, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an open-drain output pin.  "High" is a high-impedance state, while "Low"
 * pulls the pin to ground.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_SetOpenDrainOutput
(
    le_gpioPin42_Polarity_t polarity,
        ///< [IN]
        ///< Active-high or active-low.

    bool value
        ///< [IN]
        ///< Initial value to drive (true = active, false = inactive)
)
{
    return gpioCf3_SetOpenDrain(gpioRefPin42, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_SetTriStateOutput
(
    le_gpioPin2_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin2, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_SetTriStateOutput
(
    le_gpioPin7_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin7, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_SetTriStateOutput
(
    le_gpioPin8_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin8, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_SetTriStateOutput
(
    le_gpioPin13_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin13, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_SetTriStateOutput
(
    le_gpioPin21_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin21, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_SetTriStateOutput
(
    le_gpioPin22_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin22, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_SetTriStateOutput
(
    le_gpioPin23_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin23, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_SetTriStateOutput
(
    le_gpioPin24_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin24, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_SetTriStateOutput
(
    le_gpioPin25_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin25, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_SetTriStateOutput
(
    le_gpioPin32_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin32, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_SetTriStateOutput
(
    le_gpioPin33_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin33, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_SetTriStateOutput
(
    le_gpioPin42_Polarity_t polarity
        ///< [IN]
        ///< Active-high or active-low.
)
{
    return gpioCf3_SetTriState(gpioRefPin42, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin2, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin7, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin8, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin13, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin21, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin22, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin23, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin24, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin25, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin32, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin33, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-up resistor (disables pull-down if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_EnablePullUp
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin42, CF3_PULLUPDOWN_TYPE_UP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin2, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin7, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin8, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin13, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin21, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin22, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin23, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin24, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin25, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin32, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin33, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable the pull-down resistor (disables pull-up if previously enabled).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_EnablePullDown
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin42, CF3_PULLUPDOWN_TYPE_DOWN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin2, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin7, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin8, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin13, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin21, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin22, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin23, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin24, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin25, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin32, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin33, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the pull-up and pull-down resistors.  Does nothing if both are already disabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_DisableResistors
(
    void
)
{
    return gpioCf3_SetPullUpDown(gpioRefPin42, CF3_PULLUPDOWN_TYPE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin7);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin13);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin21);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin22);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin23);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin24);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin25);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin32);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin33);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_Activate
(
    void
)
{
    return gpioCf3_Activate(gpioRefPin42);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin7);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin13);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin21);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin22);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin23);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin24);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin25);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin32);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin33);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to inactive state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_Deactivate
(
    void
)
{
    return gpioCf3_Deactivate(gpioRefPin42);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin2_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin7_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin7);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin8_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin13_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin13);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin21_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin21);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin22_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin22);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin23_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin23);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin24_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin24);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin25_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin25);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin32_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin32);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin33_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin33);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read value of GPIO input pin.
 *
 * @return true = active, false = inactive.
 *
 * @note It is invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioPin42_Read
(
    void
)
{
    return gpioCf3_ReadInput(gpioRefPin42);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin2_ChangeEventHandlerRef_t le_gpioPin2_AddChangeEventHandler
(
    le_gpioPin2_Edge_t trigger,
    le_gpioPin2_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin2_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin2, gpioPin2_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin7_ChangeEventHandlerRef_t le_gpioPin7_AddChangeEventHandler
(
    le_gpioPin7_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin7_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin7_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin7, gpioPin7_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin8_ChangeEventHandlerRef_t le_gpioPin8_AddChangeEventHandler
(
    le_gpioPin8_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin8_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin8_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin8, gpioPin8_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin13_ChangeEventHandlerRef_t le_gpioPin13_AddChangeEventHandler
(
    le_gpioPin13_Edge_t trigger,
    le_gpioPin13_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin13_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                              gpioRefPin13, gpioPin13_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin21_ChangeEventHandlerRef_t le_gpioPin21_AddChangeEventHandler
(
    le_gpioPin21_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin21_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin21_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                               gpioRefPin21, gpioPin21_InputMonitorHandlerFunc, trigger,
                               handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin22_ChangeEventHandlerRef_t le_gpioPin22_AddChangeEventHandler
(
    le_gpioPin22_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin22_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin22_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin22, gpioPin22_InputMonitorHandlerFunc,
                                     trigger, handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin23_ChangeEventHandlerRef_t le_gpioPin23_AddChangeEventHandler
(
    le_gpioPin23_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin23_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin23_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin23, gpioPin23_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin24_ChangeEventHandlerRef_t le_gpioPin24_AddChangeEventHandler
(
    le_gpioPin24_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin24_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin24_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin24, gpioPin24_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin25_ChangeEventHandlerRef_t le_gpioPin25_AddChangeEventHandler
(
    le_gpioPin25_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin25_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin25_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin25, gpioPin25_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin32_ChangeEventHandlerRef_t le_gpioPin32_AddChangeEventHandler
(
    le_gpioPin32_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin32_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin32_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin32, gpioPin32_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin33_ChangeEventHandlerRef_t le_gpioPin33_AddChangeEventHandler
(
    le_gpioPin33_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin33_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin33_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin33, gpioPin33_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called when an input pin changes state.
 *
 * If the pin is not capable of interrupt-driven operation, then it will be sampled every
 * sampleMs milliseconds.  Otherwise, sampleMs will be ignored.
 */
//--------------------------------------------------------------------------------------------------
le_gpioPin42_ChangeEventHandlerRef_t le_gpioPin42_AddChangeEventHandler
(
    le_gpioPin42_Edge_t trigger,
        ///< [IN]
        ///< Change(s) that should trigger the callback to be called.

    le_gpioPin42_ChangeCallbackFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    int32_t sampleMs
        ///< [IN]
        ///< If not interrupt capable, sample the input this often (ms).
)
{
    return (le_gpioPin42_ChangeEventHandlerRef_t)gpioCf3_SetChangeCallback(
                                     gpioRefPin42, gpioPin42_InputMonitorHandlerFunc, trigger,
                                     handlerPtr, contextPtr, sampleMs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin2_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin7_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin7);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin8_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin13_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin13);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin21_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin21);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin22_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin22);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin23_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin23);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin24_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin24);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin25_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin25);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin32_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin32);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin33_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin33);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioPin42_SetHighZ
(
    void
)
{
    return gpioCf3_SetHighZ(gpioRefPin42);
}

/**
 * Boilerplate for RemoveChangeEventHandler
 */
void le_gpioPin2_RemoveChangeEventHandler(le_gpioPin2_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin2, addHandlerRef); }

void le_gpioPin7_RemoveChangeEventHandler(le_gpioPin7_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin7, addHandlerRef); }

void le_gpioPin8_RemoveChangeEventHandler(le_gpioPin8_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin8, addHandlerRef); }

void le_gpioPin13_RemoveChangeEventHandler(le_gpioPin13_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin13, addHandlerRef); }

void le_gpioPin21_RemoveChangeEventHandler(le_gpioPin21_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin21, addHandlerRef); }

void le_gpioPin22_RemoveChangeEventHandler(le_gpioPin22_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin22, addHandlerRef); }

void le_gpioPin23_RemoveChangeEventHandler(le_gpioPin23_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin23, addHandlerRef); }

void le_gpioPin24_RemoveChangeEventHandler(le_gpioPin24_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin24, addHandlerRef); }

void le_gpioPin25_RemoveChangeEventHandler(le_gpioPin25_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin25, addHandlerRef); }

void le_gpioPin32_RemoveChangeEventHandler(le_gpioPin32_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin32, addHandlerRef); }

void le_gpioPin33_RemoveChangeEventHandler(le_gpioPin33_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin33, addHandlerRef); }

void le_gpioPin42_RemoveChangeEventHandler(le_gpioPin42_ChangeEventHandlerRef_t addHandlerRef)
{ gpioCf3_RemoveChangeCallback(gpioRefPin42, addHandlerRef); }


//--------------------------------------------------------------------------------------------------
/**
 * Function will be called when the client-server session opens. This allows the relationship
 * between the session and the GPIO reference to be created
 */
//--------------------------------------------------------------------------------------------------
static void SessionOpenHandlerFunc
(
    le_msg_SessionRef_t  sessionRef,  ///<[IN] Client session reference.
    void*                contextPtr   ///<[IN] Client context pointer.
)
{
    gpioCf3_GpioRef_t gpioRefPtr = (gpioCf3_GpioRef_t)contextPtr;

    if (NULL == gpioRefPtr)
    {
        LE_KILL_CLIENT("Unable to match context to pin");
    }

    uint8_t pin = gpioRefPtr->pinNum;

    if (pin <= 0 || pin >= MAX_GPIO_PIN_NUMBER)
    {
        LE_KILL_CLIENT("Supplied bad (%d) GPIO Pin number", pin);
    }

    if (gpioCf3PinMap[pin] == 0)
    {
        LE_KILL_CLIENT("Unsupported GPIO Pin(%d) mapping", pin);
    }

    if (gpioRefPtr == NULL)
    {
        LE_KILL_CLIENT("Unable to look up GPIO");
    }

    // Make sure the GPIO is not already in use
    if (gpioRefPtr->inUse)
    {
        uid_t user = 0;
        pid_t pid = 0;
        le_msg_GetClientUserCreds(sessionRef, &user, &pid);

        LE_WARN("Attempt to use a GPIO that is already in use by uid %d with pid %d",
                user,
                pid
                );

        le_msg_CloseSession(sessionRef);
        return;
    }

    snprintf(gpioRefPtr->gpioName, sizeof(gpioRefPtr->gpioName), "gpio%d", gpioCf3PinMap[pin]);

    // Export the pin in sysfs to make it available for use
    if (LE_OK != ExportGpio(gpioRefPtr))
    {
        LE_KILL_CLIENT("Unable to export GPIO for use");
    }

    // Mark the PIN as in use
    LE_INFO("Assigning GPIO %d", pin);
    gpioRefPtr->inUse = true;

    LE_DEBUG("gpio pin:%d, GPIO Name:%s", gpioRefPtr->pinNum, gpioRefPtr->gpioName);
    return;

}

//--------------------------------------------------------------------------------------------------
/**
 * Function will be called when the client-server session closes.
 */
//--------------------------------------------------------------------------------------------------
static void SessionCloseHandlerFunc
(
    le_msg_SessionRef_t  sessionRef,  ///<[IN] Client session reference.
    void*                contextPtr   ///<[IN] Client context pointer.
)
{
    gpioCf3_GpioRef_t gpioRefPtr = (gpioCf3_GpioRef_t)contextPtr;

    if (gpioRefPtr == NULL)
    {
        LE_WARN("Unable to look up GPIO PIN for closing session");
    }
    else
    {
        // Mark the pin as not in use
        LE_INFO("Releasing GPIO %d", gpioRefPtr->pinNum);
        gpioRefPtr->inUse = false;

        // If there is an fd monitor then stop it
        if (gpioRefPtr->fdMonitor != NULL)
        {
            le_fdMonitor_Delete(gpioRefPtr->fdMonitor);
            gpioRefPtr->fdMonitor = NULL;
        }

        // If there is a callback registered then forget it
        gpioRefPtr->callbackContextPtr = NULL;
        gpioRefPtr->handlerPtr = NULL;
    }
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * The place where the component starts up.  All initialization happens here.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int i;

    for (i = 0; i < 256; i++)
    {
        gpioCf3PinMap[i]  = 0;
    }

#ifdef CF3_GPIO_PIN_MAPPING
    gpioCf3PinMap[10]  = 2;
    gpioCf3PinMap[40]  = 7;
    gpioCf3PinMap[41]  = 8;
    gpioCf3PinMap[44]  = 13;
    gpioCf3PinMap[104] = 32;
    gpioCf3PinMap[105] = 33;
    gpioCf3PinMap[109] = 42;
    gpioCf3PinMap[147] = 21;
    gpioCf3PinMap[148] = 22;
    gpioCf3PinMap[149] = 23;
    gpioCf3PinMap[150] = 24;
    gpioCf3PinMap[159] = 25;
#else
    gpioCf3PinMap[2]  = 2;
    gpioCf3PinMap[7]  = 7;
    gpioCf3PinMap[8]  = 8;
    gpioCf3PinMap[13] = 13;
    gpioCf3PinMap[32] = 32;
    gpioCf3PinMap[33] = 33;
    gpioCf3PinMap[42] = 42;
    gpioCf3PinMap[21] = 21;
    gpioCf3PinMap[22] = 22;
    gpioCf3PinMap[23] = 23;
    gpioCf3PinMap[24] = 24;
    gpioCf3PinMap[25] = 25;
#endif

    // Create my service: gpio pin2.
    le_msg_AddServiceOpenHandler(le_gpioPin2_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin2);
    le_msg_AddServiceCloseHandler(le_gpioPin2_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin2);

    // Create my service: gpio pin7.
    le_msg_AddServiceOpenHandler(le_gpioPin7_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin7);
    le_msg_AddServiceCloseHandler(le_gpioPin7_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin7);

    // Create my service: gpio pin8.
    le_msg_AddServiceOpenHandler(le_gpioPin8_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin8);
    le_msg_AddServiceCloseHandler(le_gpioPin8_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin8);

    // Create my service: gpio pin13.
    le_msg_AddServiceOpenHandler(le_gpioPin13_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin13);
    le_msg_AddServiceCloseHandler(le_gpioPin13_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin13);

    // Create my service: gpio pin21.
    le_msg_AddServiceOpenHandler(le_gpioPin21_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin21);
    le_msg_AddServiceCloseHandler(le_gpioPin21_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin21);

    // Create my service: gpio pin22.
    le_msg_AddServiceOpenHandler(le_gpioPin22_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin22);
    le_msg_AddServiceCloseHandler(le_gpioPin22_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin22);

    // Create my service: gpio pin23.
    le_msg_AddServiceOpenHandler(le_gpioPin23_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin23);
    le_msg_AddServiceCloseHandler(le_gpioPin23_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin23);

    // Create my service: gpio pin24.
    le_msg_AddServiceOpenHandler(le_gpioPin24_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin24);
    le_msg_AddServiceCloseHandler(le_gpioPin24_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin24);

    // Create my service: gpio pin25.
    le_msg_AddServiceOpenHandler(le_gpioPin25_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin25);
    le_msg_AddServiceCloseHandler(le_gpioPin25_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin25);

    // Create my service: gpio pin32.
    le_msg_AddServiceOpenHandler(le_gpioPin32_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin32);
    le_msg_AddServiceCloseHandler(le_gpioPin32_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin32);

    // Create my service: gpio pin33.
    le_msg_AddServiceOpenHandler(le_gpioPin33_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin33);
    le_msg_AddServiceCloseHandler(le_gpioPin33_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin33);

    // Create my service: gpio pin42.
    le_msg_AddServiceOpenHandler(le_gpioPin42_GetServiceRef(),
                                 SessionOpenHandlerFunc,
                                 (void*)gpioRefPin42);
    le_msg_AddServiceCloseHandler(le_gpioPin42_GetServiceRef(),
                                  SessionCloseHandlerFunc,
                                  (void*)gpioRefPin42);
}

