#include "legato.h"

COMPONENT_INIT
{
    int i;
    le_log_Level_t origLevel = le_log_GetFilterLevel();

    for(i = LE_LOG_DEBUG; i <= LE_LOG_EMERG; i++)
    {
        le_log_SetFilterLevel(i);
        LE_DEBUG("frame %d msg", LE_LOG_DEBUG);
        LE_INFO("frame %d msg", LE_LOG_INFO);
        LE_WARN("frame %d msg", LE_LOG_WARN);
        LE_ERROR("frame %d msg", LE_LOG_ERR);
        LE_CRIT("frame %d msg", LE_LOG_CRIT);
        LE_EMERG("frame %d msg", LE_LOG_EMERG);
    }

    // Restore original filter level -- required for RTOS where
    // all apps share a single filter level.
    le_log_SetFilterLevel(origLevel);

    le_thread_Exit(0);
}
