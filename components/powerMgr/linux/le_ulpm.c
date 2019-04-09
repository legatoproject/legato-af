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
#define GPIO_CFG_FILE          "/sys/module/swimcu_pm/boot_source/gpio%u/edge"
#define TIMER_CFG_FILE         "/sys/module/swimcu_pm/boot_source/timer/timeout"
#define SHUTDOWN_INIT_FILE     "/sys/module/swimcu_pm/boot_source/enable"
#define ADC_POLL_INTERVAL_FILE "/sys/module/swimcu_pm/boot_source/adc/interval"
#define ADC_BELOW_LEVEL_FILE   "/sys/module/swimcu_pm/boot_source/adc/adc%u/below"
#define ADC_ABOVE_LEVEL_FILE   "/sys/module/swimcu_pm/boot_source/adc/adc%u/above"
#define ADC_SELECT_FILE        "/sys/module/swimcu_pm/boot_source/adc/adc%u/select"
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
 *      - LE_NOT_PERMITTED if permissions prevent writing to the given file.
 *      - LE_NOT_FOUND if the sysfs file does not exist.
 *      - LE_BAD_PARAMETER if the value written was not accepted.
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
        switch (errno)
        {
            case EINVAL:
                return LE_BAD_PARAMETER;
                break;

            default:
                return LE_FAULT;
                break;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Writes to one of the "above" or "below" files for configuring the ADC boot source parameters.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteAdcLevel
(
    const char* levelFile, ///< ADC level file to write the level to
    double level           ///< ADC level parameter to configure
)
{
    char value[16] = {0};
    le_result_t sysfsWriteResult;
    le_result_t result = LE_OK;

    const int charsRequired = snprintf(value, sizeof(value), "%.4f", level);
    if (charsRequired >= sizeof(value))
    {
        LE_ERROR("String conversion of (%f) is too large to fit in string buffer.", level);
        return LE_OVERFLOW;
    }

    sysfsWriteResult = WriteToSysfs(levelFile, value);
    if (sysfsWriteResult == LE_BAD_PARAMETER)
    {
        level = round(level);
        if ((level > INT_MAX) || (level <= INT_MIN))
        {
            LE_ERROR("adc level (%f) doesn't fit in an int.", level);
            result = LE_OVERFLOW;
        }
        else
        {
            snprintf(value, sizeof(value), "%d", (int)level);
            sysfsWriteResult = WriteToSysfs(levelFile, value);
            if (sysfsWriteResult != LE_OK)
            {
                LE_ERROR(
                    "Failed while writing int conversion of level (%s) to \"%s\"",
                    value,
                    levelFile);
                result = (sysfsWriteResult == LE_NOT_FOUND) ? LE_UNSUPPORTED : sysfsWriteResult;
            }
        }
    }
    else
    {
        if (sysfsWriteResult != LE_OK)
        {
            LE_ERROR("Failed while writing \"%s\"", levelFile);
            result = (sysfsWriteResult == LE_NOT_FOUND) ? LE_UNSUPPORTED : sysfsWriteResult;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
// Public function definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Configure the system to boot based on a state change of a given GPIO.
 *
 * @return
 *      - LE_OK if specified gpio is configured as boot source.
 *      - LE_NOT_PERMITTED if the process lacks sufficient permissions to configure the GPIO as a
 *        boot source.
 *      - LE_UNSUPPORTED if the device lacks the ability to boot based on the given GPIO.
 *      - LE_BAD_PARAMETER if the state parameter was rejected.
 *      - LE_FAULT if there is a non-specific failure.
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

    // Write to sysfs config file.
    const le_result_t result = WriteToSysfs(gpioFilePath, gpioStateStr);
    switch (result)
    {
        case LE_NOT_FOUND:
            return LE_UNSUPPORTED;
            break;

        default:
            return result;
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Boot after expiration of timer interval.
 *
 * @return
 *      - LE_OK if timer is configured as boot source.
 *      - LE_NOT_PERMITTED if the process lacks sufficient permissions to configure the timer as a
 *        boot source.
 *      - LE_UNSUPPORTED if the device lacks the ability to boot based on a timer.
 *      - LE_BAD_PARAMETER if the state parameter was rejected.
 *      - LE_FAULT if there is a non-specific failure.
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

    // Write to sysfs config file.
    const le_result_t result = WriteToSysfs(TIMER_CFG_FILE, expiryValStr);
    switch (result)
    {
        case LE_NOT_FOUND:
            return LE_UNSUPPORTED;
            break;

        default:
            return result;
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Configure and enable an ADC as a boot source.
 *
 * It is possible to specify a single range of operation or two ranges of operation with a
 * non-operational range in between.  When bootAboveAdcReading is less than bootBelowAdcReading,
 * then a single range bootAboveReading to bootBelowReading is the configured operational range.
 * However if bootAboveAdcReading is greater than bootBelowAdcReading, then there are two
 * operational ranges.  The first is any reading less than bootBelowAdcReading and the second is any
 * reading greater than bootAboveAdcReading.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_PERMITTED if the process lacks sufficient permissions to configure the adc as a
 *        boot source.
 *      - LE_OVERFLOW if the provided bootAboveAdcReading or bootBelowAdcReading are too large to
 *        convert to fit in the internal string buffer.
 *      - LE_BAD_PARAMETER if the pollIntervalInMs, bootAboveAdcReading or bootBelowAdcReading
 *        parameter were rejected.
 *      - LE_UNSUPPORTED if the device does not support using the given adc as a boot source.
 *      - LE_FAULT if there is a non-specific failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_BootOnAdc
(
    uint32_t adcNum,             ///< [IN] Number of the ADC to configure
    uint32_t pollIntervalInMs,   ///< [IN] How frequently to poll the ADC while sleeping
    double bootAboveAdcReading,  ///< [IN] Reading above which the system should boot
    double bootBelowAdcReading   ///< [IN] Reading below which the system should boot
)
{
    char filePath[128] = {0};
    char value[16] = {0};
    le_result_t writeResult;

    // NOTE: for both the above and below values below, the sysfs interface on the wp85 does not
    // currently (as of Release 13.1) support writing floating point values. There is documentation
    // in hwmon which suggests that the hwmon values should support floating point. There is an open
    // issue (LXSWI9X1517-197) which suggests adding support for floating point values into the
    // above/below files in sysfs.

    // below
    snprintf(filePath, sizeof(filePath), ADC_BELOW_LEVEL_FILE, adcNum);
    writeResult = WriteAdcLevel(filePath, bootBelowAdcReading);
    if (writeResult != LE_OK)
    {
        return writeResult;
    }

    // above
    snprintf(filePath, sizeof(filePath), ADC_ABOVE_LEVEL_FILE, adcNum);
    writeResult = WriteAdcLevel(filePath, bootAboveAdcReading);
    if (writeResult != LE_OK)
    {
        return writeResult;
    }

    // interval
    snprintf(value, sizeof(value), "%u", pollIntervalInMs);
    writeResult = WriteToSysfs(ADC_POLL_INTERVAL_FILE, value);
    if (writeResult != LE_OK)
    {
        LE_ERROR("Failed while writing interval.");
        return (writeResult == LE_NOT_FOUND) ? LE_UNSUPPORTED : writeResult;
    }

    // select
    snprintf(filePath, sizeof(filePath), ADC_SELECT_FILE, adcNum);
    writeResult = WriteToSysfs(filePath, "1");
    if (writeResult != LE_OK)
    {
        LE_ERROR("Failed while writing select.");
        return (writeResult == LE_NOT_FOUND) ? LE_UNSUPPORTED : writeResult;
    }

    return LE_OK;
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

              if (index == (versionNumElements - 1))
              {
                  // There is still data but we've run out of buffer space.
                  version[index] = '\0';
                  result = LE_OVERFLOW;
                  break;
              }

              // Store char.
              // The MCU version stored at MCU_VERSION_FILE includes a new line character
              // at the end. Trim this new line character if present.
              if (c == '\n')
              {
                  version[index] = '\0';
              }
              else
              {
                  version[index] = c;
              }
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
 *      - LE_NOT_PERMITTED if the process lacks sufficient permissions to perform a shutdown.
 *      - LE_UNSUPPORTED if the device lacks the ability to shutdown via ULPM.
 *      - LE_FAULT if there is a non-specific failure.
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

    le_framework_NotifyExpectedReboot();

    // No one holding the wakelock. Now write to sysfs file to enter ultra low power mode.
    le_result_t result = WriteToSysfs(SHUTDOWN_INIT_FILE, ULPM_ENABLE_VAL);

    LE_FATAL_IF(result == LE_BAD_PARAMETER, "Shutdown value (%s) rejected", ULPM_ENABLE_VAL);

    if (result == LE_NOT_FOUND)
    {
        result = LE_UNSUPPORTED;
    }

    return result;
}
