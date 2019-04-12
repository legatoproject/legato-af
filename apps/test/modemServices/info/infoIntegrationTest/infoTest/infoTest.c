 /**
  * This module implements the le_info's tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  * testModemInfo Application shall be installed and executed on target.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"


/*
 * This test gets the Target Hardware Platform information and it displays it in the log and in
 * the shell.
 *
 * API Tested:
 *  le_info_GetDeviceModel().
 */
static void ModelDeviceIdentityTest
(
    void
)
{
    le_result_t result;
    char modelDevice[256];

    LE_INFO("======== ModelDeviceIdentityTest ========");

    result = le_info_GetDeviceModel(modelDevice, sizeof(modelDevice));
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetDeviceModel get => %s", modelDevice);
        LE_INFO("======== ModelDeviceIdentityTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        switch (result)
        {
            case LE_OVERFLOW :
                LE_ERROR("le_info_GetDeviceModel return LE_OVERFLOW");
                break;
            case  LE_FAULT :
                LE_ERROR("le_info_GetDeviceModel return LE_FAULT");
                break;
            default:
                LE_ERROR("le_info_GetDeviceModel return code %d\r\n",result);
                break;
        }
    }
}


/*
 * This test le_info_GetMeid API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetMeid().
 */
static void MeidTest
(
    void
)
{
    le_result_t result;
    char meid[LE_INFO_MAX_MEID_BYTES];

    LE_INFO("======== MeidTest ========");

    result = le_info_GetMeid(meid, sizeof(meid));
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetMeid get => %s", meid);
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetMeid return code %d",result);
        LE_ERROR("======== MeidTest FAILED ========");
        return;
    }

    result = le_info_GetMeid(meid, 1);
    if (result == LE_OVERFLOW)
    {
        LE_INFO("le_info_GetMeid return LE_OVERFLOW");
        LE_INFO("======== MeidTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetMeid return code %d",result);
        LE_ERROR("======== MeidTest FAILED ========");
    }
}

/*
 * This test le_info_GetEsn API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetEsn().
 */
static void EsnTest
(
    void
)
{
    le_result_t result;
    char esn[LE_INFO_MAX_ESN_BYTES];

    LE_INFO("======== EsnTest ========");

    result = le_info_GetEsn(esn, sizeof(esn));
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetEsn get => %s", esn);
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetEsn return code %d",result);
        LE_ERROR("======== EsnTest FAILED ========");
        return;
    }

    result = le_info_GetEsn(esn, 1);
    if (result == LE_OVERFLOW)
    {
        LE_INFO("le_info_GetEsn return LE_OVERFLOW");
        LE_INFO("======== EsnTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetEsn return code %d",result);
        LE_ERROR("======== EsnTest FAILED ========");
    }
}


/*
 * This test le_info_GetMdn API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetMdn().
 */
static void MdnTest
(
    void
)
{
    le_result_t result;
    char mdn[LE_INFO_MAX_MDN_BYTES];

    LE_INFO("======== MdnTest ========");

    result = le_info_GetMdn(mdn, sizeof(mdn));
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetMdn get => %s", mdn);
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetMdn return code %d",result);
        LE_ERROR("======== MdnTest FAILED ========");
        return;
    }

    result = le_info_GetMdn(mdn, 1);
    if (result == LE_OVERFLOW)
    {
        LE_INFO("le_info_GetMdn return LE_OVERFLOW");
        LE_INFO("======== MdnTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetMdn return code %d",result);
        LE_ERROR("======== MdnTest FAILED ========");
    }
}


/*
 * This test le_info_GetMin API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetMin().
 */
static void MinTest
(
    void
)
{
    le_result_t result;
    char min[LE_INFO_MAX_MIN_BYTES];

    LE_INFO("======== MinTest ========");

    result = le_info_GetMin(min, sizeof(min));
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetMin get => %s", min);
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetMin return code %d",result);
        LE_ERROR("======== MinTest FAILED ========");
        return;
    }

    result = le_info_GetMin(min, 1);
    if (result == LE_OVERFLOW)
    {
        LE_INFO("le_info_GetMin return LE_OVERFLOW");
        LE_INFO("======== MinTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetMin return code %d",result);
        LE_ERROR("======== MinTest FAILED ========");
    }
}

/*
 * This test le_info_GetPrlVersion API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetPrlVersion().
 */
static void PrlVersionTest
(
    void
)
{
    le_result_t result;
    uint16_t prlVersion;

    LE_INFO("======== PrlVersionTest ========");

    result = le_info_GetPrlVersion(&prlVersion);
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetPrlVersion get => %d", prlVersion);
        LE_INFO("======== PrlVersionTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("PrlVersionTest return code %d",result);
        LE_ERROR("======== PrlVersionTest FAILED ========");
    }
}


/*
 * This test le_info_GetPrlOnlyPreference API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetPrlOnlyPreference().
 */
static void PrlOnlyPreferenceTest
(
    void
)
{
    le_result_t result;
    bool PrlOnlyPreference;

    LE_INFO("======== PrlOnlyPreferenceTest ========");

    result = le_info_GetPrlOnlyPreference(&PrlOnlyPreference);
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetPrlOnlyPreference get => %s",
                        (PrlOnlyPreference ? "TRUE" : "FALSE") );
        LE_INFO("======== PrlOnlyPreferenceTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetPrlOnlyPreference return code %d",result);
        LE_ERROR("======== PrlOnlyPreferenceTest FAILED ========");
    }
}


/*
 * This test le_info_GetNai API.
 * CDMA Configuration need to be set in the device.
 *
 * API Tested:
 *  le_info_GetNai().
 */
static void NaiTest
(
    void
)
{
    le_result_t result;
    char nai[LE_INFO_MAX_NAI_BYTES];

    LE_INFO("======== NaiTest ========");

    result = le_info_GetNai(nai, sizeof(nai));
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetNai get => %s", nai);
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetNai return code %d",result);
        LE_ERROR("======== NaiTest FAILED ========");
        return;
    }

    result = le_info_GetNai(nai, 1);
    if (result == LE_OVERFLOW)
    {
        LE_INFO("le_info_GetNai return LE_OVERFLOW");
        LE_INFO("======== NaiTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetNai return code %d",result);
        LE_ERROR("======== NaiTest FAILED ========");
    }
}

/*
 * This test le_info_GetManufacturerName API.
 *
 * API Tested:
 *  le_info_GetManufacturerName().
 */
static void ManufacturerNameTest
(
    void
)
{
    char manufacturerNameStr[LE_INFO_MAX_MFR_NAME_BYTES] = { 0 };

    LE_INFO("======== ManufacturerNameTest ========");

    le_result_t res = le_info_GetManufacturerName(manufacturerNameStr, LE_INFO_MAX_MFR_NAME_BYTES);

    if (res == LE_OK)
    {
        LE_INFO("Manufacturer Name => '%s'",manufacturerNameStr);
        LE_INFO("======== ManufacturerNameTest PASSED ========");
    }
    else
    {
        LE_ERROR("======== ManufacturerNameTest FAILED ========");
    }
}

/*
 * This test le_info_GetPriId API.
 *
 * API Tested:
 *  - le_info_GetPriId().
 */
static void PriIdTest
(
    void
)
{
    char priIdPn[LE_INFO_MAX_PRIID_PN_BYTES];
    char priIdRev[LE_INFO_MAX_PRIID_REV_BYTES];
    le_result_t result;

    LE_INFO("======== PriIdTest ========");

    // test LE_OK or LE_FAULT if PRI+RV not present
    result = le_info_GetPriId(priIdPn, LE_INFO_MAX_PRIID_PN_BYTES,
                              priIdRev, LE_INFO_MAX_PRIID_REV_BYTES);
    LE_ASSERT((LE_OK == result) || (LE_FAULT == result));

    LE_INFO("priIdPn => %s", priIdPn);
    LE_INFO("priIdRev => %s", priIdRev);

    // test LE_OVERFLOW
    LE_ASSERT(LE_OVERFLOW == le_info_GetPriId(priIdPn,
                              LE_INFO_MAX_PRIID_PN_BYTES + 1,
                              priIdRev,
                              LE_INFO_MAX_PRIID_REV_BYTES));

    // test LE_OVERFLOW
    LE_ASSERT(LE_OVERFLOW == le_info_GetPriId(priIdPn,
                              LE_INFO_MAX_PRIID_PN_BYTES,
                              priIdRev,
                              LE_INFO_MAX_PRIID_REV_BYTES+1));

    // test LE_OVERFLOW or LE_FAULT if PRI+RV not present
    result = le_info_GetPriId(priIdPn, 1, priIdRev, LE_INFO_MAX_PRIID_REV_BYTES);
    LE_ASSERT((LE_OVERFLOW == result) || (LE_FAULT == result));

    // test LE_OVERFLOW or LE_FAULT if PRI+RV not present
    result = le_info_GetPriId(priIdPn, LE_INFO_MAX_PRIID_PN_BYTES, priIdRev, 1);
    LE_ASSERT((LE_OVERFLOW == result) || (LE_FAULT == result));
}


/*
 * This test le_info_GetSku API.
 *
 * API Tested:
 *  - le_info_GetSku().
 */
static void SkuIdTest
(
    void
)
{
    char skuId[LE_INFO_MAX_SKU_BYTES] = {0};
    char skuIdBadBuffer[2] = {0};
    le_result_t result;

    LE_INFO("======== SkuId test ========");

    // test LE_OVERFLOW or LE_FAULT if SKU not present
    result = le_info_GetSku(skuId, 1);
    LE_ASSERT((LE_OVERFLOW == result) || (LE_FAULT == result));

    // test LE_OVERFLOW or LE_FAULT if SKU not present
    result = le_info_GetSku(skuIdBadBuffer, LE_INFO_MAX_SKU_BYTES);
    LE_ASSERT((LE_OVERFLOW == result) || (LE_FAULT == result));

    // test LE_OK or LE_FAULT if SKU not present
    result = le_info_GetSku(skuId, LE_INFO_MAX_SKU_BYTES);
    LE_ASSERT((LE_OK == result) || (LE_FAULT == result));

}


/*
 * This test le_info_GetPlatformSerialNumber API.
 *
 * API Tested:
 *  - le_info_GetPlatformSerialNumber().
 */
static void PlatformSerialNumberTest
(
    void
)
{
    le_result_t result = LE_OK;
    char platformSerialNumberStr[LE_INFO_MAX_PSN_BYTES];

    LE_INFO("======== PlatformSerialNumberTest ========");

    result = le_info_GetPlatformSerialNumber(platformSerialNumberStr, LE_INFO_MAX_PSN_BYTES);
    if (result == LE_OK)
    {
        LE_INFO("le_info_GetPlatformSerialNumber get => %s", platformSerialNumberStr);
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetPlatformSerialNumber return code %d",result);
        LE_ERROR("======== PlatformSerialNumberTest FAILED ========");
        return;
    }

    result = le_info_GetPlatformSerialNumber(platformSerialNumberStr, LE_INFO_MAX_PSN_BYTES - 1);
    if (result == LE_OVERFLOW)
    {
        LE_INFO("le_info_GetPlatformSerialNumber return LE_OVERFLOW");
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetPlatformSerialNumber return code %d",result);
        LE_ERROR("======== PlatformSerialNumberTest FAILED ========");
        return;
    }
}

/*
 * This test le_info_GetRfDeviceStatus API.
 *
 * API Tested:
 *  - le_info_GetRfDeviceStatus().
 */
static void GetRfDeviceStatusTest
(
    void
)
{
    le_result_t result = LE_OK;
    int i = 0;
    uint16_t manufacturedIdPtr[LE_INFO_RF_DEVICES_STATUS_MAX];
    size_t manufacturedIdNumElements = sizeof(manufacturedIdPtr);
    uint8_t productIdPtr[LE_INFO_RF_DEVICES_STATUS_MAX];
    size_t productIdNumElements = sizeof(productIdPtr);
    bool statusPtr[LE_INFO_RF_DEVICES_STATUS_MAX];
    size_t statusNumElements = sizeof(statusPtr);

    LE_INFO("======== GetRfDeviceStatusTest ========");
    result =  le_info_GetRfDeviceStatus( manufacturedIdPtr
                                        , &manufacturedIdNumElements
                                        , productIdPtr
                                        , &productIdNumElements
                                        , statusPtr
                                        , &statusNumElements);

    if (result == LE_OK)
    {
        // Number of elements should be identical
        LE_ERROR_IF((manufacturedIdNumElements != productIdNumElements)||
                    (manufacturedIdNumElements != statusNumElements)
                    , "Unexpected NumElements (%i, %i,%i)"
                    , (int)manufacturedIdNumElements
                    , (int)productIdNumElements
                    , (int)statusNumElements);

        LE_INFO("Display RF devices status");
        for(i=0; i<manufacturedIdNumElements; i++)
        {
            LE_INFO("Status %d for MID %d, PID %d"
                    , statusPtr[i]
                    , manufacturedIdPtr[i]
                    , productIdPtr[i]);
        }
    }
    else
    {
        /* Other return values possibilities */
        LE_ERROR("le_info_GetRfDeviceStatus return code %d",result);
        LE_ERROR("======== GetRfDeviceStatusTest FAILED ========");
        return;
    }
}


/*
 * This test BootloaderVersion and FirmwareVersion.
 *
 * API Tested:
 *  - le_info_GetBootloaderVersion()
 *  - le_info_GetFirmwareVersion()
 */
static void GetDeviceBootVersionTest
(
    void
)
{
    char versionBootPtr[LE_INFO_MAX_VERS_BYTES] = {0};
    char versionFWPtr[LE_INFO_MAX_VERS_BYTES] = {0};

    LE_ASSERT(le_info_GetBootloaderVersion(versionBootPtr, 0) == LE_FAULT);
    LE_ASSERT(le_info_GetBootloaderVersion(versionBootPtr, LE_INFO_MAX_VERS_BYTES+10) == LE_OK);
    LE_ASSERT(le_info_GetBootloaderVersion(versionBootPtr, 2) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetBootloaderVersion(versionBootPtr, sizeof(versionBootPtr)) == LE_OK);
    LE_INFO("le_info_GetBootloaderVersion get => %s", versionBootPtr);

    LE_ASSERT(le_info_GetFirmwareVersion(versionFWPtr, 0) == LE_FAULT);
    LE_ASSERT(le_info_GetFirmwareVersion(versionFWPtr, LE_INFO_MAX_VERS_BYTES+10) == LE_OK);
    LE_ASSERT(le_info_GetFirmwareVersion(versionBootPtr, 2) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetFirmwareVersion(versionFWPtr, sizeof(versionFWPtr)) == LE_OK);
    LE_INFO("le_info_GetFirmwareVersion get => %s", versionFWPtr);
}

/*
 * Test le_info_GetImei and le_info_GetImeiSv APIs.
 *
 * API Tested:
 *  le_info_GetImei().
 *  le_info_GetImeiSv().
 */
static void ImeiTest
(
    void
)
{
    char imei[LE_INFO_IMEI_MAX_BYTES];
    char imeiSv[LE_INFO_IMEISV_MAX_BYTES];

    LE_INFO("======== ImeiTest ========");
    LE_ASSERT(le_info_GetImei(imei, sizeof(imei)) == LE_OK);
    LE_INFO("le_info_GetImei get => %s", imei);
    LE_ASSERT(le_info_GetImei(imei, 1) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetImei(imei, 0) == LE_FAULT);
    LE_INFO("======== ImeiTest PASSED ========");

    LE_INFO("======== ImeiSvTest ========");
    LE_ASSERT(le_info_GetImeiSv(imeiSv, sizeof(imeiSv)) == LE_OK);
    LE_INFO("le_info_GetImeiSv get => %s", imeiSv);
    LE_ASSERT(le_info_GetImeiSv(imeiSv, 1) == LE_OVERFLOW);
    LE_ASSERT(le_info_GetImeiSv(imeiSv, 0) == LE_FAULT);
    LE_INFO("======== ImeiSvTest PASSED ========");
}

/*
 * Test le_info_GetExpectedResetsCount and le_info_GetUnexpectedResetsCount APIs.
 *
 * API Tested:
 *  le_info_GetExpectedResetsCount().
 *  le_info_GetUnexpectedResetsCount().
 */
static void ReboutCountTest
(
    void
)
{
    uint64_t resetsCount;
    le_result_t res = le_info_GetExpectedResetsCount(&resetsCount);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_info_GetExpectedResetsCount => %"PRIu64"", resetsCount);
    res = le_info_GetUnexpectedResetsCount(&resetsCount);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_info_GetUnexpectedResetsCount => %"PRIu64"", resetsCount);
}



/*
 * Each Test called once.
 *  - modelDeviceIdentityTest()
 *  - ..
 */
COMPONENT_INIT
{
    LE_INFO("======== Start LE_INFO implementation Test ========");

    ReboutCountTest();

    GetDeviceBootVersionTest();

    ImeiTest();

    MeidTest();

    MdnTest();

    EsnTest();

    MinTest();

    PrlVersionTest();

    PrlOnlyPreferenceTest();

    NaiTest();

    ModelDeviceIdentityTest();
    ManufacturerNameTest();

    PriIdTest();

    SkuIdTest();

    PlatformSerialNumberTest();

    GetRfDeviceStatusTest();

    // TODO add other le_info test.
    LE_INFO("======== Test LE_INFO implementation Tests SUCCESS ========");
    exit(EXIT_SUCCESS);

}
