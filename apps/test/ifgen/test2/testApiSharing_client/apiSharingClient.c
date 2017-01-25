//--------------------------------------------------------------------------------------------------
/**
 * Client side of the API-sharing test, which tests that two interfaces can exist on the same
 * component that both use the same .api file.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"


void banner(char *testName)
{
    int i;
    char banner[41];

    for (i=0; i<sizeof(banner)-1; i++)
        banner[i]='=';
    banner[sizeof(banner)-1] = '\0';

    LE_INFO("\n%s %s %s", banner, testName, banner);
}


void StartTest(void)
{
    banner("Calling clientSideA_Foo()");
    clientSideA_Foo();
    banner("Calling clientSideB_Foo()");
    clientSideB_Foo();
}


COMPONENT_INIT
{
    // NOTE: Interfaces will auto-connect.

    StartTest();

    exit(EXIT_SUCCESS);
}
