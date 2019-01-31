/**
 * LE-11195: Bug in liblegato/linux/args.c : le_arg_GetIntOption()
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#define ARG_SHORT   "f"
#define ARG_LONG    "foo"
#define ARG_VALUE   27
#define ARG_COUNT   3
static const char *TestArgs[ARG_COUNT + 1] =
{
    "testLE_11195",                         // Program name.
    "--" ARG_LONG "=" STRINGIZE(ARG_VALUE), // Long argument.
    "parg",                                 // Positional argument.
    NULL                                    // According to the C standard "argv[argc] shall be a
                                            // null pointer."
};

static void IntArgCallback
(
    int value
)
{
    LE_TEST_OK(value == ARG_VALUE, "argument value: %d", value);
}

static void StringArgCallback
(
    const char *value
)
{
    LE_TEST_INFO("Positional argument: %s", value);
}

COMPONENT_INIT
{
    int             value = 0;
    le_result_t     result;
    unsigned long   count;

    LE_TEST_PLAN(4);
    LE_TEST_INFO("LE-11195: Bug in liblegato/linux/args.c : le_arg_GetIntOption()");

    le_arg_SetArgs(ARG_COUNT, TestArgs);
    le_arg_SetIntCallback(&IntArgCallback, ARG_SHORT, ARG_LONG);
    le_arg_AddPositionalCallback(&StringArgCallback);
    le_arg_AllowMorePositionalArgsThanCallbacks();
    le_arg_Scan();

    count = le_arg_NumArgs();
    LE_TEST_OK(count == ARG_COUNT - 1, "argument count: %lu", count);

    result = le_arg_GetIntOption(&value, ARG_SHORT, ARG_LONG);
    LE_TEST_OK(result == LE_OK, "get option: %s", LE_RESULT_TXT(result));
    LE_TEST_OK(value == ARG_VALUE, "got value: %d", value);

    LE_TEST_EXIT;
}
