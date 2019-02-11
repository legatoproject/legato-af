/**
 * @file gpioSysfsUtils.c
 *
 * Utility functions for working with the GPIO sysfs in Linux. Some features
 * of the generic Legato GPIO API are not available and hence not implemented.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "gpioSysfs.h"

//--------------------------------------------------------------------------------------------------
/**
 * GPIO signals have paths like /sys/class/gpio/gpio42/ (for GPIO #42) in Legacy GPIO design
 * and paths like /sys/class/gpio/v2/alias_exported/42/ (for GPIO #42) in GPIO design v2
 */
//--------------------------------------------------------------------------------------------------
#define SYSFS_GPIO_PATH           "/sys/class/gpio"
#define SYSFS_GPIO_ALIAS_PREFIX   "/v2/alias_"
#define SYSFS_GPIO_ALIASES_PATH   "/v2/aliases_exported/"

//--------------------------------------------------------------------------------------------------
/**
 * Max and Min Pin Numbers
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PIN_NUMBER 64
#define MIN_PIN_NUMBER 1

//--------------------------------------------------------------------------------------------------
/**
 * Static absolute path to export, unexport and GPIO pin entries
 */
//--------------------------------------------------------------------------------------------------
static char GpioAliasPrefix[sizeof(SYSFS_GPIO_ALIAS_PREFIX) + 1];
static char GpioAliasesPath[sizeof(SYSFS_GPIO_ALIASES_PATH) + 1];

//--------------------------------------------------------------------------------------------------
/**
 * Check for new GPIO design or legacy
 */
//--------------------------------------------------------------------------------------------------
static gpioSysfs_Design_t GpioDesign = SYSFS_GPIO_DESIGN_V1;

//--------------------------------------------------------------------------------------------------
/**
 * Remove the change callback for the given GPIO
 */
//--------------------------------------------------------------------------------------------------
static void RemoveChangeCallback
(
    gpioSysfs_GpioRef_t gpioRef  ///< GPIO to remove the change callback from
)
{
    // If there is an fd monitor then stop it
    if (gpioRef->fdMonitor != NULL)
    {
        LE_DEBUG("Stopping fd monitor");
        const int fd = le_fdMonitor_GetFd(gpioRef->fdMonitor);
        le_fdMonitor_Delete(gpioRef->fdMonitor);
        gpioRef->fdMonitor = NULL;
        const int ret = close(fd);
        LE_WARN_IF(ret == -1, "Failed to close file descriptor for gpio %d: %m", gpioRef->pinNum);
    }

    LE_DEBUG("Removing callback references");
    // If there is a callback registered then forget it
    gpioRef->callbackContextPtr = NULL;
    gpioRef->handlerPtr = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if sysfs gpio path exists.
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
 * Export a GPIO in the sysfs.
 * @return
 * - LE_OK if exporting was successful
 * - LE_IO_ERROR if it failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExportGpio
(
    const gpioSysfs_GpioRef_t gpioRef
)
{
    char path[128];
    char export[128];
    char gpioStr[8];
    FILE *fp = NULL;

    // First check if the GPIO has already been exported
    snprintf(path, sizeof(path), "%s/%s%s", SYSFS_GPIO_PATH, GpioAliasesPath, gpioRef->gpioName);
    if (CheckGpioPathExist(path))
    {
        return LE_OK;
    }

    // Write the GPIO number to the export file
    snprintf(export, sizeof(export), "%s/%s%s", SYSFS_GPIO_PATH, GpioAliasPrefix, "export");
    snprintf(gpioStr, sizeof(gpioStr), "%d", gpioRef->pinNum);
    do
    {
        fp = fopen(export, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if(!fp) {
        LE_WARN("Error opening file %s for writing.\n", export);
        return LE_IO_ERROR;
    }

    ssize_t written = fwrite(gpioStr, 1, strlen(gpioStr), fp);
    fflush(fp);

    int fileError = ferror(fp);
    fclose(fp);
    if (fileError != 0)
    {
        LE_WARN("Failed to export GPIO %s. Error %s", gpioStr, strerror(fileError));
        return LE_IO_ERROR;
    }

    if (written < strlen(gpioStr))
    {
        LE_WARN("Data truncated while exporting GPIO %s.", gpioStr);
        return LE_IO_ERROR;
    }

    // Now check again that it has been exported
    if (CheckGpioPathExist(path))
    {
        return LE_OK;
    }
    LE_WARN("Failed to export GPIO %s.", gpioStr);
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
 * - LE_IO_ERROR if there was an error while writing the sysfs entry
 * - LE_BAD_PARAMETER if the path doesn't exist
 * - LE_OK on success
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
        LE_ERROR("GPIO %s does not exist (probably not exported)", path);
        return LE_BAD_PARAMETER;
    }

    do
    {
        fp = fopen(path, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if (!fp)
    {
        LE_ERROR("Error opening file %s for writing.\n", path);
        return LE_IO_ERROR;
    }

    ssize_t written = fwrite(attr, 1, strlen(attr), fp);
    fflush(fp);

    int fileError = ferror(fp);
    if (fileError != 0)
    {
        LE_EMERG("Failed to write %s to GPIO config %s. Error %s", attr, path, strerror(fileError));
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
 * - LE_IO_ERROR if there was an error while reading the sysfs entry
 * - LE_BAD_PARAMETER if the path doesn't exist
 * - LE_OK on success
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadSysGpioSignalAttr
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
        LE_ERROR("File %s does not exist", path);
        return LE_BAD_PARAMETER;
    }

    do
    {
        fp = fopen(path, "r");
    }
    while ((fp == NULL) && (errno == EINTR));

    if (!fp)
    {
        LE_ERROR("Error opening file %s for reading.\n", path);
        return LE_IO_ERROR;
    }

    i = 0;
    while (((c = fgetc(fp)) != EOF) && (i < (attr_size - 1)))
    {
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
 * write value to GPIO output, low or high
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteOutputValue
(
    gpioSysfs_GpioRef_t gpioRef,              ///< [IN] GPIO object reference
    gpioSysfs_Value_t level                   ///< [IN] High or low
)
{
    char path[64];
    char attr[16];

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_ERROR("gpioRef is NULL or gpio not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
                          gpioRef->gpioName, "value");
    snprintf(attr, sizeof(attr), "%d", level);
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
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
static le_result_t SetEdgeSense
(
    gpioSysfs_GpioRef_t gpioRef,              ///< [IN] GPIO object reference
    gpioSysfs_EdgeSensivityMode_t edge        ///< [IN] The mode of GPIO Edge Sensivity.
)
{
    char path[64];
    const char *attr;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_ERROR("gpioRef is NULL or object not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "edge");

    switch(edge)
    {
        case SYSFS_EDGE_SENSE_RISING:
            attr = "rising";
            break;
        case SYSFS_EDGE_SENSE_FALLING:
            attr = "falling";
            break;
        case SYSFS_EDGE_SENSE_BOTH:
            attr = "both";
            break;
        default:
            attr = "none";
            break;
    }
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
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
static le_result_t SetDirection
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO object reference
    gpioSysfs_PinMode_t mode           ///< [IN] gpio direction input/output mode
)
{
    char path[64];
    const char *attr;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_ERROR("gpioRef is NULL or object not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
            gpioRef->gpioName, "direction");
    attr = (mode == SYSFS_PIN_MODE_OUTPUT) ? "out": "in";
    LE_DEBUG("path:%s, attribute:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}


//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO pullup or pulldown disable/enable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetPullUpDown
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO object reference
    gpioSysfs_PullUpDownType_t pud     ///< [IN] pull up, pull down type
)
{
    char path[64];
    const char *attr;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_ERROR("gpioRef is NULL or object not initialized");
        return LE_BAD_PARAMETER;
    }

    // It is not possible to disable the resistors
    if (pud == SYSFS_PULLUPDOWN_TYPE_OFF)
    {
        LE_ERROR("Disabling the resistors is not supported");
        return LE_NOT_IMPLEMENTED;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "pull");
    attr = (pud == SYSFS_PULLUPDOWN_TYPE_DOWN) ? "down": "up";
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set up PushPull Output.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetPushPullOutput
(
    gpioSysfs_GpioRef_t gpioRef,
    gpioSysfs_ActiveType_t polarity,
    bool value
)
{
    le_result_t res = LE_OK;

    res = SetDirection(gpioRef, SYSFS_PIN_MODE_OUTPUT);
    if (LE_OK != res)
    {
        LE_DEBUG("Unable to set GPIO %s as output", gpioRef->gpioName);
        return res;
    }

    res = gpioSysfs_SetPolarity(gpioRef, polarity);
    if (LE_OK != res)
    {
        LE_DEBUG("Unable to set GPIO %s polarity", gpioRef->gpioName);
        return res;
    }

    return WriteOutputValue(gpioRef, value ? SYSFS_VALUE_HIGH : SYSFS_VALUE_LOW);

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
le_result_t gpioSysfs_SetOpenDrain
(
    gpioSysfs_GpioRef_t gpioRef,          ///< [IN] GPIO object reference
    gpioSysfs_ActiveType_t polarity,      ///< [IN] Active-high or active-low
    bool value                            ///< [IN] Initial value to drive
)
{
    LE_WARN("Open Drain API not implemented in sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as a tri-state output pin.
 *
 * @note The initial state will be high-impedance.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetTriState
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity    ///< [IN] Active-high or active-low
)
{
    LE_WARN("Tri-State API not implemented in sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the pin as an input pin.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetInput
(
    gpioSysfs_GpioRef_t gpioRef,         ///< [IN] GPIO module object reference
    gpioSysfs_ActiveType_t polarity      ///< [IN] Active-high or active-low.
)
{
    le_result_t res = LE_OK;

    res = SetDirection(gpioRef, SYSFS_PIN_MODE_INPUT);

    if (LE_OK != res)
    {
        LE_DEBUG("Unable to set GPIO %s as input", gpioRef->gpioName);
        return res;
    }

    return gpioSysfs_SetPolarity(gpioRef, polarity);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set output pin to high impedance state.
 *
 * @warning Only valid for tri-state or open-drain output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetHighZ
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO module object reference
)
{
    LE_WARN("SetHighZ API not implemented in sysfs GPIO");
    return LE_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO polarity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetPolarity
(
    gpioSysfs_GpioRef_t gpioRef,            ///< [IN] GPIO object reference
    gpioSysfs_ActiveType_t level            ///< [IN] Active-high or active-low
)
{
    char path[64];
    char attr[16];

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_ERROR("gpioRef is NULL or gpio not initialized");
        return LE_BAD_PARAMETER;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "active_low");
    snprintf(attr, sizeof(attr), "%d", level);
    LE_DEBUG("path:%s, attr:%s", path, attr);

    return WriteSysGpioSignalAttr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a change callback on a particular pin
 *
 * @return This will return a reference
 */
//--------------------------------------------------------------------------------------------------
void* gpioSysfs_SetChangeCallback
(
    gpioSysfs_GpioRef_t gpioRef,                  ///< [IN] GPIO object reference
    le_fdMonitor_HandlerFunc_t fdMonFunc,         ///< [IN] The fd monitor function
    gpioSysfs_EdgeSensivityMode_t edge,           ///< [IN] Edge detection mode.
    gpioSysfs_ChangeCallbackFunc_t handlerPtr,    ///< [IN]
    void* contextPtr,                             ///< [IN]
    int32_t sampleMs                              ///< [IN] If not interrupt capable, sample this often.
)
{
    char monFile[128];
    int monFd = -1;
    le_result_t leResult;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_KILL_CLIENT("gpioRef is NULL or object not initialized");
        return NULL;
    }

    // Only one handler is allowed here
    if (gpioRef->fdMonitor != NULL)
    {
        LE_KILL_CLIENT("Only one change handler can be registered");
        return NULL;
    }

    leResult = SetEdgeSense(gpioRef, edge);
    // Set the edge detection mode
    if (leResult != LE_OK)
    {
        if (leResult == LE_BAD_PARAMETER)
        {
            LE_ERROR("Path doesn't exist to set edge detection");
        }
        else
        {
            LE_KILL_CLIENT("Unable to set edge detection correctly");
            return NULL;
        }
    }

    // Store the callback function and context pointer
    gpioRef->handlerPtr = handlerPtr;
    gpioRef->callbackContextPtr = contextPtr;

    // Start monitoring the fd for the correct GPIO
    snprintf(monFile, sizeof(monFile), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "value");

    do
    {
        monFd = open(monFile, O_RDONLY);
    }
    while ((monFd < 0) && (errno == EINTR));

    if (monFd < 0)
    {
        LE_ERROR("Unable to open GPIO file for monitoring");
        return NULL;
    }

    // Seek to the start of the file and read from it - this is required to prevent
    // false triggers- see https://www.kernel.org/doc/Documentation/gpio/sysfs.txt
    LE_DEBUG("Seek to start of file %d", monFd);

    LE_ERROR_IF(lseek(monFd, 0, SEEK_SET) == (off_t)(-1),
                "Failed to SEEK_SET for GPIO '%s'. %m.", gpioRef->gpioName );

    //We will read a single character
    char buf[1];

    if (read(monFd, buf, 1) != 1)
    {
        LE_ERROR("Unable to read value for GPIO %s. %m", gpioRef->gpioName);
    }

    LE_DEBUG("Setting up file monitor for fd %d and pin %s", monFd, gpioRef->gpioName);
    gpioRef->fdMonitor = le_fdMonitor_Create (gpioRef->gpioName, monFd, fdMonFunc, POLLPRI);

    return gpioRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a change callback on a particular pin
 *
 * @return Returns nothing.
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_RemoveChangeCallback
(
    gpioSysfs_GpioRef_t gpioRef,       ///< [IN] GPIO object reference
    void * addHandlerRef               ///< [IN] The reference from when the handler was added
)
{
    if (addHandlerRef != gpioRef)
    {
        LE_KILL_CLIENT("Invalid GPIO reference provided");
    }
    else
    {
        RemoveChangeCallback(gpioRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Turn off edge detection. This function does not require a handler to be
 * registered as it disables interrupts.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_DisableEdgeSense
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
)
{
    return SetEdgeSense(gpioRef, SYSFS_EDGE_SENSE_NONE);
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
gpioSysfs_Value_t gpioSysfs_ReadValue
(
    gpioSysfs_GpioRef_t gpioRef            ///< [IN] GPIO object reference
)
{
    char path[64];
    char result[17];
    le_result_t leResult;
    gpioSysfs_Value_t type;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_KILL_CLIENT("gpioRef is NULL or object not initialized");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "value");
    leResult = ReadSysGpioSignalAttr(path, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return -1;
    }

    type = atoi(result);
    LE_DEBUG("result:%s Value:%s", result, (type==1) ? "high": "low");

    return type;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set an output pin to active state.
 *
 * @warning Only valid for output pins.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_Activate
(
    gpioSysfs_GpioRef_t gpioRef
)
{
    if (LE_OK != SetDirection(gpioRef, SYSFS_PIN_MODE_OUTPUT))
    {
        LE_ERROR("Failed to set Direction on GPIO %s", gpioRef->gpioName);
        return LE_IO_ERROR;
    }

    if (LE_OK != WriteOutputValue(gpioRef, SYSFS_VALUE_HIGH))
    {
        LE_ERROR("Failed to set GPIO %s to high", gpioRef->gpioName);
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
le_result_t gpioSysfs_Deactivate
(
    gpioSysfs_GpioRef_t gpioRef
)
{
    if (LE_OK != SetDirection(gpioRef, SYSFS_PIN_MODE_OUTPUT))
    {
        LE_ERROR("Failed to set Direction on GPIO %s", gpioRef->gpioName);
        return LE_IO_ERROR;
    }

    if (LE_OK != WriteOutputValue(gpioRef, SYSFS_VALUE_LOW))
    {
        LE_ERROR("Failed to set GPIO %s to low", gpioRef->gpioName);
        return LE_IO_ERROR;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the pin is currently active. Returns true is a read of "value" returns 1
 *
 * @return true = active, false = inactive.
 *
 * @note this function can only be used on output pins
 */
//--------------------------------------------------------------------------------------------------
bool gpioSysfs_IsActive
(
    gpioSysfs_GpioRef_t gpioRef         ///< [IN] GPIO object reference
)
{
    if (gpioSysfs_IsInput(gpioRef))
    {
        LE_WARN("Attempt to check if an input is active");
        return false;
    }

    return (gpioSysfs_ReadValue(gpioRef) == SYSFS_VALUE_HIGH);
}

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
)
{
    char path[64];
    char result[9];
    le_result_t leResult;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_KILL_CLIENT("gpioRef is NULL or object not initialized");
        return false;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "direction");
    leResult = ReadSysGpioSignalAttr(path, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return -1;
    }

    LE_DEBUG("Read direction - result: %s", result);

    return (strncmp(result, "in", 2) == 0);
}

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
)
{
    return (!gpioSysfs_IsInput(gpioRef));
}

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
)
{
    char path[64];
    char result[9];
    le_result_t leResult;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_KILL_CLIENT("gpioRef is NULL or object not initialized");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "pull");
    leResult = ReadSysGpioSignalAttr(path, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return -1;
    }

    LE_DEBUG("Read pull up/down - result: %s", result);

    gpioSysfs_PullUpDownType_t pud = SYSFS_PULLUPDOWN_TYPE_OFF;

    if (strncmp(result, "down", 4) == 0)
    {
        LE_DEBUG("Detected pull up/down as down");
        pud = SYSFS_PULLUPDOWN_TYPE_DOWN;
    }
    else if (strncmp(result, "up", 2) == 0)
    {
        LE_DEBUG("Detected pull up/down as up");
        pud = SYSFS_PULLUPDOWN_TYPE_UP;
    }

    return pud;
}

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
)
{
    char path[64];
    char result[17];
    le_result_t leResult;
    gpioSysfs_ActiveType_t type;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_KILL_CLIENT("gpioRef is NULL or object not initialized");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "active_low");
    leResult = ReadSysGpioSignalAttr(path, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return -1;
    }

    type = atoi(result);
    LE_DEBUG("result: %s", result);

    if (type == 0)
    {
        return SYSFS_ACTIVE_TYPE_HIGH;
    }

    return SYSFS_ACTIVE_TYPE_LOW;
}

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
)
{
    char path[64];
    char result[9];
    le_result_t leResult;

    if ((!gpioRef) || (gpioRef->pinNum == 0))
    {
        LE_KILL_CLIENT("gpioRef is NULL or object not initialized");
        return -1;
    }

    if (gpioSysfs_IsOutput(gpioRef))
    {
        LE_WARN("Attempt to read edge sense on an output");
        return SYSFS_EDGE_SENSE_NONE;
    }

    snprintf(path, sizeof(path), "%s/%s%s/%s", SYSFS_GPIO_PATH, GpioAliasesPath,
             gpioRef->gpioName, "edge");
    leResult = ReadSysGpioSignalAttr(path, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return -1;
    }

    LE_DEBUG("Read edge - result: %s", result);

    gpioSysfs_EdgeSensivityMode_t edge = SYSFS_EDGE_SENSE_NONE;

    if (strncmp(result, "rising", 6) == 0)
    {
        LE_DEBUG("Detected edge as rising");
        edge = SYSFS_EDGE_SENSE_RISING;
    }
    else if (strncmp(result, "falling", 7) == 0)
    {
        LE_DEBUG("Detected edge as rising");
        edge = SYSFS_EDGE_SENSE_FALLING;
    }
    else if (strncmp(result, "both", 4) == 0)
    {
        LE_DEBUG("Detected edge as both");
        edge = SYSFS_EDGE_SENSE_BOTH;
    }

    return edge;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the edge sense value. In order to call this there must be a callback registered for
 * interrupts. Otherwise this would just generate interrupts without them being handled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gpioSysfs_SetEdgeSense
(
    gpioSysfs_GpioRef_t gpioRef,              ///< [IN] GPIO object reference
    gpioSysfs_EdgeSensivityMode_t edge        ///< [IN] The mode of GPIO Edge Sensivity.
)
{
    if (gpioRef->handlerPtr == NULL)
    {
        LE_ERROR("Attempt to change edge sense value without a registered handler");
        return LE_FAULT;
    }

    return SetEdgeSense(gpioRef, edge);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function will be called when there is a state change on a GPIO
 *
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_InputMonitorHandlerFunc
(
    const gpioSysfs_GpioRef_t gpioRef,
    int fd,
    short events
)
{
    //We're reading a single character
    char buf[1];

    LE_DEBUG("Input handler called for %s", gpioRef->gpioName);

    // Make sure the pin is in use and this isn't a spurious interrupt
    if (!gpioRef->inUse)
    {
        LE_WARN("Spurious interrupt handled - ignoring");
        return;
    }

    // Seek to the start of the file - this is required to prevent
    // repeated triggers - see https://www.kernel.org/doc/Documentation/gpio/sysfs.txt
    LE_DEBUG("Seek to start of file %d", fd);
    lseek(fd, 0, SEEK_SET);

    if (read(fd, buf, 1) != 1)
    {
        LE_ERROR("Unable to read value for GPIO %s", gpioRef->gpioName);
        return;
    }

    LE_DEBUG("Read value %c from value file for callback", buf[0]);

    // Look up the callback function
    gpioSysfs_ChangeCallbackFunc_t handlerPtr = (gpioSysfs_ChangeCallbackFunc_t)gpioRef->handlerPtr;

    if (handlerPtr != NULL)
    {
        LE_DEBUG("Calling change callback for %s", gpioRef->gpioName);
        handlerPtr(buf[0]=='1', gpioRef->callbackContextPtr);
    }
    else
    {
        LE_WARN("No callback registered for pin %s", gpioRef->gpioName);
    }

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function will be called when the client-server session opens. This allows the relationship
 * between the session and the GPIO object reference to be created.
 * A service using this module to interact with the sysfs should register this function
 * with the low-level messaging API using le_msg_AddServiceOpenHandler.
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_SessionOpenHandlerFunc
(
    le_msg_SessionRef_t  sessionRef,  ///<[IN] Client session reference.
    void*                contextPtr   ///<[IN] Client context pointer.
)
{
    gpioSysfs_GpioRef_t gpioRef = (gpioSysfs_GpioRef_t)contextPtr;

    if (NULL == gpioRef)
    {
        LE_KILL_CLIENT("Unable to match context to pin");
        return;
    }

    if (gpioRef == NULL)
    {
        LE_KILL_CLIENT("Unable to look up GPIO");
        return;
    }

    // Make sure the GPIO is not already in use
    if (gpioRef->inUse)
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

    // Export the pin in sysfs to make it available for use
    if (LE_OK != ExportGpio(gpioRef))
    {
        LE_WARN("Unable to export GPIO %s for use - stopping session", gpioRef->gpioName);
        le_msg_CloseSession(sessionRef);

        return;
    }

    // Mark the PIN as in use
    LE_INFO("Assigning GPIO %d", gpioRef->pinNum);
    gpioRef->inUse = true;

    // Store the current, valid session ref
    gpioRef->currentSession = sessionRef;

    LE_DEBUG("gpio pin:%d, GPIO Name:%s", gpioRef->pinNum, gpioRef->gpioName);
    return;

}

//--------------------------------------------------------------------------------------------------
/**
 * Function will be called when the client-server session closes.
 */
//--------------------------------------------------------------------------------------------------
void gpioSysfs_SessionCloseHandlerFunc
(
    le_msg_SessionRef_t  sessionRef,  ///<[IN] Client session reference.
    void*                contextPtr   ///<[IN] Client context pointer.
)
{
    gpioSysfs_GpioRef_t gpioRef = (gpioSysfs_GpioRef_t)contextPtr;

    if (gpioRef == NULL)
    {
        LE_WARN("Unable to look up GPIO PIN for closing session");
        return;
    }

    // Make sure this is the valid session. If we have rejected a connection
    // then no clean up should be done as this will mess up the real session
    if (gpioRef->currentSession != sessionRef)
    {
        LE_DEBUG("No clean up required. This is a rejected session");
        return;
    }

    // Mark the pin as not in use
    LE_INFO("Releasing GPIO %d", gpioRef->pinNum);
    gpioRef->inUse = false;

    RemoveChangeCallback(gpioRef);

    gpioRef->currentSession = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Determine if a GPIO PIn is available for use. This is done by reading the value of
 * /sys/class/gpio/gpiochip1/mask (on our platforms). The sysfs doc describes this as follows...
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
    int pinNum         ///< [IN] GPIO object reference
)
{
    char path[64];
    char result[33];
    le_result_t leResult;
    uint8_t check;
    uint32_t index;
    uint32_t bitInMask;

    if ((pinNum < MIN_PIN_NUMBER) || (pinNum > MAX_PIN_NUMBER))
    {
        LE_WARN("Pin number %d is out of range", pinNum);
        return false;
    }

    snprintf(path, sizeof(path), "%s/gpiochip1/mask%s",
             SYSFS_GPIO_PATH, (GpioDesign == SYSFS_GPIO_DESIGN_V2 ? "_v2" : ""));
    leResult = ReadSysGpioSignalAttr(path, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return false;
    }

    LE_DEBUG("Mask read as: %s", result);

    if (GpioDesign == SYSFS_GPIO_DESIGN_V2)
    {
        bitInMask = (pinNum - 1) % 8;
        index = ((pinNum - 1) / 8) * 3 + (bitInMask < 4);
        bitInMask %= 4;
    }
    else
    {
        // The mask is 64 bits long
        // The format of the string is 0xnnnnnnnnnnnnnnnn. Each "n" represents 4 pins, starting
        // with pin 1-4 on the far right. So we can calculate where to look based on the pin number
        // e.g. 1-4 = index 17, 5-8 = index 16 etc.
        index = 17 - (pinNum - 1) / 4;
        bitInMask = (pinNum - 1) % 4;
    }

    // Convert the mask to a number from a hex string
    LE_DEBUG("Mask calculated for %d as bit %d at index %d", pinNum, bitInMask, index);

    // Convert the entry in the mask from a hex character to a number
    check = toupper(result[index]) - (isdigit(result[index]) ? '0' : 'A' - 10);

    LE_DEBUG("About to compare %x and %x", check, (1 << bitInMask));
    return (check & (1 << bitInMask));
}

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
)
{
    char path[128];

    *gpioDesignPtr = GpioDesign;
    snprintf(path, sizeof(path), "%s/%s%s", SYSFS_GPIO_PATH, SYSFS_GPIO_ALIAS_PREFIX, "export");
    if (access(path, W_OK) == 0)
    {
        LE_INFO("GPIO design V2");
        snprintf(GpioAliasPrefix, sizeof(GpioAliasPrefix), "%s", SYSFS_GPIO_ALIAS_PREFIX);
        snprintf(GpioAliasesPath, sizeof(GpioAliasesPath), "%s", SYSFS_GPIO_ALIASES_PATH);
        *gpioDesignPtr = GpioDesign = SYSFS_GPIO_DESIGN_V2;
    }
}
