 /**
  * Main functions to use the SMS sample codes.
  *
  * Copyright (C) Sierra Wireless, Inc. 2014.
  *
  * Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include "smsSample.h"


//--------------------------------------------------------------------------------------------------
/**
 *  App init
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start SMS Sample!");

    smsmt_Receiver();
}
