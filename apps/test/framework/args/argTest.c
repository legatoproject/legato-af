
#include "legato.h"

#define PROGRAMNAME     "testFwArgs"

static void TestProgramName(void)
{
    char buff[100];
    LE_ASSERT(le_arg_GetProgramName(buff, 100, NULL) == LE_OK);
    LE_DEBUG("Our program name is: %s", buff);
    LE_ASSERT(strcmp(buff, PROGRAMNAME) == 0);
}

static void TestNumberOfArgs(void)
{
    LE_DEBUG("The number of command line arguments: %zd", le_arg_NumArgs());
    LE_ASSERT(le_arg_NumArgs() == 4);
}

static void TestArgs(void)
{
    char buff[100];

    LE_ASSERT(le_arg_GetArg(0, buff, 100) == LE_OK);
    LE_DEBUG("First arg is '%s'", buff);
    LE_ASSERT(strcmp(buff, "param1") == 0);

    LE_ASSERT(le_arg_GetArg(1, buff, 100) == LE_OK);
    LE_DEBUG("Second arg is '%s'", buff);
    LE_ASSERT(strcmp(buff, "param 2") == 0);

    LE_ASSERT(le_arg_GetArg(2, buff, 100) == LE_OK);
    LE_DEBUG("Third arg is '%s'", buff);
    LE_ASSERT(strcmp(buff, "param") == 0);

    LE_ASSERT(le_arg_GetArg(3, buff, 100) == LE_OK);
    LE_DEBUG("Fourth arg is '%s'", buff);
    LE_ASSERT(strcmp(buff, "3") == 0);

    LE_ASSERT(le_arg_GetArg(4, buff, 100) == LE_NOT_FOUND);
}


LE_EVENT_INIT_HANDLER
{
    LE_INFO("======== Begin Command Line Arguments Test ========");

    TestProgramName();
    TestNumberOfArgs();
    TestArgs();

    LE_INFO("======== Completed Command Line Arguments Test (Passed) ========");
    exit(EXIT_SUCCESS);
}
