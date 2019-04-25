//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the thread limit test.
 *
 * This is to test the behavior of thread API on limits.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"

static void* SleepThread(void * contextPtr)
{
    uintptr_t i = (uintptr_t)contextPtr;

    LE_ERROR("[thread %" PRIuPTR "] Started", i);
    while(1)
    {
        sleep(10000);
    }

    return NULL;
}

static void CreateStartThread(uintptr_t i)
{
    le_thread_Ref_t threadRef;
    char name[100];

    snprintf(name, sizeof(name), "tlimit%" PRIuPTR, i);

    LE_ERROR("[thread %" PRIuPTR "] Create", i);

    threadRef = le_thread_Create(name, SleepThread, (void*)i);
    LE_ASSERT(threadRef);

    LE_ERROR("[thread %" PRIuPTR "] Start", i);

    le_thread_Start(threadRef);
}

COMPONENT_INIT
{
    uintptr_t i;

    // Hard and soft limits are the same.
    struct rlimit lim = {10, 10};

    LE_FATAL_IF(setrlimit(RLIMIT_NPROC, &lim) != 0,
                "Could not set resource limit %s (%d): error %d.", "RLIMIT_NPROC", RLIMIT_NPROC,
                errno);

    LE_FATAL_IF(getrlimit(RLIMIT_NPROC, &lim) != 0,
                "Could not get resource limit %s (%d): error %d.", "RLIMIT_NPROC", RLIMIT_NPROC,
                errno);

    for(i=0;;i++)
    {
        CreateStartThread(i);
    }
}
