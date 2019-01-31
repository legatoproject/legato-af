//--------------------------------------------------------------------------------------------------
/**
 * @file updateInfo.h
 *
 * Stored data by legato File System (FS) service may require new treatment after new system is
 * installed. This file provides api to check/update whether new system is installed.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_UPDATE_INFO_H_INCLUDE_GUARD
#define LEGATO_UPDATE_INFO_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 *  After installation of new legato, stored data by file system service (i.e. le_fs* api) may
 *  require new treatment. This function flags that a new system is installed.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t updateInfo_FlagNewSys
(
    void
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Removes new system installation flag
 */
//--------------------------------------------------------------------------------------------------
void updateInfo_RemoveNewSysFlag
(
    void
);

#endif // LEGATO_UPDATE_INFO_H_INCLUDE_GUARD
