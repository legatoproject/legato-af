#include "legato.h"


static void* NewThreadFunc(void* context)
{
    LE_INFO("New Thread.");

    pause();

    return NULL;
}


COMPONENT_INIT
{
    LE_INFO("Hello, world.");

    le_thread_Ref_t newThread = le_thread_Create("new", NewThreadFunc, NULL);

    LE_ASSERT(le_thread_SetPriority(newThread, LE_THREAD_PRIORITY_HIGH) == LE_OK);

    le_thread_Start(newThread);
}
