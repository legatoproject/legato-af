/**
  * This module is for unit testing the le_hex module in the legato
  * runtime library (liblegato.so).
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include <legato.h>


void test_le_hex_StringToBinary(void)
{
    int32_t res;
    int32_t i;
    const char* hexString = "0123456789AbcDEF";
    const uint8_t binString[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint8_t binResult[8];

    res = le_hex_StringToBinary("010x02", strlen("010x02"), binResult, sizeof(binResult));
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of an invalid character 'x'");

    res = le_hex_StringToBinary("010X02", strlen("010X02"), binResult, sizeof(binResult));
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of an invalid character 'X'");

    // Test should give -1 because the function will be unable to parse the null terminator as a
    // hex digit
    res = le_hex_StringToBinary("0102", strlen("0102") + 2, binResult, sizeof(binResult));
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of invalid NULL terminator");

    res = le_hex_StringToBinary("01023", strlen("01023"), binResult, sizeof(binResult));
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of odd input string length");

    res = le_hex_StringToBinary(hexString, strlen(hexString), binResult, sizeof(binResult));
    LE_TEST_OK(res == 8, "Convert a hex string to a byte array");
    for(i = 0; i < sizeof(binString); i++)
    {
        LE_TEST_OK(binResult[i] == binString[i], "Converted byte matches the expected result");
    }
}

void test_le_hex_BinaryToString(void)
{
    int32_t res;
    int32_t i;
    const char* hexString = "0123456789ABCDEF";
    const uint8_t binString[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    char stringResult[16 + 1];

    res = le_hex_BinaryToString(binString, sizeof(binString), stringResult, sizeof(stringResult));
    LE_TEST_OK(res == 16, "Convert a byte array to a hex string");
    for(i=0;i<strlen(hexString);i++)
    {
        LE_TEST_OK(stringResult[i] == hexString[i], "Converted char matches the expected result");
    }
}

void test_le_hex_HexaToInteger(void)
{
    int res;

    res = le_hex_HexaToInteger("0x12Ab");
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of an invalid character 'x'");

    res = le_hex_HexaToInteger("-12Ab");
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of an invalid character '-'");

    res = le_hex_HexaToInteger("1G2Ab");
    LE_TEST_OK(res == -1, "Fail to convert a hex string because of an invalid character 'G'");

    // This test is valid as long as sizeof(int) <= 8
    res = le_hex_HexaToInteger("1234567890ABCDEF1");
    LE_TEST_OK(res == -1, "Fail to convert a hex string because the string is too long");

    res = le_hex_HexaToInteger("12Ab");
    LE_TEST_OK(res == 0x12AB, "Convert a hex string to integer");
}

COMPONENT_INIT
{
    LE_TEST_INIT;
    LE_TEST_INFO("======== le_hex Test Started ========\n");

    test_le_hex_StringToBinary();
    test_le_hex_BinaryToString();
    test_le_hex_HexaToInteger();

    LE_TEST_INFO("======== le_hex Test Complete ========\n");
    LE_TEST_EXIT;
}
