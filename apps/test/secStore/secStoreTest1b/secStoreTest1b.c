//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#define SECRET_ITEM             "secret"
#define SECRET_STRING           "Some data"
#define LOOP_STRING             "123456789"

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_INFO("=== SecStoreTest1b BEGIN ===");

    // Attempt to read the SECRET_ITEM this should fail because it should be empty.
    // NOTE: This assumes that this app is only run once each time it is re-installed.
    char buf[100];
    size_t bufSize = sizeof(buf);

    le_result_t result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_TEST_OK(result == LE_NOT_FOUND,
               "Checking item '%s' does not exist.  Result code %s.",
               SECRET_ITEM, LE_RESULT_TXT(result));

    // Write the SECRET_ITEM.
    result = le_secStore_Write(SECRET_ITEM, (uint8_t*)SECRET_STRING, sizeof(SECRET_STRING));
    LE_TEST_OK(result == LE_OK,
                "Write to sec store.  %s.", LE_RESULT_TXT(result));

    // Read SECRET_ITEM.
    bufSize = sizeof(buf);
    result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_TEST_OK(result == LE_OK,
               "Read from sec store.  %s.", LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(buf, SECRET_STRING) == 0,
                "Check read item. Should be '%s', is '%s'.", SECRET_STRING, buf);

    // clean-up
    LE_TEST_INFO("Clean up...");
    result = le_secStore_Delete(SECRET_ITEM);
    LE_TEST_OK(result == LE_OK,
               "Delete item '%s'.  %s.", SECRET_ITEM, LE_RESULT_TXT(result));

    LE_TEST_INFO("=== SecStoreTest1b END ===");

    LE_TEST_EXIT;
}
