//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"

#define SECRET_ITEM             "secret"
#define SECRET_STRING           "Some data"
#define LOOP_STRING             "123456789"

COMPONENT_INIT
{
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
    result = le_secStore_Read(SECRET_ITEM, (uint8_t*)buf, &bufSize);
    LE_FATAL_IF(result != LE_OK,
                "Could not read from sec store.  %s.", LE_RESULT_TXT(result));

    LE_FATAL_IF(strcmp(buf, SECRET_STRING) != 0,
                "Read item should be '%s', but is '%s'.", SECRET_STRING, buf);

    LE_INFO("Read secret correctly '%s'.", buf);

    LE_INFO("============ SecStoreTest2 PASSED =============");

    exit(EXIT_SUCCESS);
}
