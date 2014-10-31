 /**
  * This module implements the le_info's tests.
  *
  * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
  *
  * testModemInfo executable shall be copied on target and executed in a shell
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

    LE_INFO("modelDeviceIdentityTest test called.");

    result = le_info_GetDeviceModel(modelDevice, sizeof(modelDevice));
    if (result == LE_OK)
    {
        fprintf(stderr, "\n\n le_info_GetDeviceModel get => %s\r\n", modelDevice);
        LE_INFO("le_info_GetDeviceModel get => %s", modelDevice);
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
 * Each Test called once.
 *  - modelDeviceIdentityTest()
 *  - ..
 */
COMPONENT_INIT
{
    LE_INFO("le_info test called.");

    ModelDeviceIdentityTest();

    // TODO add other le_info test.

    LE_INFO("Exit le_info Test!");
    exit(0);
}
