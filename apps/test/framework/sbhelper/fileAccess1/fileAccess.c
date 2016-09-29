//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"


static void Access(const char* pathPtr)
{
    LE_FATAL_IF(open(pathPtr, O_RDONLY) == -1, "Could not open %s.  %m.", pathPtr);
}


COMPONENT_INIT
{
    Access("/bin/grep");
}
