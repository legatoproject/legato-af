/*
 * The "real" implementation of the functions on the server side of the API-sharing test.
 *
 * Copyright (C), Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"


void serverSide1_Foo()
{
    LE_INFO("FOO 1");
}


void serverSide2_Foo()
{
    LE_INFO("FOO 2");
}


COMPONENT_INIT
{
    // NOTE: Interfaces will auto-connect.
}
