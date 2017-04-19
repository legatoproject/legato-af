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
void le_fwupdate_Init
(
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
    le_result_t result;
    int fd;

    LE_INFO ("======== Test: le_fwupdate_Download ========");

    // Test invalid file descriptor: API needs to return LE_BAD_PARAMETER
    fd = -1;
    // Indicate that systems are synchronized
    pa_fwupdateSimu_SetSyncState (true);
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
    LE_ASSERT (result == LE_BAD_PARAMETER);

    // Test valid file descriptor and error on PA: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    fd = 1;
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
    LE_ASSERT (result == LE_FAULT);

    // Systems are not synchronized: API needs to return LE_NOT_POSSIBLE
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Set the synchronization state to false
    pa_fwupdateSimu_SetSyncState (false);
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
    LE_ASSERT (result == LE_NOT_POSSIBLE);

    // No data received during timeout: API needs to return LE_TIMEOUT
    // Set returned error code for PA function: LE_TIMEOUT
    pa_fwupdateSimu_SetReturnCode (LE_TIMEOUT);
    // Set the synchronization state to true
    pa_fwupdateSimu_SetSyncState (true);
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
    LE_ASSERT (result == LE_TIMEOUT);

    // Systems are not synchronized: API needs to return LE_UNAVAILABLE
    // Set returned error code for PA function: LE_UNAVAILABLE
    pa_fwupdateSimu_SetReturnCode (LE_UNAVAILABLE);
    // Set the synchronization state to false
    pa_fwupdateSimu_SetSyncState (true);
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
    LE_ASSERT (result == LE_UNAVAILABLE);

    // The file descriptor has been closed during download: API needs to return LE_CLOSED
    // Set returned error code for PA function: LE_CLOSED
    pa_fwupdateSimu_SetReturnCode (LE_CLOSED);
    // Set the synchronization state to true
    pa_fwupdateSimu_SetSyncState (true);
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
    LE_ASSERT (result == LE_CLOSED);

    // Valid treatment: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Set the synchronization state to true
    pa_fwupdateSimu_SetSyncState (true);
    // Call the function to be tested
    result = le_fwupdate_Download (fd);
    // Check required values
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
static void Testle_fwupdate_GetFirmwareVersion
(
    void
)
{
    le_result_t result;
    char Version[20];
    LE_INFO ("======== Test: le_fwupdate_GetFirmwareVersion ========");

    /* Test LE_NOT_FOUND error if the version string is not available:
     * API needs to return LE_NOT_FOUND
     */
    // Set returned error code for PA function: LE_NOT_FOUND
    pa_fwupdateSimu_SetReturnCode (LE_NOT_FOUND);
    // Call the function to be tested
    result = le_fwupdate_GetFirmwareVersion (Version, 20);
    // Check required values
    LE_ASSERT (result == LE_NOT_FOUND);

    /* Test LE_OVERFLOW error if version string to big to fit in provided buffer:
     * API needs to return LE_OVERFLOW
     */
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_GetFirmwareVersion (Version, 2);
    // Check required values
    LE_ASSERT (result == LE_OVERFLOW);

    // Test LE_FAULT error for any other errors : API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    // Call the function to be tested
    result = le_fwupdate_GetFirmwareVersion (Version, 20);
    // Check required values
    LE_ASSERT (result == LE_FAULT);

    // Test correct behavior: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_GetFirmwareVersion (Version, 20);
    // Check required values
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
static void Testle_fwupdate_GetBootloaderVersion
(
    void
)
{
    le_result_t result;
    char Version[20];
    LE_INFO ("======== Test: le_fwupdate_GetBootloaderVersion ========");

    // Test errors
    /* Test LE_NOT_FOUND error if the version string is not available:
     * API needs to return LE_NOT_FOUND
     */
    // Set returned error code for PA function: LE_NOT_FOUND
    pa_fwupdateSimu_SetReturnCode (LE_NOT_FOUND);
    // Call the function to be tested
    result = le_fwupdate_GetBootloaderVersion (Version, 20);
    // Check required values
    LE_ASSERT (result == LE_NOT_FOUND);

    /* Test LE_OVERFLOW error if version string to big to fit in provided buffer:
     * API needs to return LE_OVERFLOW
     */
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_GetBootloaderVersion (Version, 2);
    // Check required values
    LE_ASSERT (result == LE_OVERFLOW);

    // Test LE_FAULT error for any other errors: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    // Call the function to be tested
    result = le_fwupdate_GetBootloaderVersion (Version, 20);
    // Check required values
    LE_ASSERT (result == LE_FAULT);

    // Test correct behavior: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_GetBootloaderVersion (Version, 20);
    // Check required values
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (0 == strncmp(Version, BOOT_VERSION_UT, strlen (BOOT_VERSION_UT)));

    LE_INFO ("======== Test: le_fwupdate_GetBootloaderVersion PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_IsSystemMarkedGood API
 *
 * API Tested:
 *  le_fwupdate_IsSystemMarkedGood().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_IsSystemMarkedGood
(
    void
)
{
    le_result_t result;
    bool isSystemGood = false;

    LE_INFO ("======== Test: le_fwupdate_IsSystemMarkedGood ========");
    // Simulate unsupported API: API needs to return LE_UNSUPPORTED
    // Set returned error code for PA function: LE_UNSUPPORTED
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    // Call the function to be tested
    result = le_fwupdate_IsSystemMarkedGood (&isSystemGood);
    // Check required values
    LE_ASSERT (result == LE_UNSUPPORTED);

    // Simulate unsynchronized systems: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Set the synchronization state to false
    pa_fwupdateSimu_SetSyncState (false);
    // Call the function to be tested
    result = le_fwupdate_IsSystemMarkedGood (&isSystemGood);
    // Check required values
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSystemGood == false);

    // Simulate synchronized systems: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Set the synchronization state to true
    pa_fwupdateSimu_SetSyncState (true);
    // Call the function to be tested
    result = le_fwupdate_IsSystemMarkedGood (&isSystemGood);
    // Check required values
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSystemGood == true);

    LE_INFO ("======== Test: le_fwupdate_IsSystemMarkedGood PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_Install API
 *
 * API Tested:
 *  le_fwupdate_Install().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_Install
(
    void
)
{
    le_result_t result;
    bool isResetRequested = false;
    bool isNvupApplyRequested = false;

    LE_INFO ("======== Test: le_fwupdate_Install ========");

    // Simulate unsupported API: API needs to return LE_UNSUPPORTED
    // Set returned error code for PA function: LE_UNSUPPORTED
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    // Call the function to be tested
    result = le_fwupdate_Install ();
    // Check required values
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_UNSUPPORTED);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);

    // Simulate error: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    // Call the function to be tested
    result = le_fwupdate_Install ();
    // Check required values
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);

    // Simulate swap busy: API needs to return LE_BUSY
    // Set returned error code for PA function: LE_BUSY
    pa_fwupdateSimu_SetReturnCode (LE_BUSY);
    // Call the function to be tested
    result = le_fwupdate_Install ();
    // Check required values
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_BUSY);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);
    // Reset the reset flag
    pa_fwupdateSimu_SetResetState();
    // Reset the nvup apply flag
    pa_fwupdateSimu_SetNvupApplyState();

    // Simulate swap acceptance: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_Install ();
    // Check required values
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isResetRequested == true);
    LE_ASSERT (isNvupApplyRequested == true);
    // Reset the reset flag
    pa_fwupdateSimu_SetResetState();
    // Reset the nvup apply flag
    pa_fwupdateSimu_SetNvupApplyState();

    LE_INFO ("======== Test: le_fwupdate_Install PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_MarkGood API
 *
 * API Tested:
 *  le_fwupdate_Sync().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_MarkGood
(
    void
)
{
    le_result_t result;
    bool isSystemGood = false;
    pa_fwupdate_state_t state = PA_FWUPDATE_STATE_INVALID;

    LE_INFO ("======== Test: le_fwupdate_MarkGood ========");

    // Simulate unsupported API: API needs to return LE_UNSUPPORTED
    // Set returned error code for PA function: LE_UNSUPPORTED
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    // Call the function to be tested
    result = le_fwupdate_MarkGood();
    // Check required values
    LE_ASSERT (result == LE_UNSUPPORTED);

    // Simulate error: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    // Call the function to be tested
    result = le_fwupdate_MarkGood();
    // Check required values
    LE_ASSERT (result == LE_FAULT);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_NORMAL);

    // Simulate error: API needs to return LE_UNAVAILABLE
    // Set returned error code for PA function: LE_UNAVAILABLE
    pa_fwupdateSimu_SetReturnCode (LE_UNAVAILABLE);
    // Call the function to be tested
    result = le_fwupdate_MarkGood();
    // Check required values
    LE_ASSERT (result == LE_UNAVAILABLE);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_NORMAL);

    // Simulate sync acceptance: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Set the synchronization state to false
    pa_fwupdateSimu_SetSyncState (false);
    // Call the function to be tested
    result = le_fwupdate_MarkGood();
    // Check required values
    LE_ASSERT (result == LE_OK);
    result = pa_fwupdate_GetSystemState (&isSystemGood);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSystemGood == true);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_SYNC);
    // reset values
    pa_fwupdateSimu_SetSyncState(false);
    pa_fwupdateSimu_SetSwUpdateState(PA_FWUPDATE_STATE_NORMAL);

    LE_INFO ("======== Test: le_fwupdate_Sync PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_InstallAndMarkGood API
 *
 * API Tested:
 *  le_fwupdate_InstallAndMarkGood().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_InstallAndMarkGood
(
    void
)
{
    le_result_t result;
    bool isResetRequested = false;
    bool isNvupApplyRequested = false;
    bool isSystemGood = false;
    pa_fwupdate_state_t state = PA_FWUPDATE_STATE_INVALID;

    LE_INFO ("======== Test: le_fwupdate_InstallAndMarkGood ========");

    // Simulate unsupported API: API needs to return LE_UNSUPPORTED
    // Set returned error code for PA function: LE_UNSUPPORTED
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    // Call the function to be tested
    result = le_fwupdate_InstallAndMarkGood();
    // Check required values
    LE_ASSERT (result == LE_UNSUPPORTED);

    // Simulate error: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    // Call the function to be tested
    result = le_fwupdate_InstallAndMarkGood ();
    // Check required values
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (isResetRequested == false);
    LE_ASSERT (isNvupApplyRequested == false);
    result = pa_fwupdate_GetSystemState (&isSystemGood);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (isSystemGood == false);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_NORMAL);

    // Simulate swap acceptance: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_InstallAndMarkGood ();
    // Check required values
    pa_fwupdateSimu_GetResetState(&isResetRequested);
    pa_fwupdateSimu_GetNvupApplyState(&isNvupApplyRequested);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isResetRequested == true);
    LE_ASSERT (isNvupApplyRequested == true);
    result = pa_fwupdate_GetSystemState (&isSystemGood);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isSystemGood == true);
    pa_fwupdateSimu_GetSwUpdateState (&state);
    LE_ASSERT (state == PA_FWUPDATE_STATE_SYNC);
    // Reset the reset flag
    pa_fwupdateSimu_SetResetState();
    // Reset the nvup apply flag
    pa_fwupdateSimu_SetNvupApplyState();

    LE_INFO ("======== Test: le_fwupdate_InstallAndMarkGood PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_InitDownload API
 *
 * API Tested:
 *  le_fwupdate_InitDownload().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_InitDownload
(
    void
)
{
    le_result_t result;
    bool isInitDownloadRequested = false;

    LE_INFO ("======== Test: le_fwupdate_InitDownload ========");

    // Simulate unsupported API: API needs to return LE_UNSUPPORTED
    // Set returned error code for PA function: LE_UNSUPPORTED
    pa_fwupdateSimu_SetReturnCode (LE_UNSUPPORTED);
    // Call the function to be tested
    result = le_fwupdate_InitDownload();
    // Check required values
    pa_fwupdateSimu_GetInitDownloadState(&isInitDownloadRequested);
    LE_ASSERT (result == LE_UNSUPPORTED );
    LE_ASSERT (isInitDownloadRequested == false);

    // Simulate error: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    // Call the function to be tested
    result = le_fwupdate_InitDownload ();
    // Check required values
    pa_fwupdateSimu_GetInitDownloadState(&isInitDownloadRequested);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (isInitDownloadRequested == false);

    // Simulate SYNC OK: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Set the sync state to true
    pa_fwupdateSimu_SetSyncState(true);
    // Call the function to be tested
    result = le_fwupdate_InitDownload ();
    // Check required values
    pa_fwupdateSimu_GetInitDownloadState(&isInitDownloadRequested);
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (isInitDownloadRequested == true);
    // Reset the init download flag
    pa_fwupdateSimu_SetInitDownloadState();

    LE_INFO ("======== Test: le_fwupdate_InitDownload PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * This test gets the le_fwupdate_GetResumePosition API
 *
 * API Tested:
 *  le_fwupdate_GetResumePosition().
 */
//--------------------------------------------------------------------------------------------------
static void Testle_fwupdate_GetResumePosition
(
    void
)
{
#define TEST_VALUE 0x55555555

    le_result_t result;
    size_t resumePosition = 0;

    LE_INFO ("======== Test: le_fwupdate_GetResumePosition ========");

    // Simulate unsupported API: API needs to return LE_FAULT
    // Set returned error code for PA function: LE_FAULT
    pa_fwupdateSimu_SetReturnCode (LE_FAULT);
    pa_fwupdateSimu_SetResumePosition(TEST_VALUE);
    // Call the function to be tested
    result = le_fwupdate_GetResumePosition(&resumePosition);
    LE_ASSERT (result == LE_FAULT);
    LE_ASSERT (resumePosition == 0);

    // Simulate error: API needs to return LE_BAD_PARAMETER
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_GetResumePosition(NULL);
    // Check required values
    LE_ASSERT (result == LE_BAD_PARAMETER);
    LE_ASSERT (resumePosition == 0);

    // Simulate swap acceptance: API needs to return LE_OK
    // Set returned error code for PA function: LE_OK
    pa_fwupdateSimu_SetReturnCode (LE_OK);
    // Call the function to be tested
    result = le_fwupdate_GetResumePosition(&resumePosition);
    // Check required values
    LE_ASSERT (result == LE_OK);
    LE_ASSERT (resumePosition == TEST_VALUE);

    LE_INFO ("======== Test: le_fwupdate_GetResumePosition PASSED ========");
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
     * le_fwupdate_IsSystemMarkedGood
     * le_fwupdate_Install
     * le_fwupdate_MarkGood
     * le_fwupdate_InstallAndMarkGood
     *
     * Not linked to dual system:
     * le_fwupdate_GetFirmwareVersion
     * le_fwupdate_GetBootloaderVersion
     */
    Testle_fwupdate_Download();
    Testle_fwupdate_IsSystemMarkedGood();
    Testle_fwupdate_Install();
    Testle_fwupdate_MarkGood();
    Testle_fwupdate_InstallAndMarkGood();
    Testle_fwupdate_GetFirmwareVersion();
    Testle_fwupdate_GetBootloaderVersion();
    Testle_fwupdate_InitDownload();
    Testle_fwupdate_GetResumePosition();


    LE_INFO ("======== Test FW update implementation Tests SUCCESS ========");
    exit(0);
}

