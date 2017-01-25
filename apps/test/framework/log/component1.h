 /**
  * This is component 1 for the multi-component logging unit test.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"


extern le_log_SessionRef_t comp1_LogSession;
extern int comp1_LogLevelFilter;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the component.
 */
//--------------------------------------------------------------------------------------------------
void comp1_Init(void);


//--------------------------------------------------------------------------------------------------
/**
 * Component code that does some work and logs messages and traces.
 */
//--------------------------------------------------------------------------------------------------
void comp1_Foo(void);
