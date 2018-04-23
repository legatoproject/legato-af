//-------------------------------------------------------------------------------------------------
/**
 * @file cm_info.h
 *
 * Handle info related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_info.h"
#include "cm_common.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of information string
 */
//--------------------------------------------------------------------------------------------------
#define CM_INFO_MAX_STRING_BYTES 100

//-------------------------------------------------------------------------------------------------
/**
 * Print the data help text to stdout.
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
            "To print the IMEI:\n"
            "\tcm info imei\n\n"
            "To print the IMEISV:\n"
            "\tcm info imeiSv\n\n"
            "To print the serial number:\n"
            "\tcm info fsn\n\n"
            "To print the firmware version:\n"
            "\tcm info firmware\n\n"
            "To print the bootloader version:\n"
            "\tcm info bootloader\n\n"
            "To print the PRI part and the PRI revision:\n"
            "\tcm info pri\n\n"
            "To print the SKU:\n"
            "\tcm info sku\n\n"
            "To print the last reset cause:\n"
            "\tcm info reset\n\n"
            "To print the number of resets:\n"
            "\tcm info resetsCount\n\n"
            );
}

//-------------------------------------------------------------------------------------------------
/**
 * Print the IMEI
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintImei
(
    bool withHeaders
)
{
    char imei[LE_INFO_IMEI_MAX_BYTES] = {0};

    le_info_GetImei(imei, sizeof(imei));

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
 * Print the IMEISV
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintImeiSv
(
    bool withHeaders
)
{
    char imeiSv[LE_INFO_IMEISV_MAX_BYTES] = {0};

    le_info_GetImeiSv(imeiSv, sizeof(imeiSv));

    if(withHeaders)
    {
        cm_cmn_FormatPrint("IMEISV", imeiSv);
    }
    else
    {
        printf("%s\n", imeiSv);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Print the last reset cause
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintResetCause
(
    bool withHeaders
)
{
    le_result_t result;
    le_info_Reset_t reset;
    char resetStr[LE_INFO_MAX_RESET_BYTES] = {0};

    result = le_info_GetResetInformation(&reset, resetStr, LE_INFO_MAX_RESET_BYTES);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to get last reset cause: %s", LE_RESULT_TXT(result));
        snprintf(resetStr, LE_INFO_MAX_RESET_BYTES, "Unknown");
    }

    if (withHeaders)
    {
        cm_cmn_FormatPrint("Last Reset Cause", resetStr);
    }
    else
    {
        printf("%s\n", resetStr);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Print the serial number
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintSerialNumber
(
    bool withHeaders
)
{
    char serialNumber[LE_INFO_MAX_PSN_BYTES] = {0};

    le_info_GetPlatformSerialNumber(serialNumber, sizeof(serialNumber));

    if(withHeaders)
    {
        cm_cmn_FormatPrint("FSN", serialNumber);
    }
    else
    {
        printf("%s\n", serialNumber);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Print the firmware version
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintFirmwareVersion
(
    bool withHeaders
)
{
    char version[LE_INFO_MAX_VERS_BYTES] = {0};

    le_info_GetFirmwareVersion(version, sizeof(version));

    if(withHeaders)
    {
        cm_cmn_FormatPrint("Firmware Version", version);
    }
    else
    {
        printf("%s\n", version);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Print the bootloader version
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintBootloaderVersion
(
    bool withHeaders
)
{
    char version[LE_INFO_MAX_VERS_BYTES] = {0};

    le_info_GetBootloaderVersion(version, sizeof(version));

    if(withHeaders)
    {
        cm_cmn_FormatPrint("Bootloader Version", version);
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
    char model[LE_INFO_MAX_MODEL_BYTES] = {0};

    le_info_GetDeviceModel(model, sizeof(model));

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
 * Print the product requirement information (PRI) part number and revision number.
 */
//--------------------------------------------------------------------------------------------------
void cm_info_PrintGetPriId
(
    bool withHeaders
)
{
    char priIdPn[LE_INFO_MAX_PRIID_PN_BYTES] = {0};
    char priIdRev[LE_INFO_MAX_PRIID_REV_BYTES] = {0};
    le_result_t  res;

    res = le_info_GetPriId(priIdPn,
                           LE_INFO_MAX_PRIID_PN_BYTES,
                           priIdRev,
                           LE_INFO_MAX_PRIID_REV_BYTES);
    if (LE_OK != res)
    {
        LE_ERROR("The function failed to get the value.");
        return;
    }
    if(withHeaders)
    {
        cm_cmn_FormatPrint("PRI Part Number (PN)", priIdPn);
        cm_cmn_FormatPrint("PRI Revision", priIdRev);
    }
    else
    {
        printf("%s %s\n", priIdPn, priIdRev);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print the carrier product requirement information (PRI) name and revision number.
 */
//--------------------------------------------------------------------------------------------------
void cm_info_PrintGetCarrierPri
(
    bool withHeaders
)
{
    char priName[LE_INFO_MAX_CAPRI_NAME_BYTES] = {0};
    char priRev[LE_INFO_MAX_CAPRI_REV_BYTES] = {0};

    le_info_GetCarrierPri(priName,
                          LE_INFO_MAX_CAPRI_NAME_BYTES,
                          priRev,
                          LE_INFO_MAX_CAPRI_REV_BYTES);

    if(withHeaders)
    {
        cm_cmn_FormatPrint("Carrier PRI Name", priName);
        cm_cmn_FormatPrint("Carrier PRI Revision", priRev);
    }
    else
    {
        printf("%s %s\n", priName, priRev);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print the MCU version
 */
//--------------------------------------------------------------------------------------------------
void cm_info_PrintMcuVersion
(
    bool withHeaders
)
{
    char mcuVersion[LE_ULPM_MAX_VERS_LEN+1] = {0};

    le_ulpm_ConnectService();

    le_ulpm_GetFirmwareVersion(mcuVersion, sizeof(mcuVersion));

    if(withHeaders)
    {
        cm_cmn_FormatPrint("MCU Version", mcuVersion);
    }
    else
    {
        printf("%s\n", mcuVersion);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print the product stock keeping unit number (SKU).
 */
//--------------------------------------------------------------------------------------------------
void cm_info_PrintGetSku
(
    bool withHeaders
)
{
    char skuId[LE_INFO_MAX_SKU_BYTES] = {0};

    le_info_GetSku(skuId, LE_INFO_MAX_SKU_BYTES);

    if(withHeaders)
    {
        cm_cmn_FormatPrint("SKU", skuId);
    }
    else
    {
        printf("%s\n", skuId);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Print the number of resets
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintResetsCount
(
    bool withHeaders
)
{
    char buf[CM_INFO_MAX_STRING_BYTES] = {0};
    int64_t expected, unexpected;

    if (LE_OK != le_info_GetExpectedResetsCount((uint64_t *)&expected))
    {
        expected = -1;
    }

    if (LE_OK != le_info_GetUnexpectedResetsCount((uint64_t *)&unexpected))
    {
        unexpected = -1;
    }

    snprintf(buf, CM_INFO_MAX_STRING_BYTES, "Expected: %ld\tUnexpected: %ld",
        (long)expected, (long)unexpected);

    if (withHeaders)
    {
        cm_cmn_FormatPrint("Resets Count", buf);
    }
    else
    {
        printf("%s\n", buf);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for info service.
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
        cm_info_PrintImeiSv(true);
        cm_info_PrintSerialNumber(true);
        cm_info_PrintFirmwareVersion(true);
        cm_info_PrintBootloaderVersion(true);
        cm_info_PrintMcuVersion(true);
        cm_info_PrintGetPriId(true);
        cm_info_PrintGetCarrierPri(true);
        cm_info_PrintGetSku(true);
        cm_info_PrintResetCause(true);
        cm_info_PrintResetsCount(true);
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
    else if (0 == strcmp(command, "imeiSv"))
    {
        cm_info_PrintImeiSv(false);
    }
    else if (strcmp(command, "fsn") == 0)
    {
        cm_info_PrintSerialNumber(false);
    }
    else if (strcmp(command, "pri") == 0)
    {
        cm_info_PrintGetPriId(false);
    }
    else if (strcmp(command, "capri") == 0)
    {
        cm_info_PrintGetCarrierPri(false);
    }
    else if (strcmp(command, "sku") == 0)
    {
        cm_info_PrintGetSku(false);
    }
    else if (strcmp(command, "mcu") == 0)
    {
        cm_info_PrintMcuVersion(false);
    }
    else if (0 == strcmp(command, "reset"))
    {
        cm_info_PrintResetCause(false);
    }
    else if (0 == strcmp(command, "resetsCount"))
    {
        cm_info_PrintResetsCount(false);
    }
    else
    {
        printf("Invalid command for info service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
