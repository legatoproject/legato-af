/**
 * This module implements the unit tests for SIM API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_simu.h"
#include "pa_info_simu.h"

static char ImeiSv[LE_INFO_IMEISV_MAX_BYTES] = "111111111111111";
static char Imei[LE_INFO_IMEI_MAX_BYTES] = "314159265300979";
static char BootLoaderVersion[LE_INFO_MAX_VERS_BYTES] = "Bootloader 2.00";
static char FirmwareVersion[LE_INFO_MAX_VERS_BYTES] = "Firmware 2.00";
static char ModelDevice[LE_INFO_MAX_MODEL_LEN] = "VIRT_SIMU";

//--------------------------------------------------------------------------------------------------
/**
 * Set the pa_info_simu
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetInfo
(
    void
)
{
    pa_infoSimu_SetImei(Imei);
    pa_infoSimu_SetImeiSv(ImeiSv);
    pa_infoSimu_SetFirmwareVersion(FirmwareVersion);
    pa_infoSimu_SetDeviceModel(ModelDevice);
    /* pa_infoSimu_SetBootloaderVersion is not called; lets PA_SIMU_INFO_DEFAULT_BOOT_VERSION be
    the default */
    pa_infoSimu_ResetErrorCase();
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== INFO API UnitTests ========");

    // set pa info simu
    SetInfo();

    LE_INFO("======== ImeiTest ========");
    LE_ASSERT_OK(le_info_GetImei(Imei, sizeof(Imei)));
    LE_INFO("le_info_GetImei get => %s", Imei);
    LE_ASSERT(le_info_GetImei(Imei, 1) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetImei(Imei, 0) == LE_FAULT);

    LE_INFO("======== ImeiSvTest ========");
    LE_ASSERT_OK(le_info_GetImeiSv(ImeiSv, sizeof(ImeiSv)));
    LE_INFO("le_info_GetImeiSv get => %s", ImeiSv);
    LE_ASSERT(le_info_GetImeiSv(ImeiSv, 1) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetImeiSv(ImeiSv, 0) == LE_FAULT);

    LE_INFO("======== GetBootloaderVersionTest ========");
    LE_ASSERT(le_info_GetBootloaderVersion(BootLoaderVersion, 0) == LE_FAULT);
    /* Inserting a buffer longer than the max size (LE_INFO_MAX_VERS_BYTES) isn't an error */
    LE_ASSERT_OK(le_info_GetBootloaderVersion(BootLoaderVersion, LE_INFO_MAX_VERS_BYTES*2));
    LE_INFO("le_info_GetBootloaderVersion get => %s", BootLoaderVersion);
    LE_ASSERT(le_info_GetBootloaderVersion(BootLoaderVersion, 2) == LE_OVERFLOW);
    LE_ASSERT_OK(le_info_GetBootloaderVersion(BootLoaderVersion, sizeof(BootLoaderVersion)));
    pa_infoSimu_SetErrorCase(LE_NOT_FOUND);
    LE_ASSERT(le_info_GetBootloaderVersion
                             (BootLoaderVersion, sizeof(BootLoaderVersion)) == LE_NOT_FOUND);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetFirmwareVersionTest ========");
    LE_ASSERT(le_info_GetFirmwareVersion(FirmwareVersion, 0) == LE_FAULT);
    /* Inserting a buffer longer than the max size (LE_INFO_MAX_VERS_BYTES) isn't an error */
    LE_ASSERT_OK(le_info_GetFirmwareVersion(FirmwareVersion, LE_INFO_MAX_VERS_BYTES*2));
    LE_INFO("le_info_GetFirmwareVersion get => %s", FirmwareVersion);
    LE_ASSERT(le_info_GetFirmwareVersion(FirmwareVersion, 2) == LE_OVERFLOW);
    LE_ASSERT_OK(le_info_GetFirmwareVersion(FirmwareVersion,sizeof(FirmwareVersion)));
    pa_infoSimu_SetErrorCase(LE_NOT_FOUND);
    LE_ASSERT(le_info_GetFirmwareVersion(FirmwareVersion, sizeof(FirmwareVersion)) == LE_NOT_FOUND);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== ModelDeviceIdentityTest ========");
    LE_ASSERT_OK(le_info_GetDeviceModel(ModelDevice, sizeof(ModelDevice)));
    LE_INFO("le_info_GetDeviceModel get => %s", ModelDevice);
    LE_ASSERT(le_info_GetDeviceModel(ModelDevice, 1) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetDeviceModel(ModelDevice, 0) == LE_FAULT);

    LE_INFO("======== INFO API UnitTests OK ========");
    exit(0);
}
