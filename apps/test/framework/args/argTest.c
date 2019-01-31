
#include "legato.h"

#define PROGRAMNAME     "testFwArgs"

static void TestProgramName(void)
{
    const char* programName = le_arg_GetProgramName();
    LE_ASSERT(NULL != programName);
    LE_DEBUG("Our program name is: %s", programName);
    LE_ASSERT(strcmp(programName, PROGRAMNAME) == 0);
}

static void TestNumberOfArgs(void)
{
    LE_DEBUG("The number of command line arguments: %zd", le_arg_NumArgs());
    LE_ASSERT(le_arg_NumArgs() == 4);
}

static void TestArgs(void)
{
    const char* arg;

    LE_ASSERT(NULL != (arg = le_arg_GetArg(0)));
    LE_DEBUG("First arg is '%s'", arg);
    LE_ASSERT(strcmp(arg, "param1") == 0);

    LE_ASSERT(NULL != (arg = le_arg_GetArg(1)));
    LE_DEBUG("Second arg is '%s'", arg);
    LE_ASSERT(strcmp(arg, "param 2") == 0);

    LE_ASSERT(NULL != (arg = le_arg_GetArg(2)));
    LE_DEBUG("Third arg is '%s'", arg);
    LE_ASSERT(strcmp(arg, "param") == 0);

    LE_ASSERT(NULL != (arg = le_arg_GetArg(3)));
    LE_DEBUG("Fourth arg is '%s'", arg);
    LE_ASSERT(strcmp(arg, "3") == 0);

    LE_ASSERT(NULL == (arg = le_arg_GetArg(4)));
}

static void TestScanResult(void)
{
    le_result_t result = le_arg_GetScanResult();
    LE_DEBUG("Argument scan result: %u", (unsigned int) result);
    LE_ASSERT(result == LE_OK);
}

COMPONENT_INIT
{
    LE_INFO("======== Begin Command Line Arguments Test ========");

    TestProgramName();
    TestNumberOfArgs();
    TestArgs();
    TestScanResult();

    LE_INFO("======== Completed Command Line Arguments Test (Passed) ========");
    exit(EXIT_SUCCESS);
}
