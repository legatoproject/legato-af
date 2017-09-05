//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the User Adder/Remover component.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "appUser.h"
#include "user.h"


//--------------------------------------------------------------------------------------------------
/**
 * Add an application's user to the system.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t appUser_Add
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

    result = user_Create(userName, &uid, &gid);

    if (result == LE_OK)
    {
        printf("Created user '%s' (uid %u, gid %u).\n", userName, uid, gid);

        // TODO: Groups configuration.

        return LE_OK;
    }
    else if (result == LE_DUPLICATE)
    {
        // TODO: Verify correct groups configuration.

        printf("User '%s' already exists (uid %u, gid %u).\n", userName, uid, gid);

        return LE_OK;
    }
    else
    {
        fprintf(stderr, "** ERROR: user_Create() failed for user '%s'.\n", userName);
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's user from the system.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t appUser_Remove
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

        return LE_OK;
    }
    else if (result == LE_NOT_FOUND)
    {
        // TODO: Verify groups have been cleaned of this user.

        printf("User '%s' doesn't exist.\n", userName);

        return LE_OK;
    }
    else
    {
        fprintf(stderr,
                "** ERROR: user_Delete() failed for user '%s'.\n",
                userName);
    }

    return LE_FAULT;
}
