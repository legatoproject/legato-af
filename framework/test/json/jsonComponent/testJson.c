/**
 * Simple test of Legato JSON API
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

struct JsonExpectation
{
    le_json_Event_t  event;
    const char      *stringValue;
    double           numericValue;
};

static const char *StaticJson =
    "{\n"
    "    \"one\": 1,\n"
    "    \"two\": [2, 2],\n"
    "    \"three\": {\n"
    "        \"3\": 3.3,\n"
    "        \"III\": null,\n"
    "        \"trois\": true,\n"
    "        \"tres\": \"\\\"three\\\"\"\n"
    "    }\n"
    "}\n";

static struct JsonExpectation Expected[] =
{
    { LE_JSON_OBJECT_START,     NULL,       0 },
    { LE_JSON_OBJECT_MEMBER,    "one",      0 },
    { LE_JSON_NUMBER,           NULL,       1 },
    { LE_JSON_OBJECT_MEMBER,    "two",      0 },
    { LE_JSON_ARRAY_START,      NULL,       0 },
    { LE_JSON_NUMBER,           NULL,       2 },
    { LE_JSON_NUMBER,           NULL,       2 },
    { LE_JSON_ARRAY_END,        NULL,       0 },
    { LE_JSON_OBJECT_MEMBER,    "three",    0 },
    { LE_JSON_OBJECT_START,     NULL,       0 },
    { LE_JSON_OBJECT_MEMBER,    "3",        0 },
    { LE_JSON_NUMBER,           NULL,       3.3 },
    { LE_JSON_OBJECT_MEMBER,    "III",      0 },
    { LE_JSON_NULL,             NULL,       0 },
    { LE_JSON_OBJECT_MEMBER,    "trois",    0 },
    { LE_JSON_TRUE,             NULL,       0 },
    { LE_JSON_OBJECT_MEMBER,    "tres",     0 },
    { LE_JSON_STRING,           "\"three\"", 0 },
    { LE_JSON_OBJECT_END,       NULL,       0 },
    { LE_JSON_OBJECT_END,       NULL,       0 }
};

static size_t TestIndex;

static void OnEvent
(
    le_json_Event_t event
)
{
    const char                  *stringValue;
    double                       numericValue;
    le_json_ParsingSessionRef_t  session;
    struct JsonExpectation      *expected;

    if (event == LE_JSON_DOC_END)
    {
        LE_TEST_OK(TestIndex == NUM_ARRAY_MEMBERS(Expected), "Saw %" PRIuS " events", TestIndex);

        session = le_json_GetSession();
        LE_TEST_OK(session != NULL, "Got session");

        le_json_Cleanup(session);
        LE_TEST_INFO("======== END SUCCESSFUL JSON TEST ========");
        LE_TEST_EXIT;
        return;
    }

    LE_TEST_OK(TestIndex < NUM_ARRAY_MEMBERS(Expected), "At event %" PRIuS, TestIndex);

    expected = &Expected[TestIndex];
    LE_TEST_OK(event == expected->event, "Expected %s event and got %s",
        le_json_GetEventName(expected->event), le_json_GetEventName(event));

    switch (expected->event)
    {
        case LE_JSON_OBJECT_MEMBER:
        case LE_JSON_STRING:
            stringValue = le_json_GetString();
            LE_TEST_OK(strcmp(expected->stringValue, stringValue) == 0,
                "Got value '%s'", stringValue);
            break;
        case LE_JSON_NUMBER:
            numericValue = le_json_GetNumber();
            LE_TEST_OK(expected->numericValue == numericValue, "Got value %f", numericValue);
            break;
        default:
            LE_TEST_OK(1, "No value to check");
            break;
    }

    ++TestIndex;
}

static void OnError
(
    le_json_Error_t  error,
    const char      *msg
)
{
    LE_TEST_FATAL("Parse error (%d): %s", error, msg);
}

COMPONENT_INIT
{
    int testCount = NUM_ARRAY_MEMBERS(Expected) * 3 + 3;

    LE_TEST_INFO("======== BEGIN JSON TEST ========");
    TestIndex = 0;
    LE_TEST_PLAN(testCount);

    LE_TEST_OK(le_json_ParseString(StaticJson, &OnEvent, &OnError, NULL) != NULL, "Created parser");
}
