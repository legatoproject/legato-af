//--------------------------------------------------------------------------------------------------
/**
 * @file le_ulpm.c
 *
 * This file contains the source code of the top level ultra lower power mode API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pm.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


///@{
//--------------------------------------------------------------------------------------------------
/**
 * Ultra Low power Management sysfs interface.
 */
//--------------------------------------------------------------------------------------------------
#define GPIO_CFG_FILE           "/sys/module/swimcu_pm/boot_source/gpio%u/edge"
#define TIMER_CFG_FILE          "/sys/module/swimcu_pm/boot_source/timer/timeout"
#define SHUTDOWN_INIT_FILE      "/sys/module/swimcu_pm/boot_source/enable"
///@}


//--------------------------------------------------------------------------------------------------
/**
 * Value need to be written in sysfs to enter ultra low power mode.
 */
//--------------------------------------------------------------------------------------------------
#define ULPM_ENABLE_VAL     "1"


//--------------------------------------------------------------------------------------------------
// Private function definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Closes a file descriptor.
 *
 * @note This is a wrapper around close() that takes care of retrying if interrupted by a signal,
 *       and logging a fatal error if close() fails.
 */
//--------------------------------------------------------------------------------------------------
static void CloseFd
(
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    int result;

    // Keep trying to close the fd as long as it keeps getting interrupted by signals.
    do
    {
        result = close(fd);
    }
    while ((result == -1) && (errno == EINTR));

    LE_FATAL_IF(result != 0, "Failed to close file descriptor %d. Errno = %d (%m).", fd, errno);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write a null-terminated string to a file given by filePath. The null terminator will not be written.
 * The file will be opened, the string will be written and the file will be closed.
 *
 * @return
 *      - LE_OK if write is successful.
 *      - LE_FAULT if failed to write.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteToSysfs
(
    const char* filePath,  ///<[IN] Path to the sysfs file to write.
    const char* string     ///<[IN] A string to write to this file.
)
{
    int fd = open(filePath, O_WRONLY);

    // Failed to open boot source file, there should be something wrong.
    if (fd == -1)
    {
        LE_KILL_CLIENT("Unable to open file %s for writing (%m). Wrong Boot-source or Firmware",
                       filePath);
        return LE_FAULT;
    }

    ssize_t result;

    do
    {
       result = write(fd, string, strlen(string));
    }
    while ((result == -1) && (errno == EINTR));

    CloseFd(fd);

    if (result < 0)
    {
        LE_ERROR("Error writing to sysfs file '%s' (%m).", filePath);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
// Public function definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Boot on changing of a gpio state. Gpio number is specified as parameter.
 *
 * @return
 *      - LE_OK if specified gpio is configured as boot source.
 *      - LE_FAULT if failed to configure.
 *
 * @note The process exits if an invalid gpio number is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_BootOnGpio
(
    uint32_t gpioNum,           ///< Gpio number to boot.
    le_ulpm_GpioState_t state   ///< State which should cause boot.
)
{
    char gpioFilePath[256] = "";

    // Build gpio boot source path, i.e. "/sys/module/swimcu_pm/boot_source/gpio<gpio-num>/edge"
    snprintf(gpioFilePath, sizeof(gpioFilePath), GPIO_CFG_FILE, gpioNum);

    // Only two gpio state, so this approach is fine.
    const char* gpioStateStr = (state == LE_ULPM_GPIO_HIGH) ? "high" : "low";

    //Write to sysfs config file.
    return  WriteToSysfs(gpioFilePath, gpioStateStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Boot after expiration of timer interval.
 *
 * @return
 *      - LE_OK if timer is configured as boot source.
 *      - LE_FAULT if failed to configure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_BootOnTimer
(

    uint32_t expiryVal          ///< Expiration time(in second) to boot. This is relative time from
                                ///< MDM shutdown.
)
{
    if (expiryVal > LE_ULPM_MAX_TIMEOUT_VAL)
    {
        LE_ERROR("Timer expiry value exceeds it max limit. Provided %u, Max Allowed: %u",
                expiryVal,
                LE_ULPM_MAX_TIMEOUT_VAL);
        return LE_FAULT;
    }

    char expiryValStr[11] = "";
    snprintf(expiryValStr, sizeof(expiryValStr), "%u", expiryVal);

    //Write to sysfs config file.
    return WriteToSysfs(TIMER_CFG_FILE, expiryValStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiate shutting down of app processor/modem etc.
 *
 * @return
 *      - LE_OK if entry to ultra low power mode initiates properly.
 *      - LE_NOT_POSSIBLE if shutting is not possible now. Try again.
 *      - LE_FAULT if failed to initiate.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_ShutDown
(
    void
)
{
    if (pm_CheckWakeLock())
    {
        LE_ERROR("Wakelock held!! System can't be shut down. Try again.");
        return LE_NOT_POSSIBLE;
    }

    //No one holding the wakelock. Now write to sysfs file to enter ultra low power mode.
    return WriteToSysfs(SHUTDOWN_INIT_FILE, ULPM_ENABLE_VAL);
}
