//--------------------------------------------------------------------------------------------------
/**
 * @file firstComponent.c
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "firstComponent.h"

int FIRSTCOMPONENT_VAR = 1000;

COMPONENT_INIT
{
    LE_INFO("Testing firstComponent... FIRSTCOMPONENT_VAR = %d", FIRSTCOMPONENT_VAR);
}
