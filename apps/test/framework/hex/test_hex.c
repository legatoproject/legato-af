
#include <legato.h>

#include <CUnit/Console.h>
#include <CUnit/Basic.h>

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    return 0;
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{

    return 0;
}


void test_le_hex_StringToBinary(void)
{
    uint32_t res,i;
    const char*     hexString = "0123456789AbcDEF";
    const uint8_t  binString[] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    uint8_t binResult[8];

    res = le_hex_StringToBinary("010x02",strlen("010x02"),binResult,sizeof(binResult));
    CU_ASSERT_EQUAL(res,-1);

    res = le_hex_StringToBinary("010X02",strlen("010X02"),binResult,sizeof(binResult));
    CU_ASSERT_EQUAL(res,-1);

    // Test should give -1 because the function will be unable to parse the null terminator as a
    // hex digit
    res = le_hex_StringToBinary("0102",strlen("0102")+2,binResult,sizeof(binResult));
    CU_ASSERT_EQUAL(res,-1);

    // Test should give -1 because of odd input string length
    res = le_hex_StringToBinary("01023",strlen("01023"),binResult,sizeof(binResult));
    CU_ASSERT_EQUAL(res,-1);

    res = le_hex_StringToBinary(hexString,strlen(hexString),binResult,sizeof(binResult));
    CU_ASSERT_EQUAL(res,8);
    for(i=0;i<sizeof(binString);i++)
    {
        CU_ASSERT_EQUAL(binResult[i],binString[i]);
    }

    CU_PASS("le_hex_StringToBinary");
}

void test_le_hex_BinaryToString(void)
{
    uint32_t res,i;
    const char*     hexString = "0123456789ABCDEF";
    const uint8_t  binString[] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    char stringResult[16+1];

    res = le_hex_BinaryToString(binString,sizeof(binString),stringResult,sizeof(stringResult));
    CU_ASSERT_EQUAL(res,16);
    for(i=0;i<strlen(hexString);i++)
    {
        CU_ASSERT_EQUAL(stringResult[i],hexString[i]);
    }

    CU_PASS("le_hex_BinaryToString");
}

void test_le_hex_HexaToInteger(void)
{
    int result;

    result = le_hex_HexaToInteger("0x12Ab");
    CU_ASSERT_EQUAL(result, -1);

    result = le_hex_HexaToInteger("-12Ab");
    CU_ASSERT_EQUAL(result, -1);

    result = le_hex_HexaToInteger("1G2Ab");
    CU_ASSERT_EQUAL(result, -1);

    // This test is valid as long as sizeof(int) <= 8
    result = le_hex_HexaToInteger("1234567890ABCDEF1");
    CU_ASSERT_EQUAL(result, -1);

    result = le_hex_HexaToInteger("12Ab");
    CU_ASSERT_EQUAL(result, 0x12AB);
}

COMPONENT_INIT
{
    CU_TestInfo test_array[] = {
    { "Convert HexString to binary", test_le_hex_StringToBinary },
    { "Convert Binary to HexString", test_le_hex_BinaryToString },
    { "Convert hex string to int",   test_le_hex_HexaToInteger },
    CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] = {
    { "Suite test always ok"                , init_suite, clean_suite, test_array },
    CU_SUITE_INFO_NULL,
    };

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
    }

    CU_cleanup_registry();
    exit(CU_get_error());
}

