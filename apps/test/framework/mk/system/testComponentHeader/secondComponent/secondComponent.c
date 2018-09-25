//--------------------------------------------------------------------------------------------------
/**
 * @file secondComponent.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "firstComponent.h"

COMPONENT_INIT
{
    LE_INFO("Testing headerComponent... FIRSTCOMPONENT_VAR = %d", FIRSTCOMPONENT_VAR);
}
