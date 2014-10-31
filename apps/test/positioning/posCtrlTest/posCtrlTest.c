 /**
  * This module implements the le_posCtrl's tests.
  *
  * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"

// max length of command line
#define CMD_LEN_MAX 50


static bool GetCmd
(
    void
)
{
    char *strPtr;
    char cmd_string[CMD_LEN_MAX];
    char cmd1[CMD_LEN_MAX];
    bool res = true;
    le_posCtrl_ActivationRef_t actRef = NULL;
    fprintf(stderr, "Command are: 'start' to start a new client, returns an id.\n");
    fprintf(stderr, "             'stop id'to release an id.\n");
    fprintf(stderr, "              other command to exit of application\n");

    do
    {
        // Get command.
        fprintf(stderr, "\n\nSet command: 'start', 'stop id', or other to exit of application\n");
        strPtr=fgets ((char*)cmd_string, CMD_LEN_MAX, stdin);
    }
    while (strlen(strPtr) == 0);

    cmd_string[strlen(cmd_string)-1]='\0';

    sscanf (cmd_string, "%s %p", cmd1, &actRef);

    LE_INFO("command %s id %p", cmd1, (actRef ? actRef : NULL));

    if (!strcmp(cmd1, "stop"))
    {
        LE_INFO("Call le_posCtrl_Release 0x%p", actRef);
        fprintf(stderr, "le_posCtrl_Release id %p\n", actRef);
        le_posCtrl_Release(actRef);
        res = false;
    }
    else if (!strcmp(cmd1, "start"))
    {
        actRef = le_posCtrl_Request();
        LE_INFO("Call le_posCtrl_Request return 0x%p", actRef);
        fprintf(stderr, "le_posCtrl_Request id %p\n", actRef);
        res = false;
    }
    return res;
}


COMPONENT_INIT
{
    bool stopTest = false;

    LE_INFO("le_posCtrl test called.");

    while(!stopTest)
    {
        stopTest = GetCmd();
    }

    LE_INFO("Exit le_posCtrl Test!");
    exit(0);
}
