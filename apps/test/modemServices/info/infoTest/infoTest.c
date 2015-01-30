 /**
  * This module implements the le_info's tests.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  * testModemInfo Application shall be installed and executed on target.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"


/*
 * This test gets the Target Hardware Platform information and it displays it in the log and in the shell.
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
        fprintf(stderr, "\n\n le_info_GetDeviceModel get => %s\r\n", modelDevice);
        LE_INFO("le_info_GetDeviceModel get => %s", modelDevice);
        LE_INFO("======== ModelDeviceIdentityTest PASSED ========");
    }
    else
    {
        /* Other return values possibilities */
        switch (result)
        {
            case LE_OVERFLOW :
                fprintf(stderr, "\n\n le_info_GetDeviceModel return LE_OVERFLOW \r\n");
                LE_ERROR("le_info_GetDeviceModel return LE_OVERFLOW");
                break;
            case  LE_FAULT :
                fprintf(stderr, "\n\n le_info_GetDeviceModel return LE_FAULT \r\n");
                LE_ERROR("le_info_GetDeviceModel return LE_FAULT");
                break;
            default:
                fprintf(stderr, "\n\n le_info_GetDeviceModel return code %d\r\n",result);
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
 * Each Test called once.
 *  - modelDeviceIdentityTest()
 *  - ..
 */
COMPONENT_INIT
{
    LE_INFO("======== Start LE_INFO implementation Test ========");

    MeidTest();

    MdnTest();

    EsnTest();

    MinTest();

    PrlVersionTest();

    PrlOnlyPreferenceTest();

    NaiTest();

    ModelDeviceIdentityTest();

    // TODO add other le_info test.
    LE_INFO("======== Test LE_INFO implementation Tests SUCCESS ========");
    exit(EXIT_SUCCESS);

}
