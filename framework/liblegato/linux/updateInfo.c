//--------------------------------------------------------------------------------------------------
/**
 * @file updateInfo.c
 *
 * Stored data by legato File Systm(FS) service may require new treatment after new system is
 * installed. This file implements few api to check/update whether new system is installed.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * File containing new system flag.
 */
//--------------------------------------------------------------------------------------------------
#define NEW_SYS_FLAG_PATH   "/newSystem"


//--------------------------------------------------------------------------------------------------
/**
 * New system flag value
 */
//--------------------------------------------------------------------------------------------------
#define NEW_SYS_FLAG   1


//--------------------------------------------------------------------------------------------------
/**
 *  After installation of new legato, stored data by file system service (i.e. le_fs* api) may
 *  require new treatment. This function flags that a new system is installed.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_NOT_PERMITTED  Access denied to the file containing new system flag.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t updateInfo_FlagNewSys
(
    void
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    result = le_fs_Open(NEW_SYS_FLAG_PATH, LE_FS_WRONLY | LE_FS_CREAT, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to open %s: %s", NEW_SYS_FLAG_PATH, LE_RESULT_TXT(result));
        return result;
    }

    int flag = NEW_SYS_FLAG;
    result = le_fs_Write(fileRef, (uint8_t *)&flag, sizeof(int));
    if (LE_OK != result)
    {
        LE_ERROR("failed to write %s: %s", NEW_SYS_FLAG_PATH, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("failed to close %s", NEW_SYS_FLAG_PATH);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to close %s: %s", NEW_SYS_FLAG_PATH, LE_RESULT_TXT(result));
        return result;
    }

    LE_DEBUG("Successfully written to file '%s'", NEW_SYS_FLAG_PATH);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether new system is installed.
 *
 * @return
 *  - true       If new system installed.
 *  - false      Otherwise
 */
//--------------------------------------------------------------------------------------------------
bool updateInfo_IsNewSys
(
    void
)
{
    return le_fs_Exists(NEW_SYS_FLAG_PATH);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes new system installation flag
 */
//--------------------------------------------------------------------------------------------------
void updateInfo_RemoveNewSysFlag
(
    void
)
{
    le_result_t result;

    result = le_fs_Delete(NEW_SYS_FLAG_PATH);
    if (LE_OK != result)
    {
        LE_ERROR("failed to delete %s: %s", NEW_SYS_FLAG_PATH, LE_RESULT_TXT(result));
    }
}
