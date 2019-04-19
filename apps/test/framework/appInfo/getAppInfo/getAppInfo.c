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
    LE_TEST_PLAN(7);

    pid_t pid = getpid();
    char nameBuffer[100];
    size_t nameBufferSize = sizeof(nameBuffer);

    LE_INFO("********** Start App Info Test ***********");
    // Positive tests on own app
    LE_TEST_OK(LE_OK == le_appInfo_GetName(pid, nameBuffer, nameBufferSize),
               "Getting own app name");
    LE_TEST_OK(0 == strcmp(nameBuffer, "testAppInfo"),
               "App name is 'testAppInfo'");
    LE_TEST_OK(LE_APPINFO_RUNNING == le_appInfo_GetState("testAppInfo"),
               "testAppInfo app is running");
    LE_TEST_OK(LE_APPINFO_PROC_RUNNING == le_appInfo_GetProcState("testAppInfo", "testAppInfo"),
               "testAppInfo proc in testAppInfo app is running");

    // Negative test on non-existant app
    LE_TEST_OK(LE_APPINFO_STOPPED == le_appInfo_GetState("bogusNonExistantAppName"),
               "non-existant app is stopped");
    LE_TEST_OK(LE_APPINFO_PROC_STOPPED == le_appInfo_GetProcState("bogusNonExistantAppName",
                                                                  "bogusNonExistantProcName"),
               "non-existant app process is stopped");

    // Negative test on own app but non-existant process
    LE_TEST_OK(LE_APPINFO_PROC_STOPPED == le_appInfo_GetProcState("testAppInfo",
                                                                  "bogusNonExistantProcName"),
               "non-existant process in own app is stopped");
    LE_INFO("============ App Info Test PASSED =============");
    LE_TEST_EXIT;
}

