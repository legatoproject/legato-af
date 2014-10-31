//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the User Adder/Remover component.
 *
 * Copyright 2014, Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "userAdderRemover.h"
#include "user.h"


COMPONENT_INIT
{
    user_Init();
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an application's user to the system.
 **/
//--------------------------------------------------------------------------------------------------
void userAddRemove_Add
(
    const char* appName
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;
    uid_t uid;
    gid_t gid;

    char userName[256] = "app";

    result = le_utf8_Append(userName, appName, sizeof(userName), NULL);
    LE_FATAL_IF(result != LE_OK, "App name '%s' is too long.", appName);

    LE_INFO("Creating user '%s' for application '%s'.", userName, appName);

    // Start a read transaction and go to node /apps/app-name
    le_cfg_ConnectService();
    le_cfg_IteratorRef_t i = le_cfg_CreateReadTxn("/apps");
    le_cfg_GoToNode(i, appName);

    // If the node doesn't exist, bail out.
    if (!le_cfg_NodeExists(i, ""))
    {
        fprintf(stderr, "** ERROR: App '%s' doesn't exist in the system configuration.\n", appName);
    }
    else
    {
        result = user_Create(userName, &uid, &gid);

        if (result == LE_OK)
        {
            printf("Created user '%s' (uid %u, gid %u).\n", userName, uid, gid);

            // TODO: Groups configuration.

            le_cfg_CancelTxn(i);

            exit(EXIT_SUCCESS);
        }
        else if (result == LE_DUPLICATE)
        {
            // TODO: Verify correct groups configuration.

            printf("User '%s' already exists (uid %u, gid %u).\n", userName, uid, gid);

            le_cfg_CancelTxn(i);

            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr, "** ERROR: user_Create() failed for user '%s'.\n", userName);
        }
    }

    le_cfg_CancelTxn(i);
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's user from the system.
 **/
//--------------------------------------------------------------------------------------------------
void userAddRemove_Remove
(
    const char* appName
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    char userName[256] = "app";

    result = le_utf8_Append(userName, appName, sizeof(userName), NULL);
    LE_FATAL_IF(result != LE_OK, "App name '%s' is too long.", appName);

    LE_INFO("Deleting user '%s' for application '%s'.",
            userName,
            appName);

    result = user_Delete(userName);

    if (result == LE_OK)
    {
        printf("Deleted user '%s'.\n", userName);

        // TODO: Remove from supplementary groups.

        exit(EXIT_SUCCESS);
    }
    else if (result == LE_NOT_FOUND)
    {
        // TODO: Verify groups have been cleaned of this user.

        printf("User '%s' doesn't exist.\n", userName);

        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr,
                "** ERROR: user_Delete() failed for user '%s'.\n",
                userName);
    }

    exit(EXIT_FAILURE);
}
