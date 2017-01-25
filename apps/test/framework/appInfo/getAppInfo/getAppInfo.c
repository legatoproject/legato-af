//--------------------------------------------------------------------------------------------------
/**
 * Test of appInfo API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"



COMPONENT_INIT
{
    LE_INFO("********** Start App Info Test ***********");

    char appName[100];

    le_appInfo_GetName(getpid(), appName, sizeof(appName));

    if (strcmp(appName, "testAppInfo") != 0)
    {
        LE_FATAL("My app name should be testAppInfo, but is %s", appName);
    }

    LE_INFO("My app name is %s", appName);


    LE_INFO("============ App Info Test PASSED =============");
}
