 /**
  * This module implements the le_fwupdate's unit tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include <pthread.h>

#include "interfaces.h"
#include "pa_fwupdate_simu.h"

#include "log.h"
#include "args.h"

extern void _fwupdateComp_COMPONENT_INIT(void);

//--------------------------------------------------------------------------------------------------
/**
 * Redefinition of le_fwupdate COMPONENT_INIT
 */
//--------------------------------------------------------------------------------------------------
void le_fwupdate_Init(
    void
);

//--------------------------------------------------------------------------------------------------
// Test public functions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_Download API
 *
 * API Tested:
 *  le_fwupdate_Download().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_Download
(
    void
)
{
    le_result_t result = LE_FAULT;
    int fd;

    LE_INFO ("======== Test: le_fwupdate_Download ========");

    /* Test invalid file descriptor: API needs to return LE_BAD_PARAMETER */
    fd = -1;
    /* Indicate that systems are synchronized */
    pa_fwupdateSimu_SetSyncState (true);
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_Download (fd);
    /* Check required values */
    LE_ASSERT (result == LE_BAD_PARAMETER);

    /* Test valid file descriptor and error on PA: API needs to return LE_FAULT */
    /* Set returned error code for PA function: LE_FAULT */
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    fd = 1;
    /* Call the function to be tested */
    result = le_fwupdate_Download (fd);
    /* Check required values */
    LE_ASSERT (result == LE_FAULT);

    /* Systems are not synchronized: API needs to return LE_NOT_POSSIBLE */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the synchronization state to false */
    pa_fwupdateSimu_SetSyncState (false);
    /* Call the function to be tested */
    result = le_fwupdate_Download (fd);
    /* Check required values */
    LE_ASSERT (result == LE_NOT_POSSIBLE);

    /* Valid treatment: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the synchronization state to true */
    pa_fwupdateSimu_SetSyncState (true);
    /* Call the function to be tested */
    result = le_fwupdate_Download (fd);
    /* Check required values */
    LE_ASSERT (result == LE_OK);

    LE_INFO ("======== Test: le_fwupdate_Download PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_GetFirmwareVersion API
 *
 * API Tested:
 *  le_fwupdate_GetFirmwareVersion().
 */
//--------------------------------------------------------------------------------------------------
void Testle_fwupdate_GetFirmwareVersion
(
    void
)
{
    le_result_t result = LE_FAULT;
    char Version[20];
    LE_INFO ("======== Test: le_fwupdate_GetFirmwareVersion ========");

    /* Test LE_NOT_FOUND error if the version string is not available:
     * API needs to return LE_NOT_FOUND
     */
    /* Set returned error code for PA function: LE_NOT_FOUND */
    pa_fwupdateSimu_SetReturnCode (LE_NOT_FOUND);
    /* Call the function to be tested */
    result = le_fwupdate_GetFirmwareVersion (Version, 20);
    /* Check required values */
    LE_ASSERT (result == LE_NOT_FOUND);

    /* Test LE_OVERFLOW error if version string to big to fit in provided buffer:
     * API needs to return LE_OVERFLOW
     */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_GetFirmwareVersion (Version, 2);
    /* Check required values */
    LE_ASSERT (result == LE_OVERFLOW);

    /* Test LE_FAULT error for any other errors : API needs to return LE_FAULT */
    /* Set returned error code for PA function: LE_FAULT */
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    /* Call the function to be tested */
    result = le_fwupdate_GetFirmwareVersion (Version, 20);
    /* Check required values */
    LE_ASSERT (result == LE_FAULT);

    /* Test correct behavior: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_GetFirmwareVersion (Version, 20);
    /* Check required values */
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (0 == strncmp(Version, FW_VERSION_UT, strlen (FW_VERSION_UT)));

    LE_INFO ("======== Test: le_fwupdate_GetFirmwareVersion PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_GetBootloaderVersion API
 *
 * API Tested:
 *  le_fwupdate_GetBootloaderVersion().
 */
//--------------------------------------------------------------------------------------------------
void Testle_fwupdate_GetBootloaderVersion
(
    void
)
{
    le_result_t result = LE_FAULT;
    char Version[20];
    LE_INFO ("======== Test: le_fwupdate_GetBootloaderVersion ========");

    /* Test errors */
    /* Test LE_NOT_FOUND error if the version string is not available:
     * API needs to return LE_NOT_FOUND
     */
    /* Set returned error code for PA function: LE_NOT_FOUND */
    pa_fwupdateSimu_SetReturnCode (LE_NOT_FOUND);
    /* Call the function to be tested */
    result = le_fwupdate_GetBootloaderVersion (Version, 20);
    /* Check required values */
    LE_ASSERT (result == LE_NOT_FOUND);

    /* Test LE_OVERFLOW error if version string to big to fit in provided buffer:
     * API needs to return LE_OVERFLOW
     */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_GetBootloaderVersion (Version, 2);
    /* Check required values */
    LE_ASSERT (result == LE_OVERFLOW);

    /* Test LE_FAULT error for any other errors: API needs to return LE_FAULT */
    /* Set returned error code for PA function: LE_FAULT */
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    /* Call the function to be tested */
    result = le_fwupdate_GetBootloaderVersion (Version, 20);
    /* Check required values */
    LE_ASSERT (result == LE_FAULT);

    /* Test correct behavior: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_GetBootloaderVersion (Version, 20);
    /* Check required values */
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (0 == strncmp(Version, BOOT_VERSION_UT, strlen (BOOT_VERSION_UT)));

    LE_INFO ("======== Test: le_fwupdate_GetBootloaderVersion PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_DualSysSyncState API
 *
 * API Tested:
 *  le_fwupdate_DualSysSyncState().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_DualSysSyncState
(
    void
)
{
    le_result_t result = LE_FAULT;
    bool isSync = false;

    LE_INFO ("======== Test: le_fwupdate_DualSysSyncState ========");
    /* Simulate unsupported API: API needs to return LE_UNSUPPORTED */
    /* Set returned error code for PA function: LE_UNSUPPORTED */
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSyncState (&isSync);
    /* Check required values */
    LE_ASSERT (result == LE_UNSUPPORTED);

    /* Simulate unsynchronized systems: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the synchronization state to false */
    pa_fwupdateSimu_SetSyncState (false);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSyncState (&isSync);
    /* Check required values */
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSync == false);

    /* Simulate synchronized systems: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the synchronization state to true */
    pa_fwupdateSimu_SetSyncState (true);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSyncState (&isSync);
    /* Check required values */
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSync == true);

    LE_INFO ("======== Test: le_fwupdate_DualSysSyncState PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_DualSysSwap API
 *
 * API Tested:
 *  le_fwupdate_DualSysSwap().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_DualSysSwap
(
    void
)
{
    le_result_t result = LE_FAULT;
    bool isResetRequested = false;
    bool isNvupApplyRequested = false;

    LE_INFO ("======== Test: le_fwupdate_DualSysSwap ========");

    /* Simulate unsupported API: API needs to return LE_UNSUPPORTED */
    /* Set returned error code for PA function: LE_UNSUPPORTED */
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSwap ();
    /* Check required values */
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_UNSUPPORTED);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);

    /* Simulate error: API needs to return LE_FAULT */
    /* Set returned error code for PA function: LE_FAULT */
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSwap ();
    /* Check required values */
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);

    /* Simulate swap acceptance: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSwap ();
    /* Check required values */
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isResetRequested == true);
    LE_ASSERT (isNvupApplyRequested == true);
    /* Reset the reset flag */
    pa_fwupdateSimu_SetResetState();
    /* Reset the nvup apply flag */
    pa_fwupdateSimu_SetNvupApplyState();

    LE_INFO ("======== Test: le_fwupdate_DualSysSwap PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_DualSysSync API
 *
 * API Tested:
 *  le_fwupdate_Sync().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_DualSysSync
(
    void
)
{
    le_result_t result = LE_FAULT;
    bool isSync = false;
    pa_fwupdate_state_t state = PA_FWUPDATE_STATE_INVALID;

    LE_INFO ("======== Test: le_fwupdate_DualSysSync ========");

    /* Simulate unsupported API: API needs to return LE_UNSUPPORTED */
    /* Set returned error code for PA function: LE_UNSUPPORTED */
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSync();
    /* Check required values */
    LE_ASSERT (result == LE_UNSUPPORTED);

    /* Simulate error: API needs to return LE_FAULT */
    /* Set returned error code for PA function: LE_FAULT */
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSync();
    /* Check required values */
    LE_ASSERT (result == LE_FAULT);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_NORMAL);

    /* Simulate sync acceptance: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the synchronization state to false */
    pa_fwupdateSimu_SetSyncState (false);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSync();
    /* Check required values */
    LE_ASSERT (result == LE_OK);
    result = pa_fwupdate_DualSysGetSyncState (&isSync);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSync == true);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_SYNC);

    LE_INFO ("======== Test: le_fwupdate_Sync PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_DualSysSwapAndSync API
 *
 * API Tested:
 *  le_fwupdate_DualSysSwapAndSync().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_DualSysSwapAndSync
(
    void
)
{
    le_result_t result = LE_FAULT;
    bool isResetRequested = false;
    bool isNvupApplyRequested = false;

    LE_INFO ("======== Test: le_fwupdate_DualSysSwapAndSync ========");

    /* Simulate unsupported API: API needs to return LE_UNSUPPORTED */
    /* Set returned error code for PA function: LE_UNSUPPORTED */
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSwapAndSync();
    /* Check required values */
    LE_ASSERT (result == LE_UNSUPPORTED );

    /* Simulate error: API needs to return LE_FAULT */
    /* Set returned error code for PA function: LE_FAULT */
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSwapAndSync ();
    /* Check required values */
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);

    /* Simulate swap acceptance: API needs to return LE_OK */
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Call the function to be tested */
    result = le_fwupdate_DualSysSwapAndSync ();
    /* Check required values */
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isResetRequested == true);
    LE_ASSERT (isNvupApplyRequested == true);
    /* Reset the reset flag */
    pa_fwupdateSimu_SetResetState();
    /* Reset the nvup apply flag */
    pa_fwupdateSimu_SetNvupApplyState();


    LE_INFO ("======== Test: le_fwupdate_DualSysSwapAndSync PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the SyncAtStartupCheck API
 *
 * API Tested:
 *  SyncAtStartupCheck().
 */
//--------------------------------------------------------------------------------------------------
static void TestSyncAtStartupCheck
(
    void
)
{
    pa_fwupdate_state_t state = PA_FWUPDATE_STATE_INVALID;

    LE_INFO ("======== Test: SyncAtStartupCheck TODO ========");

    /* simulate that a sync is not requested and pa_fwupdate_DualSysCheckSync returns LE_OK */
    /* Set that the sync request is not required */
    pa_fwupdateSimu_SetSyncState (false);
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the simulated SW update state value: PA_FWUPDATE_STATE_INVALID */
    pa_fwupdateSimu_setSwUpdateState (PA_FWUPDATE_STATE_INVALID);
    /* Call the function to be tested */
    /* SyncAtStartupCheck() is a static function so we call the COMPONENT_INIT which is calling it */
    _fwupdateComp_COMPONENT_INIT();
    /* Get the the simulated SW update value */
    pa_fwupdateSimu_GetSwUpdateState (&state);
    /* Check that the SW update state wasn't updated meaning that a SYNC operation was not called */
    LE_ASSERT (state == PA_FWUPDATE_STATE_INVALID);

    /* simulate that a sync is requested and pa_fwupdate_DualSysCheckSync returns LE_OK */
    /* Set that the sync request is required */
    pa_fwupdateSimu_SetSyncState (true);
    /* Set returned error code for PA function: LE_OK */
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    /* Set the simulated SW update state value: PA_FWUPDATE_STATE_INVALID */
    pa_fwupdate_SetState (PA_FWUPDATE_STATE_INVALID);
    /* Call the function to be tested */
    /* SyncAtStartupCheck() is a static function so we call the COMPONENT_INIT which is calling it */
    _fwupdateComp_COMPONENT_INIT();
    /* Get the the simulated SW update value */
    pa_fwupdateSimu_GetSwUpdateState (&state);
    /* Check that the SW update state was updated meaning that a SYNC operation was called */
    LE_ASSERT (state == PA_FWUPDATE_STATE_SYNC);

    LE_INFO ("======== Test: SyncAtStartupCheck PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * Each Test called once.
 *  - DownloadTestTest()
 *  - ..
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    le_log_SetFilterLevel(LE_LOG_DEBUG);

    LE_INFO ("======== Start UnitTest of FW update ========");
    LE_INFO ("======== Start UnitTest for public functions ========");
    /* Public APIs to be tested:
     * le_fwupdate_Download
     * le_fwupdate_DualSysSyncState
     * le_fwupdate_DualSysSwap
     * le_fwupdate_DualSysSync
     * le_fwupdate_DualSysSwapAndSync
     * SyncAtStartupCheck
     *
     * Not linked to dual system:
     * le_fwupdate_GetFirmwareVersion
     * le_fwupdate_GetBootloaderVersion
     */
    Testle_fwupdate_Download();
    Testle_fwupdate_DualSysSyncState();
    Testle_fwupdate_DualSysSwap();
    Testle_fwupdate_DualSysSync();
    Testle_fwupdate_DualSysSwapAndSync();
    TestSyncAtStartupCheck();
    Testle_fwupdate_GetFirmwareVersion();
    Testle_fwupdate_GetBootloaderVersion();


    LE_INFO ("======== Test FW update implementation Tests SUCCESS ========");
    exit(0);
}

