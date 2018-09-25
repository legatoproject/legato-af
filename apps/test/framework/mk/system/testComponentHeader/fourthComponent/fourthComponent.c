//--------------------------------------------------------------------------------------------------
/**
 * @file fourthComponent.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "firstComponent.h"

COMPONENT_INIT
{
    LE_INFO("Testing fourthComponent... FIRSTCOMPONENT_VAR = %d", FIRSTCOMPONENT_VAR);
}
