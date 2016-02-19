//--------------------------------------------------------------------------------------------------
/**
 * @file system.c
 * Allows the supervisor to check if the current system is marked good
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "properties.h"
#include "system.h"
#include "file.h"

//--------------------------------------------------------------------------------------------------
/**
 * Location of the status file for the current system
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentStatusPath = "/legato/systems/current/status";


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return true if the system is marked "good", false otherwise (e.g., if "tried 2").
 */
//--------------------------------------------------------------------------------------------------
bool system_IsGood
(
    void
)
{
    char statusBuffer[100] = "";

    if (file_Exists(CurrentStatusPath) == false)
    {
        LE_DEBUG("System status file does not exist, system is 'untried'.");
        return false;
    }

    if (file_ReadStr(CurrentStatusPath, statusBuffer, sizeof(statusBuffer)) == -1)
    {
        LE_ERROR("The system status file '%s' could not be read, assuming a bad system.",
                 CurrentStatusPath);
        return false;
    }

    if (strncmp("good", statusBuffer, sizeof(statusBuffer)) == 0)
    {
        return true;
    }

    if (strncmp("tried ", statusBuffer, sizeof("tried ")) == 0)
    {
        LE_DEBUG("System status is '%s'.", statusBuffer);

        // Not good, yet, but not bad.
        return false;
    }

    LE_ERROR("Unknown system status found, '%s', assuming a bad system.", statusBuffer);
    return false;
}

