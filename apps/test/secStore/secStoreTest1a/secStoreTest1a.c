//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#define SECRET_ITEM             "secret"
#define SECRET_STRING           "My secret data"

static char loopString[900] = "1234567890";

COMPONENT_INIT
{
    // Number of items is determined at test time (depending on storage limit)
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_TEST_INFO("=== SecStoreTest1a BEGIN ===");

    // Attempt to read the SECRET_ITEM this should fail because it should be empty.
    // NOTE: This assumes that this app is only run once each time it is re-installed.
    char buf[100];
    size_t bufSize = sizeof(buf);

    le_result_t result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_TEST_OK(result == LE_NOT_FOUND,
               "Checking '%s' does not exist.  Result code %s.",
               SECRET_ITEM, LE_RESULT_TXT(result));

    // Write the SECRET_ITEM.
    result = le_secStore_Write(SECRET_ITEM, (uint8_t*)SECRET_STRING, sizeof(SECRET_STRING));
    LE_TEST_OK(result == LE_OK,
               "write secret to sec store.  %s.", LE_RESULT_TXT(result));

    // Read SECRET_ITEM.
    bufSize = sizeof(buf);
    result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_TEST_OK(result == LE_OK,
                "read secret from sec store.  %s.", LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(buf, SECRET_STRING) == 0,
               "Read item should be '%s', is '%s'.", SECRET_STRING, buf);

    // Test the secure storage limit.  Get the limit from the argument list.
    int limit;
    result = le_arg_GetIntOption(&limit, "l", NULL);
    if (result != LE_OK)
    {
        // Bail (instead of writing this as a test) as we are testing sec store,
        // not arg API
        LE_TEST_FATAL("Could not get storage limit.  %s.", LE_RESULT_TXT(result));
    }

    // Write items in a loop until our secure storage limit is reached.
    unsigned numLoopItems = (limit - sizeof(SECRET_STRING)) / sizeof(loopString);

    LE_TEST_INFO("numLoopItems %u", numLoopItems);

    int i;
    for (i = 0; i < numLoopItems; i++)
    {
        char loopItemName[100];
        snprintf(loopItemName, sizeof(loopItemName), "loop%d", i);

        result = le_secStore_Write(loopItemName, (uint8_t*)loopString, sizeof(loopString));
        LE_TEST_OK(result == LE_OK,
                   "Write %s.  %s.", loopItemName, LE_RESULT_TXT(result));
    }

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_LINUX), 1);
    // Write one more loop item.  This should fail.
    result = le_secStore_Write("lastLoopItem", (uint8_t*)loopString, sizeof(loopString));
    LE_TEST_OK(result == LE_NO_MEMORY,
               "Write beyond limit fails due to a memory limit.  %s.", LE_RESULT_TXT(result));
    LE_TEST_END_SKIP();

    // Delete item that does not exist.
    result = le_secStore_Delete("NonExistence");
    LE_TEST_OK(result == LE_NOT_FOUND,
               "Delete non-existant item.  %s.", LE_RESULT_TXT(result));

    // clean-up
    LE_INFO("Clean up...");
    result = le_secStore_Delete(SECRET_ITEM);
    LE_TEST_OK(result == LE_OK,
               "Delete item '%s'.  %s.", SECRET_ITEM, LE_RESULT_TXT(result));

    for (i = 0; i < numLoopItems; i++)
    {
        char loopItemName[100];
        snprintf(loopItemName, sizeof(loopItemName), "loop%d", i);

        result = le_secStore_Delete(loopItemName);
        LE_TEST_OK(result == LE_OK,
                   "Delete item '%s'.  %s.", loopItemName, LE_RESULT_TXT(result));
    }

    LE_TEST_INFO("=== SecStoreTest1a END ===");

    LE_TEST_EXIT;
}
