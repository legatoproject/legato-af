// -------------------------------------------------------------------------------------------------
/**
 *  @file cm_info.h
 *
 *  Handle info related functionality
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_info.h"
#include "cm_common.h"

//-------------------------------------------------------------------------------------------------
/**
 *  Print the data help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintInfoHelp
(
    void
)
{
    printf("Info usage\n"
            "==========\n\n"
            "To print all known info:\n"
            "\tcm info\n"
            "\tcm info all\n\n"
            "To print the device model:\n"
            "\tcm info device\n\n"
            "To print IMEI:\n"
            "\tcm info imei\n\n"
            "To print firmware version:\n"
            "\tcm info firmware\n\n"
            "To print bootloader version:\n"
            "\tcm info bootloader\n\n"
            );
}

//-------------------------------------------------------------------------------------------------
/**
 *  Print the IMEI
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintImei
(
    bool withHeaders
)
{
    le_result_t result;
    char imei[LE_INFO_IMEI_MAX_BYTES] = {0};

    result = le_info_GetImei(imei, sizeof(imei));
    LE_ASSERT(result == LE_OK);

    if(withHeaders)
    {
        cm_cmn_FormatPrint("IMEI", imei);
    }
    else
    {
        printf("%s\n", imei);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 *  Print the firmware version
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintFirmwareVersion
(
    bool withHeaders
)
{
    le_result_t result;
    char version[LE_INFO_MAX_VERS_BYTES] = {0};

    result = le_info_GetFirmwareVersion(version, sizeof(version));
    LE_ASSERT(result == LE_OK);

    if(withHeaders)
    {
        cm_cmn_FormatPrint("Firmware", version);
    }
    else
    {
        printf("%s\n", version);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 *  Print the bootloader version
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintBootloaderVersion
(
    bool withHeaders
)
{
    le_result_t result;
    char version[LE_INFO_MAX_VERS_BYTES] = {0};

    result = le_info_GetBootloaderVersion(version, sizeof(version));
    LE_ASSERT(result == LE_OK);

    if(withHeaders)
    {
        cm_cmn_FormatPrint("Bootloader", version);
    }
    else
    {
        printf("%s\n", version);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print the device model identity (Target Hardware Platform).
 */
//--------------------------------------------------------------------------------------------------
void cm_info_PrintDeviceModel
(
    bool withHeaders
)
{
    le_result_t result;
    char model[LE_INFO_MAX_MODEL_BYTES] = {0};

    result = le_info_GetDeviceModel(model, sizeof(model));
    LE_ASSERT(result == LE_OK);

    if(withHeaders)
    {
        cm_cmn_FormatPrint("Device", model);
    }
    else
    {
        printf("%s\n", model);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for SMS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_info_ProcessInfoCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_info_PrintInfoHelp();
    }
    else if (strcmp(command, "all") == 0)
    {
        cm_info_PrintDeviceModel(true);
        cm_info_PrintImei(true);
        cm_info_PrintFirmwareVersion(true);
        cm_info_PrintBootloaderVersion(true);
    }
    else if (strcmp(command, "firmware") == 0)
    {
        cm_info_PrintFirmwareVersion(false);
    }
    else if (strcmp(command, "bootloader") == 0)
    {
        cm_info_PrintBootloaderVersion(false);
    }
    else if (strcmp(command, "device") == 0)
    {
        cm_info_PrintDeviceModel(false);
    }
    else if (strcmp(command, "imei") == 0)
    {
        cm_info_PrintImei(false);
    }
    else
    {
        printf("Invalid command for info service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
