/**
 * Simple test of Legato CRC API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

COMPONENT_INIT
{
    uint32_t    crc;
    uint32_t    expected;
    uint8_t     data1[5] = { 0x1A, 0x2B, 0x3C, 0x4D, 0x5E };
    uint8_t     data2[16] =
                {
                    0xFE, 0xFE, 0xFE, 0xFE,
                    0xFE, 0xFE, 0xFE, 0xFE,
                    0xFE, 0xFE, 0xFE, 0xFE,
                    0xFE, 0xFE, 0xFE, 0xFE
                };
    uint8_t     data3[2] = { 0x00, 0x00 };

    LE_TEST_PLAN(3); // Indicate that there are 3 test cases in this app.

    // Execute the 3 test cases.
    crc = le_crc_Crc32(data1, sizeof(data1), LE_CRC_START_CRC32);
    expected = ~0x7F34014E;
    LE_TEST_OK(crc == expected, "Verified initial CRC (0x%08" PRIX32 ") is valid (0x%08" PRIX32 ")",
        crc, expected);

    crc = le_crc_Crc32(data2, sizeof(data2), crc);
    expected = ~0x19511C5F;
    LE_TEST_OK(crc == expected, "Verified updated CRC (0x%08" PRIX32 ") is valid (0x%08" PRIX32 ")",
        crc, expected);

    crc = le_crc_Crc32(data3, sizeof(data3), crc);
    expected = ~0x68FB167A;
    LE_TEST_OK(crc == expected, "Verified final CRC (0x%08" PRIX32 ") is valid (0x%08" PRIX32 ")",
        crc, expected);

    // End the test sequence.
    LE_TEST_EXIT;
}
