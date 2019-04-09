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
        LE_KILL_CLIENT("Unable to open file '%s' for reading (%m). Wrong boot-source or firmware ",
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
