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
    LE_INFO("=====================================================================");
    LE_INFO("==================== SecStoreTest1a BEGIN ===========================");
    LE_INFO("=====================================================================");


    // Attempt to read the SECRET_ITEM this should fail because it should be empty.
    // NOTE: This assumes that this app is only run once each time it is re-installed.
    char buf[100];
    size_t bufSize = sizeof(buf);

    le_result_t result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_FATAL_IF(result != LE_NOT_FOUND,
                "Item '%s' should not have existed.  Result code %s.", SECRET_ITEM, LE_RESULT_TXT(result));

    LE_INFO("Secret does not exist as expected.");

    // Write the SECRET_ITEM.
    result = le_secStore_Write(SECRET_ITEM, (uint8_t*)SECRET_STRING, sizeof(SECRET_STRING));
    LE_FATAL_IF(result != LE_OK,
                "Could not write to sec store.  %s.", LE_RESULT_TXT(result));

    LE_INFO("Done writing secret.");

    // Read SECRET_ITEM.
    bufSize = sizeof(buf);
    result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_FATAL_IF(result != LE_OK,
                "Could not read from sec store.  %s.", LE_RESULT_TXT(result));

    LE_FATAL_IF(strcmp(buf, SECRET_STRING) != 0,
                "Read item should be '%s', but is '%s'.", SECRET_STRING, buf);

    LE_INFO("Read secret correctly '%s'.", buf);

    // Test the secure storage limit.  Get the limit from the argument list.
    int limit;
    result = le_arg_GetIntOption(&limit, "l", NULL);
    LE_FATAL_IF(result != LE_OK,
                "Could not get storage limit.  %s.", LE_RESULT_TXT(result));

    // Write items in a loop until our secure storage limit is reached.
    size_t numLoopItems = (limit - sizeof(SECRET_STRING)) / sizeof(loopString);

    LE_INFO("numLoopItems %zd", numLoopItems);

    int i;
    for (i = 0; i < numLoopItems; i++)
    {
        char loopItemName[100];
        snprintf(loopItemName, sizeof(loopItemName), "loop%d", i);

        result = le_secStore_Write(loopItemName, (uint8_t*)loopString, sizeof(loopString));
        LE_FATAL_IF(result != LE_OK,
                    "Could not write to sec store.  %s.", LE_RESULT_TXT(result));

        LE_INFO("Wrote %s", loopItemName);
    }

    // Write one more loop item.  This should fail.
    result = le_secStore_Write("lastLoopItem", (uint8_t*)loopString, sizeof(loopString));
    LE_FATAL_IF(result != LE_NO_MEMORY,
                    "Should have failed due to a memory limit.  %s.", LE_RESULT_TXT(result));

    // Delete item that does not exist.
    result = le_secStore_Delete("NonExistence");
    LE_FATAL_IF(result != LE_NOT_FOUND,
                    "Should have failed to delete non-existent item.  %s.", LE_RESULT_TXT(result));

    // clean-up
    LE_INFO("Clean up...");
    result = le_secStore_Delete(SECRET_ITEM);
    LE_FATAL_IF(result != LE_OK,
                    "Failed to delete item '%s'.  %s.", SECRET_ITEM, LE_RESULT_TXT(result));
    LE_INFO("Deleted %s", SECRET_ITEM);

    for (i = 0; i < numLoopItems; i++)
    {
        char loopItemName[100];
        snprintf(loopItemName, sizeof(loopItemName), "loop%d", i);

        result = le_secStore_Delete(loopItemName);
        LE_FATAL_IF(result != LE_OK,
                    "Could not delete item '%s'.  %s.", loopItemName, LE_RESULT_TXT(result));

        LE_INFO("Deleted %s", loopItemName);
    }

    LE_INFO("============ SecStoreTest1a PASSED =============");

    exit(EXIT_SUCCESS);
}
