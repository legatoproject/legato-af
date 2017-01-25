//--------------------------------------------------------------------------------------------------
/**
 * @file le_ulpm.c
 *
 * This file contains the source code of the top level ultra lower power mode API.
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * Sysfs interface to read mcu version.
 */
//--------------------------------------------------------------------------------------------------
#define MCU_VERSION_FILE       "/sys/module/swimcu_pm/firmware/version"


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

    const char* gpioStateStr;

    switch(state)
    {
        case LE_ULPM_GPIO_LOW:
            gpioStateStr = "low";
            break;
        case LE_ULPM_GPIO_HIGH:
            gpioStateStr = "high";
            break;
        case LE_ULPM_GPIO_RISING:
            gpioStateStr = "rising";
            break;
        case LE_ULPM_GPIO_FALLING:
            gpioStateStr = "falling";
            break;
        case LE_ULPM_GPIO_BOTH:
            gpioStateStr = "both";
            break;
        case LE_ULPM_GPIO_OFF:
            gpioStateStr = "off";
            break;
        default:
            LE_KILL_CLIENT("Unknown gpio state: %d", state);
            return LE_FAULT;
    }

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

    char expiryValStr[11] = "";
    snprintf(expiryValStr, sizeof(expiryValStr), "%u", expiryVal);

    //Write to sysfs config file.
    return WriteToSysfs(TIMER_CFG_FILE, expiryValStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the ultra low power manager firmware version.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_GetFirmwareVersion
(
    char* version,              ///< [OUT] Firmware version string
    size_t versionNumElements   ///< [IN] Size of version buffer
)
{
    if (version == NULL || versionNumElements <= 1)
    {
        LE_KILL_CLIENT("Client supplied bad parameter (version: %p, versionNumElements: %zd)",
                        version,
                        versionNumElements);
        return LE_FAULT;
    }

    int fd = open(MCU_VERSION_FILE, O_RDONLY);

    // Failed to open boot source file, there should be something wrong.
    if (fd == -1)
    {
        LE_ERROR("Unable to open file %s for reading (%m). Wrong platform/mcu-firmware",
                  MCU_VERSION_FILE);
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    int index = 0;

    for (; index < versionNumElements; index++)
    {
          // read one byte at a time.
          char c;
          int readCnt;

          do
          {
              readCnt = read(fd, &c, 1);
          }
          while ( (readCnt == -1) && (errno == EINTR) );

          if (readCnt == 1)
          {

              if (index == versionNumElements - 1)
              {
                  // There is still data but we've run out of buffer space.
                  version[index] = '\0';
                  result = LE_OVERFLOW;
                  break;
              }

              // Store char.
              version[index] = c;
          }
          else if (readCnt == 0)
          {
              // End of file nothing else to read.  Terminate the string and return.
              version[index] = '\0';
              result = LE_OK;
              break;
          }
          else
          {
              LE_ERROR("Could not read file: %s.  %m.", MCU_VERSION_FILE);
              result = LE_FAULT;
              break;
          }
    }

    CloseFd(fd);

    return result;
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
