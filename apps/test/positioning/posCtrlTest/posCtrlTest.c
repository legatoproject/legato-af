 /**
  * This module implements the le_posCtrl's tests.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"

// Max length of command line
#define CMD_LEN_MAX 50

/*
 * All theses commands can be used to test the positioning service behavior by using different
 *  sequences : Start, stop and kill events.
 * Multi instance of this application can be started in the shell to create different
 *  positioning client.
 * Commands:
 * - start     : Requests a positioning client and returns/displays the <ID>.
 * - stop <ID> : Releases a positioning client request with the specific <ID> (without 0x).
 * - stop 0    : LE_KILL_CLIENT() occurs in the positioning control service side.
 * - kill_null   : same behavior than stop 0.
 * - kill_assert : LE_ASSERT() API is called on the application test positioning client side.
 */
static bool GetCmd
(
    void
)
{
    char *strPtr = NULL;
    char cmd_string[CMD_LEN_MAX] = {0};
    char cmd1[CMD_LEN_MAX] = {0};
    bool res = true;
    le_posCtrl_ActivationRef_t actRef = NULL;
    fprintf(stderr, "Command are: 'start' to start a new client, returns an ID.\n");
    fprintf(stderr, "             'stop <ID>'to release the <ID> specified (without 0x).\n");
    fprintf(stderr, "             'kill_null' to kill application from service with LE_KILL_CLIENT().\n");
    fprintf(stderr, "             'kill_assert' to kill application from itself with LE_ASSERT().\n");
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
    else if (!strcmp(cmd1, "kill_null"))
    {
        LE_INFO("kill_null application");
        fprintf(stderr, "kill_null applciation\n");
        le_posCtrl_Release(NULL);
        LE_ASSERT(false);
    }
    else if (!strcmp(cmd1, "kill_assert"))
    {
        LE_INFO("kill_assert application");
        fprintf(stderr, "kill_assert applciation\n");
        LE_ASSERT(false);
    }
    return res;
}

/*
 * Multi instance of this application can be started in the shell to create different
 * positioning client.
 *
 **/
COMPONENT_INIT
{
    bool stopTest = false;

    LE_INFO("le_posCtrl test called.");

    while(!stopTest)
    {
        stopTest = GetCmd();
    }

    LE_INFO("Exit le_posCtrl Test!");
    exit(EXIT_SUCCESS);
}
