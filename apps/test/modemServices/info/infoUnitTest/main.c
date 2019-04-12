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
#include "sysResets.h"

//--------------------------------------------------------------------------------------------------
/**
 * Specific software update reason
 */
//--------------------------------------------------------------------------------------------------
#define PA_SPECIFIC_REASON_SWAP "swap"

//--------------------------------------------------------------------------------------------------
/**
 * Info parameters
 */
//--------------------------------------------------------------------------------------------------
static char ImeiSv[LE_INFO_IMEISV_MAX_BYTES] = "111111111111111";
static char Imei[LE_INFO_IMEI_MAX_BYTES] = "314159265300979";
static char BootLoaderVersion[LE_INFO_MAX_VERS_BYTES] = "Bootloader 2.00";
static char FirmwareVersion[LE_INFO_MAX_VERS_BYTES] = "Firmware 2.00";
static char ModelDevice[LE_INFO_MAX_MODEL_LEN] = "VIRT_SIMU";
static char Meid[LE_INFO_MAX_MEID_BYTES] = "11111111";
static char Esn[LE_INFO_MAX_ESN_BYTES] = "222222222222";
static char Min[LE_INFO_MAX_MIN_BYTES] = "111111111111";
static uint16_t PrlVersion = 2;
static bool PrlFlag = true;
static char Nai[LE_INFO_MAX_NAI_BYTES] = "111111111111";
static char MfrName[LE_INFO_MAX_MFR_NAME_BYTES] = "VIRT_SIMU_MFR";
static char PriIdPn[LE_INFO_MAX_PRIID_PN_BYTES] = "11111";
static char PriIdRev[LE_INFO_MAX_PRIID_REV_BYTES] = "2222";
static char Sku[LE_INFO_MAX_SKU_BYTES] = "SKU1111111";
static char Psn[LE_INFO_MAX_PSN_BYTES] = "LY523300110105";
static char ResetStr[LE_INFO_MAX_RESET_BYTES] = { 0 };
static le_info_Reset_t ResetInformation = LE_INFO_RESET_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * Rf device status
 */
//--------------------------------------------------------------------------------------------------
static uint16_t ManufacturedId1 = 11;
static uint8_t ProductId1 = 1;
static bool Status1 = true;
static uint16_t ManufacturedId2 = 22;
static uint8_t ProductId2 = 2;
static bool Status2 = true;
static uint16_t ManufacturedIdPtr[LE_INFO_RF_DEVICES_STATUS_MAX];
static size_t ManufacturedIdNumElements = sizeof(ManufacturedIdPtr);
static uint8_t ProductIdPtr[LE_INFO_RF_DEVICES_STATUS_MAX];
static size_t ProductIdNumElements = sizeof(ProductIdPtr);
static bool StatusPtr[LE_INFO_RF_DEVICES_STATUS_MAX];
static size_t StatusNumElements = sizeof(StatusPtr);

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
    pa_infoSimu_SetMeid(Meid);
    pa_infoSimu_SetEsn(Esn);
    pa_infoSimu_SetMin(Min);
    pa_infoSimu_SetPrlVersion(PrlVersion);
    pa_infoSimu_SetPrlOnlyPreference(PrlFlag);
    pa_infoSimu_SetNai(Nai);
    pa_infoSimu_SetManufacturerName(MfrName);
    pa_infoSimu_SetPriId(PriIdPn, PriIdRev);
    pa_infoSimu_SetSku(Sku);
    pa_infoSimu_SetPlatformSerialNumber(Psn);
    pa_infoSimu_SetRfDeviceStatus(0, ManufacturedId1, ProductId1, Status1);
    pa_infoSimu_SetRfDeviceStatus(1, ManufacturedId2, ProductId2, Status2);
    pa_infoSimu_ResetErrorCase();
    pa_infoSimu_SetResetInformation(LE_INFO_RESET_USER, "");
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
    if (LE_OK != sysResets_Init())
    {
        LE_ERROR("Failed to initialize system resets counter");
    }
    // set pa info simu
    SetInfo();
    LE_INFO("======== ResetCountTest ========");
    uint64_t resetsCount;
    le_result_t res = le_info_GetExpectedResetsCount(&resetsCount);
    LE_ASSERT((res == LE_OK) || (res == LE_UNSUPPORTED));
    if(res == LE_OK)
    {
        LE_INFO("le_info_GetExpectedResetsCount => %"PRIu64"", resetsCount);
    }
    res = le_info_GetUnexpectedResetsCount(&resetsCount);
    LE_ASSERT((res == LE_OK) || (res == LE_UNSUPPORTED));
    if(res == LE_OK)
    {
        LE_INFO("le_info_GetUnexpectedResetsCount => %"PRIu64"", resetsCount);
    }

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

    LE_INFO("======== GetMeidTest ========");
    LE_ASSERT_OK(le_info_GetMeid(Meid, sizeof(Meid)));
    LE_INFO("le_info_GetMeid get => %s", Meid);
    LE_ASSERT(le_info_GetMeid(Meid, 1) == LE_OVERFLOW);
    // set error case LE_FAULT
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetMeid(Meid, sizeof(Meid)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetEsnTest ========");
    LE_ASSERT_OK(le_info_GetEsn(Esn, sizeof(Esn)));
    LE_INFO("le_info_GetEsn get => %s", Esn);
    LE_ASSERT(le_info_GetEsn(Esn, 1) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetEsn(Esn, sizeof(Esn)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetMinTest ========");
    LE_ASSERT_OK(le_info_GetMin(Min, sizeof(Min)));
    LE_INFO("le_info_GetMin get => %s", Min);
    LE_ASSERT(le_info_GetMin(Min, 1) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetMin(Min, sizeof(Min)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetPrlVersionTest ========");
    LE_ASSERT_OK(le_info_GetPrlVersion(&PrlVersion));
    pa_infoSimu_SetErrorCase(LE_NOT_FOUND);
    LE_ASSERT(le_info_GetPrlVersion(&PrlVersion) == LE_NOT_FOUND);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetPrlOnlyPreferenceTest ========");
    LE_ASSERT_OK(le_info_GetPrlOnlyPreference(&PrlFlag));
    pa_infoSimu_SetErrorCase(LE_NOT_FOUND);
    LE_ASSERT(le_info_GetPrlOnlyPreference(&PrlFlag) == LE_NOT_FOUND);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetNaiTest ========");
    LE_ASSERT_OK(le_info_GetNai(Nai, sizeof(Nai)));
    LE_INFO("le_info_GetNai get => %s", Nai);
    LE_ASSERT(le_info_GetNai(Nai, 1) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetNai(Nai, sizeof(Nai)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetManufacturerNameTest ========");
    LE_ASSERT_OK(le_info_GetManufacturerName(MfrName, sizeof(MfrName)));
    LE_INFO("le_info_GetManufacturerName get => %s", MfrName);
    LE_ASSERT(le_info_GetManufacturerName(MfrName, 1) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetManufacturerName(MfrName, sizeof(MfrName)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetPriIdTest ========");
    LE_ASSERT_OK(le_info_GetPriId(PriIdPn, sizeof(PriIdPn), PriIdRev, sizeof(PriIdRev)));
    LE_INFO("le_info_GetPriId get => PriIdPn: %s, PriIdRev: %s", PriIdPn, PriIdRev);
    LE_ASSERT(le_info_GetPriId(PriIdPn, sizeof(PriIdPn), PriIdRev, 1) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetPriId(PriIdPn, 1, PriIdRev, sizeof(PriIdRev)) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetPriId(PriIdPn, sizeof(PriIdPn), PriIdRev, sizeof(PriIdRev)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetSkuTest ========");
    LE_ASSERT_OK(le_info_GetSku(Sku, sizeof(Sku)));
    LE_INFO("le_info_GetSku get => %s", Sku);
    LE_ASSERT(le_info_GetSku(Sku, 1) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetSku(Sku, sizeof(Sku)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetPlatformSerialNumberTest ========");
    LE_ASSERT_OK(le_info_GetPlatformSerialNumber(Psn, sizeof(Psn)));
    LE_INFO("le_info_GetPlatformSerialNumber get => %s", Psn);
    LE_ASSERT(le_info_GetPlatformSerialNumber(Psn, 1) == LE_OVERFLOW);
    pa_infoSimu_SetErrorCase(LE_FAULT);
    LE_ASSERT(le_info_GetPlatformSerialNumber(Psn, sizeof(Psn)) == LE_FAULT);
    pa_infoSimu_ResetErrorCase();

    LE_INFO("======== GetRfDeviceStatusTest ========");
    pa_infoSimu_SetErrorCase(LE_UNSUPPORTED);
    LE_ASSERT(le_info_GetRfDeviceStatus(ManufacturedIdPtr,
                                        &ManufacturedIdNumElements,
                                        ProductIdPtr,
                                        &ProductIdNumElements,
                                        StatusPtr,
                                        &StatusNumElements) == LE_UNSUPPORTED);
    pa_infoSimu_ResetErrorCase();

    LE_ASSERT_OK(le_info_GetRfDeviceStatus(ManufacturedIdPtr,
                                          &ManufacturedIdNumElements,
                                          ProductIdPtr,
                                          &ProductIdNumElements,
                                          StatusPtr,
                                          &StatusNumElements));

    LE_INFO("======== GetResetInformationTest ========");
    pa_infoSimu_SetResetInformation(LE_INFO_RESET_USER, "");
    LE_ASSERT_OK(le_info_GetResetInformation(&ResetInformation, ResetStr, sizeof(ResetStr)));
    LE_ASSERT(LE_INFO_RESET_USER == ResetInformation);
    pa_infoSimu_SetResetInformation(LE_INFO_RESET_UPDATE, PA_SPECIFIC_REASON_SWAP);
    LE_ASSERT_OK(le_info_GetResetInformation(&ResetInformation, ResetStr, sizeof(ResetStr)));
    LE_ASSERT(LE_INFO_RESET_UPDATE == ResetInformation);
    LE_ASSERT(0 == memcmp(ResetStr,PA_SPECIFIC_REASON_SWAP, strlen(PA_SPECIFIC_REASON_SWAP)));

    LE_INFO("======== INFO API UnitTests OK ========");
    exit(0);
}
