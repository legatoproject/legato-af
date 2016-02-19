/*
 * Tests the priority value.  This program should be called with the Legato priority,
 * expected policy, expected nice value and expected realtime priority as command line arguments.
 * For example:
 *
 * $ priorities "low" "SCHED_IDLE" "0" "0"
 *
 */

#include "legato.h"

static void TestPolicy(void)
{
    char expectedPolicy[100];
    LE_ASSERT(le_arg_GetArg(1, expectedPolicy, sizeof(expectedPolicy)) == LE_OK);
    LE_INFO("My expected policy is %s", expectedPolicy);

    int policy = sched_getscheduler(0);

    switch (policy)
    {
        case SCHED_IDLE:
            LE_INFO("My test policy is SCHED_IDLE");
            LE_ASSERT(strcmp(expectedPolicy, "SCHED_IDLE") == 0);
            break;

        case SCHED_OTHER:
            LE_INFO("My test policy is SCHED_OTHER");
            LE_ASSERT(strcmp(expectedPolicy, "SCHED_OTHER") == 0);
            break;

        case SCHED_RR:
            LE_INFO("My test policy is SCHED_RR");
            LE_ASSERT(strcmp(expectedPolicy, "SCHED_RR") == 0);
            break;

        default:
            LE_FATAL("Unexpected scheduling policy %d", policy);
    }
}

static void TestNiceLevel(void)
{
    char expectedNiceStr[100];
    LE_ASSERT(le_arg_GetArg(2, expectedNiceStr, sizeof(expectedNiceStr)) == LE_OK);
    LE_INFO("My expected nice level is %s", expectedNiceStr);

    char niceStr[100];
    LE_ASSERT(snprintf(niceStr, sizeof(niceStr), "%d", nice(0)) < sizeof(niceStr));
    LE_INFO("My nice level is %s", niceStr);

    LE_ASSERT(strcmp(niceStr, expectedNiceStr) == 0);
}

static void TestRtPriority(void)
{
    char expectedPriorityStr[100];
    LE_ASSERT(le_arg_GetArg(3, expectedPriorityStr, sizeof(expectedPriorityStr)) == LE_OK);
    LE_INFO("My expected realtime priority is %s", expectedPriorityStr);

    struct sched_param param;
    LE_ASSERT(sched_getparam(0, &param) == 0);

    char priorityStr[100];
    LE_ASSERT(snprintf(priorityStr, sizeof(priorityStr), "%d", param.sched_priority) < sizeof(priorityStr));
    LE_INFO("My realtime priority is %s", priorityStr);

    LE_ASSERT(strcmp(priorityStr, expectedPriorityStr) == 0);
}


COMPONENT_INIT
{
    char prioritySetting[100];
    LE_ASSERT(le_arg_GetArg(0, prioritySetting, sizeof(prioritySetting)) == LE_OK);

    LE_INFO("======== Starting '%s' Priorities Test ========", prioritySetting);

    TestPolicy();
    TestNiceLevel();
    TestRtPriority();

    LE_INFO("======== '%s' Priorities Test Completed Successfully ========", prioritySetting);
    exit(EXIT_SUCCESS);
}
