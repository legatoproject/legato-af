/*
 * Check optional APIs work without binding being provided.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    LE_TEST_PLAN(1);
    le_result_t result = ipcTest_TryConnectService();
    LE_TEST_OK((LE_UNAVAILABLE == result) || (LE_NOT_PERMITTED == result), "unbound service fails");
    LE_TEST_EXIT;
}
