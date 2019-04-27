/**
 * LE-2322: le_thread_CleanupLegatoThreadData doesn't seem to cleanup properly anymore
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

static void *testThread(void *arg)
{
    char             buffer[16];
    const char      *name = "testThread";
    le_thread_Ref_t  threadRef;

    LE_UNUSED(arg);

    le_thread_InitLegatoThreadData(name);

    threadRef = le_thread_GetCurrent();
    LE_TEST_OK(threadRef != NULL, "Current thread has Legato data (%p)", threadRef);
    le_thread_GetName(threadRef, buffer, sizeof(buffer));
    LE_TEST_OK(strncmp(buffer, name, sizeof(buffer)) == 0, "Thread name is set (%s)", buffer);

    le_thread_CleanupLegatoThreadData();

    // This should abort:
    threadRef = le_thread_GetCurrent();

    LE_TEST_FATAL("Test should have raised SIGABRT already, before getting thread reference %p",
        threadRef);
    return NULL;
}

COMPONENT_INIT
{
    int         res;
    pthread_t   thread;

    LE_TEST_PLAN(3);
    LE_TEST_INFO("LE-2322: le_thread_CleanupLegatoThreadData doesn't seem to cleanup properly "
        "anymore");

    res = pthread_create(&thread, NULL, &testThread, NULL);
    LE_TEST_ASSERT(res == 0, "Create thread: %d", res);
#if !LE_CONFIG_LINUX
    pthread_join(thread, NULL);
    LE_TEST_EXIT;
#endif /* end not LE_CONFIG_LINUX */
}
