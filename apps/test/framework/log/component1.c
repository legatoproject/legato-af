 /**
  * This is component 1 for the multi-component logging unit test.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#ifdef LE_COMPONENT_NAME
#undef LE_COMPONENT_NAME
#endif

#ifdef LE_LOG_SESSION
#undef LE_LOG_SESSION
#endif

#ifdef LE_LOG_LEVEL_FILTER_PTR
#undef LE_LOG_LEVEL_FILTER_PTR
#endif

#define LE_COMPONENT_NAME       Comp_1
#define LE_LOG_SESSION          Comp_1_LogSession
#define LE_LOG_LEVEL_FILTER_PTR Comp_1_LogLevelFilterPtr

#include "legato.h"
#include "component1.h"



//--------------------------------------------------------------------------------------------------
/**
 * Initialize the component.
 */
//--------------------------------------------------------------------------------------------------
void comp1_Init(void)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Component code that does some work and logs messages and traces.
 */
//--------------------------------------------------------------------------------------------------
void comp1_Foo(void)
{
    LE_DEBUG("comp1 %d msg", LE_LOG_DEBUG);
    LE_INFO("comp1 %d msg", LE_LOG_INFO);
    LE_WARN("comp1 %d msg", LE_LOG_WARN);
    LE_ERROR("comp1 %d msg", LE_LOG_ERR);
    LE_CRIT("comp1 %d msg", LE_LOG_CRIT);
    LE_EMERG("comp1 %d msg", LE_LOG_EMERG);

    le_log_TraceRef_t trace1 = le_log_GetTraceRef("key 1");
    le_log_TraceRef_t trace2 = le_log_GetTraceRef("key 2");

    LE_TRACE(trace1, "Trace msg in %s", STRINGIZE(LE_COMPONENT_NAME));
    LE_TRACE(trace2, "Trace msg in %s", STRINGIZE(LE_COMPONENT_NAME));
}
