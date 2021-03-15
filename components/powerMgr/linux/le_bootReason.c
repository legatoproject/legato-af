//--------------------------------------------------------------------------------------------------
/**
 * @file le_pm.c
 *
 * This file contains the source code of the top level Power Management API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


///@{
//--------------------------------------------------------------------------------------------------
/**
 * Low power Management sysfs interface files
 */
//--------------------------------------------------------------------------------------------------
#define GPIO_TRIGGER_FILE       "/sys/module/swimcu_pm/boot_source/gpio%u/triggered"
#define TIMER_TRIGGER_FILE      "/sys/module/swimcu_pm/boot_source/timer/triggered"
#define ADC_TRIGGER_FILE        "/sys/module/swimcu_pm/boot_source/adc/adc%u/triggered"
///@}

#define BOOT_SOURCE_DIR         "/sys/module/swimcu_pm/boot_source"
#define GPIO_EDGE_CFG_FILE      "/sys/module/swimcu_pm/boot_source/%s/edge"
#define GPIO_PULL_CFG_FILE      "/sys/module/swimcu_pm/boot_source/%s/pull"
#define GPIO_TRIGGERED_CFG_FILE "/sys/module/swimcu_pm/boot_source/%s/triggered"
#define GPIO_STATUS_BLOCK       "\tEdge:%s\n\tPull:%s\n\tTriggered:%s\n"

#define ADC_SOURCE_DIR          "/sys/module/swimcu_pm/boot_source/adc"
#define ADC_POLL_INTERVAL_FILE  "/sys/module/swimcu_pm/boot_source/adc/interval"
#define ADC_BELOW_LEVEL_FILE    "/sys/module/swimcu_pm/boot_source/adc/%s/below"
#define ADC_ABOVE_LEVEL_FILE    "/sys/module/swimcu_pm/boot_source/adc/%s/above"
#define ADC_SELECT_FILE         "/sys/module/swimcu_pm/boot_source/adc/%s/select"
#define ADC_TRIGGER_CFG_FILE    "/sys/module/swimcu_pm/boot_source/adc/%s/triggered"
#define ADC_STATUS_BLOCK        "\tBelow:%s\n\tAbove:%s\n\tSelect:%s\n\tTriggered:%s\n"

#define TIMER_SOURCE_DIR        "/sys/module/swimcu_pm/boot_source/timer"
#define TIMER_TIMEOUT_FILE      "/sys/module/swimcu_pm/boot_source/timer/timeout"

#define SHUTDOWN_INIT_FILE     "/sys/module/swimcu_pm/boot_source/enable"
//--------------------------------------------------------------------------------------------------
/**
 * Value written to sysfs file in case of any hardware trigger.
 */
//--------------------------------------------------------------------------------------------------
#define TRIGGER_VAL   '1'


//--------------------------------------------------------------------------------------------------
// Private function definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Check whether a boot is triggered or not. Boot source trigger path is specified in parameter.
 *
 * @return
 *      - TRUE if boot source was triggered.
 *      - FALSE otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsBootSourceTriggered
(
    const char* bootSrcTrigPath  ///< [IN]  Path to the trigger file.
)
//--------------------------------------------------------------------------------------------------
{
    int fd;
    char triggerEnable[2] = "";
    int result = 0;

    fd = open(bootSrcTrigPath, O_RDONLY);

    // Failed to open boot source file. Bad boot source or firmware.
    if (fd == -1)
    {
        LE_ERROR("Unable to open file '%s' for reading (%m). Wrong boot-source or firmware ",
                  bootSrcTrigPath);
        return false;
    }

    do
    {
        result = read(fd, triggerEnable, sizeof(triggerEnable) - 1);
    }
    while ((result == -1) && (errno == EINTR));

    LE_FATAL_IF(result == -1, "Error reading from trigger file '%s' (%m).", bootSrcTrigPath);

    // Keep trying to close the fd as long as it keeps getting interrupted by signals.
    do
    {
        result = close(fd);
    }
    while ((result == -1) && (errno == EINTR));

    LE_FATAL_IF(result != 0, "Failed to close file descriptor %d. Errno = %d (%m).", fd, errno);

    return (triggerEnable[0] == TRIGGER_VAL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempt to read from the filePath with the given objPtr such as gpio number or adc number.
 *
 * @return
 *      - TRUE if successfully read the values into the buffer.
 *      - FALSE otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool ReadFile
(
    const char *objPtr,
    const char *filePathPtr,
    char *buf,
    size_t bufSize
)
{
    int fd;
    int result = 0;
    char filePath[256] = "";
    snprintf(filePath, sizeof(filePath), filePathPtr, objPtr);

    fd = open(filePath, O_RDONLY);

    // Failed to file. Bad boot source or firmware.
    if (fd == -1)
    {
        LE_ERROR("Unable to open file '%s' for reading (%m).",
                  filePath);
        return false;
    }

    do
    {
        result = read(fd, buf, bufSize);
    }
    while ((result == -1) && (errno == EINTR));

    LE_FATAL_IF(result == -1, "Error reading from file '%s' (%m).", filePath);

    // Make sure to add EOL or clear the new line from the buffer.
    if (result == bufSize || buf[result-1] ==  '\n')
    {
        buf[result-1] = '\0';
    }

    // Keep trying to close the fd as long as it keeps getting interrupted by signals.
    do
    {
        result = close(fd);
    }
    while ((result == -1) && (errno == EINTR));

    LE_FATAL_IF(result != 0, "Failed to close file descriptor %d. Errno = %d (%m).", fd, errno);
    return true;
}


//--------------------------------------------------------------------------------------------------
//                                       Public function definitions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether boot-reason was timer expiry.
 *
 * @return
 *      - TRUE if boot-reason was timer expiry.
 *      - FALSE otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_bootReason_WasTimer
(
    void
)
{
   // Check the trigger file and return the result.
   return IsBootSourceTriggered(TIMER_TRIGGER_FILE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether boot-reason was specific gpio change. GPIO number is specified in parameter.
 *
 * @return
 *      - TRUE if boot-reason was specified gpio change.
 *      - FALSE otherwise.
 *
 * @note The process will kill caller if invalid gpio number is passed.  Check corresponding device
 *  documents for valid list of gpio.
 */
//--------------------------------------------------------------------------------------------------
bool le_bootReason_WasGpio
(
    uint32_t gpioNum       ///< Gpio number.
)
{
    // Only two gpio, so this approach is fine.
    char gpioTriggerPath[256] = "";
    // Build gpio trigger path, i.e. "/sys/module/swimcu_pm/boot_source/gpio<gpio-num>/triggered"
    snprintf(gpioTriggerPath, sizeof(gpioTriggerPath), GPIO_TRIGGER_FILE, gpioNum);

    // Check the trigger file and return the result.
    return IsBootSourceTriggered(gpioTriggerPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether boot reason was due to the specified ADC having a reading above or below the
 * configured limits.
 *
 * @return
 *      true if boot reason was due to the given ADC or false otherwise.
 *
 * @note
 *      The process exits if an invalid ADC number is passed. Check corresponding device documents
 *      for valid list of ADC numbers.
 */
//--------------------------------------------------------------------------------------------------
bool le_bootReason_WasAdc
(
    uint32_t adcNum
)
{
    char adcTriggerPath[256] = "";
    // Build ADC trigger path, i.e. "/sys/module/swimcu_pm/boot_source/adc/adc<adcNum>/triggered"
    snprintf(adcTriggerPath, sizeof(adcTriggerPath), ADC_TRIGGER_FILE, adcNum);

    // Check the trigger file and return the result.
    return IsBootSourceTriggered(adcTriggerPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the number of GPIOs that are specified in the system.
 *
 * @return
 *      LE_OK if we successfully get all the GPIOs and place them in the array.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetGpioCount
(
    uint8_t* gpioArrayPtr,      ///< [INOUT]    Buffer that will be filled with GPIO numbers.
    size_t* gpioArraySizePtr    ///< [INOUT]    The amount of GPIOs returned.
)
{
    struct stat statBuf;
    DIR* dirPtr;
    struct dirent* entryPtr;
    *gpioArraySizePtr = 0;

    if (stat(BOOT_SOURCE_DIR, &statBuf) == -1)
    {
        LE_ERROR("Boot source directory does not exist");
        return LE_FAULT;
    }

    if ((dirPtr = opendir(BOOT_SOURCE_DIR)) == NULL)
    {
        LE_ERROR("Unable to open boot source directory");
        return LE_FAULT;
    }
    while ((entryPtr = readdir(dirPtr)) != NULL)
    {
        // Find the directories starting with gpio.
        if (strncmp(entryPtr->d_name, "gpio", 4) == 0)
        {
            // Get the integer value of the gpio so we don't need to pass
            // as much data between processes.
            // Ex: 'gpio38' -> only pass 38
            gpioArrayPtr[*gpioArraySizePtr] = atoi(entryPtr->d_name + 4);
            *gpioArraySizePtr += 1;
        }
    }
    // Close directory.
    closedir(dirPtr);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the number of ADCs that are specified in the system.
 *
 * @return
 *      LE_OK if we successfully get all the ADCs and place them in the array.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetAdcCount
(
    uint8_t* adcArrayPtr,       ///< [INOUT]    Buffer that will be filled with ADC numbers.
    size_t* adcArraySizePtr     ///< [INOUT]    The amount of ADCs returned.
)
{
    struct stat statBuf;
    DIR* dirPtr;
    struct dirent* entryPtr;
    *adcArraySizePtr = 0;

    if (stat(ADC_SOURCE_DIR, &statBuf) == -1)
    {
        LE_ERROR("ADC source directory does not exist");
        return LE_FAULT;
    }

    // Iterate through the ADC directory.
    if ((dirPtr = opendir(ADC_SOURCE_DIR)) == NULL)
    {
        LE_ERROR("Unable to open ADC source directory");
        return LE_FAULT;
    }
    while ((entryPtr = readdir(dirPtr)) != NULL)
    {
        // Find the directories starting with adc.
        if (strncmp(entryPtr->d_name, "adc", 3) == 0)
        {
            adcArrayPtr[*adcArraySizePtr] = atoi(entryPtr->d_name + 3);
            *adcArraySizePtr += 1;
        }
    }

    // Close directory.
    closedir(dirPtr);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the information for a specific GPIO and place all the information into the buffer.
 *
 * @return
 *      LE_OK if we successfully get all the information.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetGpioInfo
(
    char* buf,                  ///< [INOUT]    Buffer that will be filled.
    size_t bufSize,             ///< [IN]       Size of the buffer passed into this function.
    const char* gpio            ///< [IN]       GPIO that we are looking for.
)
{
    // Information we are trying to get.
    char edge[8] = "";
    char pull[5] = "";
    char triggered[3] = "";
    // Make sure the directory exists.
    struct stat statBuf;
    char gpioPath[256] = {0};
    int bytesCopied = 0;

    // Check to make sure this directory exists.
    bytesCopied = snprintf(gpioPath, sizeof(gpioPath), "%s/%s", BOOT_SOURCE_DIR, gpio);
    if (bytesCopied <= 0 || bytesCopied > sizeof(gpioPath))
    {
        LE_ERROR("Issue with copying GPIO path");
        return LE_FAULT;
    }
    if (stat(gpioPath, &statBuf) == -1)
    {
        LE_ERROR("Cannot access this path: '%s'", gpioPath);
        return LE_FAULT;
    }

    // to "NA" if it's false
    if (!ReadFile(gpio, GPIO_EDGE_CFG_FILE, edge, sizeof(edge)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(edge, sizeof(edge), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(edge))
        {
            LE_ERROR("Issue copying 'NA' to edge buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }
    if (!ReadFile(gpio, GPIO_PULL_CFG_FILE, pull, sizeof(pull)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(pull, sizeof(pull), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(pull))
        {
            LE_ERROR("Issue copying 'NA' to pull buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }
    if (!ReadFile(gpio, GPIO_TRIGGERED_CFG_FILE, triggered, sizeof(triggered)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(triggered, sizeof(triggered), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(triggered))
        {
            LE_ERROR("Issue copying 'NA' to triggered buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }

    bytesCopied = snprintf(buf, bufSize, GPIO_STATUS_BLOCK,
                            edge, pull, triggered);
    if (bytesCopied <= 0 || bytesCopied > bufSize)
    {
        LE_ERROR("Issue copying to buffer for '%s',likely buffer is too small", gpio);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the information for a specific ADC and place all the information into the buffer.
 *
 * @return
 *      LE_OK if we successfully get all the information.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetAdcInfo
(
    char* buf,                  ///< [INOUT]    Buffer that will be filled.
    size_t bufSize,             ///< [IN]       Size of the buffer passed into this function.
    const char* adc             ///< [IN]       The ADC that we are looking for.
)
{
    // Values we are trying to read.
    char above[16] = "";
    char below[16] = "";
    char select[3] = "";
    char triggered[3] = "";

    struct stat statBuf;
    char adcPath[256] = {0};

    int bytesCopied = 0;

    // Check to make sure this directory exists.
    bytesCopied =  snprintf(adcPath, sizeof(adcPath), "%s/%s", ADC_SOURCE_DIR, adc);
    if (bytesCopied <= 0 || bytesCopied > sizeof(adcPath))
    {
        LE_ERROR("Buffer to hold the ADC path is too small");
        return LE_FAULT;
    }
    if (stat(adcPath, &statBuf) == -1)
    {
        LE_ERROR("Cannot access this path: '%s'", adcPath);
        return LE_FAULT;
    }

    if (!ReadFile(adc, ADC_BELOW_LEVEL_FILE, below, sizeof(below)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(below, sizeof(below), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(below))
        {
            LE_ERROR("Issue copying 'NA' to below buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }
    if (!ReadFile(adc, ADC_ABOVE_LEVEL_FILE, above, sizeof(above)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(above, sizeof(above), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(above))
        {
            LE_ERROR("Issue copying 'NA' to above buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }
    if (!ReadFile(adc, ADC_SELECT_FILE, select, sizeof(select)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(select, sizeof(select), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(select))
        {
            LE_ERROR("Issue copying 'NA' to select buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }
    if (!ReadFile(adc, ADC_TRIGGER_CFG_FILE, triggered, sizeof(triggered)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(triggered, sizeof(triggered), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(triggered))
        {
            LE_ERROR("Issue copying 'NA' to triggered buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }

    bytesCopied = snprintf(buf, bufSize, ADC_STATUS_BLOCK,
                                    below, above, select, triggered);
    if (bytesCopied <= 0 || bytesCopied > bufSize)
    {
        LE_ERROR("Issue copying to buffer for '%s',likely buffer is too small", adc);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the information about ADC interval and place all the information into the buffer.
 *
 * @return
 *      LE_OK if we successfully get all the information.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetAdcInterval
(
    char* buf,                  ///< [INOUT]    Buffer that will be filled.
    size_t bufSize              ///< [IN]       Size of the buffer passed into this function.
)
{
    struct stat statBuf;
    int bytesCopied = 0;
    char interval[16];

    if (stat(ADC_POLL_INTERVAL_FILE, &statBuf) == -1)
    {
        LE_ERROR("ADC interval file does not exist: %s", ADC_POLL_INTERVAL_FILE);
        return LE_FAULT;
    }

    if (!ReadFile(NULL, ADC_POLL_INTERVAL_FILE, interval, sizeof(interval)))
    {
        bytesCopied = snprintf(buf, bufSize, "%s", "NA");

        if (bytesCopied <= 0 || bytesCopied > bufSize)
        {
            LE_ERROR("Issue copying 'NA' to buffer, likely buffer is too small");
            return LE_FAULT;
        }
        return LE_UNAVAILABLE;
    }

    bytesCopied = snprintf(buf, bufSize, "Interval:%s", interval);
    if (bytesCopied <= 0 || bytesCopied > bufSize)
    {
        LE_ERROR("Issue copying 'interval' value to buffer, likely buffer is too small");
        return LE_FAULT;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the information about timer and place all the information into the buffer.
 *
 * @return
 *      LE_OK if we successfully get all the information.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetTimerInfo
(
    char* buf,                  ///< [INOUT]    Buffer that will be filled.
    size_t bufSize              ///< [IN]       Size of the buffer passed into this function.
)
{
    struct stat statBuf;
    int bytesCopied = 0;
    char timeout[11];
    char triggered[3];

    if (stat(TIMER_SOURCE_DIR, &statBuf) == -1)
    {
        LE_ERROR("Timer directory does not exist: %s", TIMER_SOURCE_DIR);
        return LE_FAULT;
    }

    if (!ReadFile(NULL, TIMER_TIMEOUT_FILE, timeout, sizeof(timeout)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(timeout, sizeof(timeout), "%s", "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(timeout))
        {
            LE_ERROR("Issue copying 'NA' to timeout buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }

    if (!ReadFile(NULL, TIMER_TRIGGER_FILE, triggered, sizeof(triggered)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(triggered, sizeof(triggered), "%s", "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(triggered))
        {
            LE_ERROR("Issue copying 'NA' to triggered buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }

    bytesCopied = snprintf(buf, bufSize, "\tTimeout:%s\n\tTriggered:%s\n", timeout, triggered);
    if (bytesCopied <= 0 || bytesCopied > bufSize)
    {
        LE_ERROR("Issue copying timer values to buffer, likely buffer is too small");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the information about shutdown strategy and place all the information into the buffer.
 *
 * @return
 *      LE_OK if we successfully get all the information.
 *      LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_bootReason_GetShutdownStrategy
(
    char* buf,                  ///< [INOUT]    Buffer that will be filled.
    size_t bufSize              ///< [IN]       Size of the buffer passed into this function.
)
{
    struct stat statBuf;
    int bytesCopied = 0;
    // Hold the value of the ultra low power state.
    char enable[3];
    if (stat(BOOT_SOURCE_DIR, &statBuf) == -1)
    {
        LE_ERROR("Boot source directory does not exist: %s", BOOT_SOURCE_DIR);
        return LE_FAULT;
    }

    if (!ReadFile(NULL, SHUTDOWN_INIT_FILE, enable, sizeof(enable)))
    {
        // If we fail to read the file, we will just return 'NA' and LE_OK
        bytesCopied = snprintf(enable, sizeof(enable), "NA");

        if (bytesCopied <= 0 || bytesCopied > sizeof(enable))
        {
            LE_ERROR("Issue copying 'NA' to timeout buffer, likely buffer is too small");
            return LE_FAULT;
        }
    }

    // Enable should only have valid values from 0,1,2,4,5,6.
    if (enable[0] == '0')
    {
        bytesCopied = snprintf(buf, bufSize, "Resquest to disable PSM\n");
    }
    else if (enable[0] == '0')
    {
        bytesCopied = snprintf(buf, bufSize, "Resquest to disable PSM\n");
    }
    else if (enable[0] == '1')
    {
        bytesCopied = snprintf(buf, bufSize, "Request enable PSM with ULPM fallback\n");
    }
    else if (enable[0] == '2')
    {
        bytesCopied = snprintf(buf, bufSize, "Request power off module\n");
    }
    else if (enable[0] == '4')
    {
        bytesCopied = snprintf(buf, bufSize, "No request (Default value)\n");
    }
    else if (enable[0] == '5')
    {
        bytesCopied = snprintf(buf, bufSize, "Request enable PSM only\n");
    }
    else if (enable[0] == '6')
    {
        bytesCopied = snprintf(buf, bufSize, "Request enable ULPM only\n");
    }
    else
    {
        bytesCopied = snprintf(buf, bufSize, "%s\n", enable);
    }

    if (bytesCopied <= 0 || bytesCopied > bufSize)
    {
        LE_ERROR("Issue copying timer values to buffer, likely buffer is too small");
        return LE_FAULT;
    }

    return LE_OK;
}