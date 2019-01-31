//--------------------------------------------------------------------------------------------------
/**
 * @file exitTest.c
 *
 * This file includes tests of platform dependent functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#if LE_CONFIG_RTOS
#   include "task.h"

static void *FreeRtosTask(void *dummy)
{
    LE_INFO("\nFreeRtos task test exit\n");
    exit(EXIT_SUCCESS);
}
#endif

COMPONENT_INIT
{
    // exit() function test
    LE_INFO("\nUnit test for platform dependent function: exit() call\n");
#if LE_CONFIG_RTOS
    xTaskCreate((TaskFunction_t)FreeRtosTask, "FreeRtosExitTest", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
#endif
    exit(EXIT_SUCCESS);
    // Nothing can be logged at this level after exit()
}
