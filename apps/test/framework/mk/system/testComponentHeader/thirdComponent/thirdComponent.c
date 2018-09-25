//--------------------------------------------------------------------------------------------------
/**
 * @file thirdComponent.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "firstComponent.h"

COMPONENT_INIT
{
    LE_INFO("Testing thirdComponent... FIRSTCOMPONENT_VAR = %d", FIRSTCOMPONENT_VAR);
}
