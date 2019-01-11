//--------------------------------------------------------------------------------------------------
/**
 * @file updateUnpack.c
 *
 * Implementation of the Update Pack parser.  This file parses an update pack, and drives the
 * rest of the update based on the contents of the update pack.
 *
 * This is single-threaded, event-driven code that shares the main thread's event loop.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "updateUnpack.h"
#include "pipeline.h"
#include "fileDescriptor.h"
#include "system.h"
#include "app.h"


/// An MD5 hash string is 32 characters long, plus a null terminator.
#define MD5_STRING_BYTES 33

/// File descriptor to read the update pack from.
static int InputFd = -1;

/// InputFd could have been closed by the legato messaging infrastructure; and in that case it
/// shouldn't be closed again.
static bool InputFdClosed = false;

/// Reference to the FD Monitor for the input stream (NULL if not unpacking).
static le_fdMonitor_Ref_t InputFdMonitor = NULL;

/// Reference to an unpack pipeline (NULL if not unpacking).
static pipeline_Ref_t Pipeline = NULL;

/// File descriptor connected to the input of a pipeline (-1 if not unpacking)
static int PipelineFd = -1;

/// Function to be called to report progress.
static updateUnpack_ProgressHandler_t ProgressFunc = NULL;

/// The command name found in the JSON update pack section header.
static char Command[32];

/// What type of update pack is it?
static updateUnpack_Type_t Type = TYPE_UNKNOWN;

/// The name of the app being updated/removed.
static char AppName[LIMIT_MAX_APP_NAME_BYTES];

/// The MD5 hash obtained from a JSON header.
static char Md5[MD5_STRING_BYTES]; ///< The system's MD5 hash.

/// # of bytes of payload following the JSON.
static size_t PayloadSize;

/// # of bytes of payload that have been copied to the unpack pipeline.
static size_t PayloadBytesCopied;

/// Percentage complete on current task.
static unsigned int PercentDone;


//--------------------------------------------------------------------------------------------------
/**
 * The state machine looks like this:
 *
 * @verbatim

  +---------------<--------------+
  |                              |
  +------<------+                |
  |             |                |
  v             |                |
IDLE --> PARSING_JSON --> UNPACKING/SKIPPING_PAYLOAD
                ^                |
                |                |
                +-------<--------+
@endverbatim
 *
 * The transition from PARSING_JSON to UNPACKING_PAYLOAD happens when the JSON header for a section
 * of the update pack has been successfully parsed and all required fields have been extracted
 * from it.
 *
 * The transition from UNPACKING_PAYLOAD to IDLE happens when the payload has been extracted
 * to the file system.
 *
 * If there is no payload for a section, then the state machine skips
 * UNPACKING_PAYLOAD and goes back to IDLE from PARSING_JSON.
 *
 * The transition back from UNPACKING_PAYLOAD to PARSING_JSON happens whenever more JSON data is
 * found after the end of the section that was being applied.
 *
 * The state machine starts in the IDLE state and returns to the IDLE state whenever an update
 * pack is successfully installed or an error occurs.
 *
 * If a system update pack contains an app that is already installed on the target, then the
 * payload bytes are read from the input stream and discarded.  In this case, the SKIPPING_PAYLOAD
 * state replaces the UNPACKING_PAYLOAD state.
 */
//--------------------------------------------------------------------------------------------------
static enum
{
    STATE_IDLE,
    STATE_PARSING_JSON,
    STATE_UNPACKING_PAYLOAD,
    STATE_SKIPPING_PAYLOAD
}
State = STATE_IDLE;


//--------------------------------------------------------------------------------------------------
/**
 * JSON parsing session.  Used to stop parsing early, if necessary.
 *
 * Only valid in the PARSING_JSON state.
 */
//--------------------------------------------------------------------------------------------------
static le_json_ParsingSessionRef_t ParsingSession = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Delete the FD Monitor object.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFdMonitor
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (InputFdMonitor != NULL)
    {
        le_fdMonitor_Delete(InputFdMonitor);
        InputFdMonitor = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Reset the update unpacker.
 */
//--------------------------------------------------------------------------------------------------
static void Reset
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Reset the state machine.
    State = STATE_IDLE;

    // Stop JSON parsing.
    if (ParsingSession != NULL)
    {
        le_json_Cleanup(ParsingSession);
        ParsingSession = NULL;
    }

    DeleteFdMonitor();

    // Close the pipes.
    if (InputFd != -1)
    {
        // Close InputFd only if it hasn't been closed by the Legato messaging infrastructure.
        if (InputFdClosed == false)
        {
            fd_Close(InputFd);
        }

        InputFd = -1;
    }
    if (PipelineFd != -1)
    {
        fd_Close(PipelineFd);
        PipelineFd = -1;
    }

    // Delete the pipeline.
    if (Pipeline != NULL)
    {
        pipeline_Delete(Pipeline);
        Pipeline = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Report progress.
 */
//--------------------------------------------------------------------------------------------------
static void ReportProgress
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Percentage complete and state on previous task.
    static unsigned int previousPercentDone;
    static unsigned int previousState;

    // Return without updating if there is no change in the percent complete and state.
    if ((previousPercentDone == PercentDone) &&
        (previousState == State))
    {
        return;
    }

    // Don't report progress if skipping payload.
    if (State != STATE_SKIPPING_PAYLOAD)
    {
        ProgressFunc(UPDATE_UNPACK_STATUS_UNPACKING, PercentDone);
    }

    // Remember the state and percent done so we can throttle the reports (see above).
    previousState = State;
    previousPercentDone = PercentDone;
}


//--------------------------------------------------------------------------------------------------
/**
 * Error handler for problems with the update pack format.
 */
//--------------------------------------------------------------------------------------------------
static void HandleFormatError
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    Reset();

    // Report the error back to the client and terminate the update.
    ProgressFunc(UPDATE_UNPACK_STATUS_BAD_PACKAGE, PercentDone);
}


//--------------------------------------------------------------------------------------------------
/**
 * Error handler for problems with reading or processing the update pack.
 */
//--------------------------------------------------------------------------------------------------
static void HandleInternalError
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    Reset();

    // Report the error back to the client and terminate the update.
    ProgressFunc(UPDATE_UNPACK_STATUS_INTERNAL_ERROR, PercentDone);
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when app unpack finishes successfully.
 */
//--------------------------------------------------------------------------------------------------
static void AppUnpackDone
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char buf[1];
    //Check whether InputFd reaches EOF, if not it is an error condition.
    if (fd_ReadSize(InputFd, buf, sizeof(buf)) != 0)
    {
        LE_ERROR("Malformed update pack. Only one app update/remove allowed per update pack.");
        HandleFormatError();
    }
    else
    {
        PercentDone = 100;
        ReportProgress();
        // As we allow single app, notify that unpack is done.
        ProgressFunc(UPDATE_UNPACK_STATUS_DONE, 100);
        Reset();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Error handling function called by the JSON parser when an error occurs.
 */
//--------------------------------------------------------------------------------------------------
static void JsonErrorHandler
(
    le_json_Error_t error,
    const char* msg     ///< Human-readable error message.
)
//--------------------------------------------------------------------------------------------------
{
    switch (error)
    {
        case LE_JSON_SYNTAX_ERROR:

            LE_ERROR("Malformed update pack (%s)", msg);
            HandleFormatError();
            break;

        case LE_JSON_READ_ERROR:

            // If a read error happens when we haven't even read the opening '{', then
            // this is probably just the end of the file.  But if it happens in the middle
            // of the JSON, or before we've figured out what type of update pack this
            // is (i.e., if it's an empty update pack) then it's an error.
            if ((le_json_GetContextType() != LE_JSON_CONTEXT_DOC) || (Type == TYPE_UNKNOWN))
            {
                LE_ERROR("Error reading update pack (%s)", msg);
                HandleInternalError();
            }
            // When we hit the end of a system update, we need to finish applying the
            // system update.
            else if (Type == TYPE_SYSTEM_UPDATE)
            {
                ProgressFunc(UPDATE_UNPACK_STATUS_DONE, 100);

                Reset();
            }
            else
            {
                // Only system update should invoke this function, all other update type is considered
                // as error.
                LE_CRIT("Unexpected update type: %d.", Type);
                HandleInternalError();
            }
            break;
    }
}


static void JsonEventHandler(le_json_Event_t event);

//--------------------------------------------------------------------------------------------------
/**
 * Start parsing a JSON header.
 */
//--------------------------------------------------------------------------------------------------
static void StartParsing
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Reset the JSON header info variables.
    Command[0] = '\0';
    AppName[0] = '\0';
    Md5[0] = '\0';
    PayloadSize = 0;

    // Set the state
    State = STATE_PARSING_JSON;

    // Start the parser (and wait for callbacks).
    ParsingSession = le_json_Parse(InputFd, JsonEventHandler, JsonErrorHandler, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Completion callback for "tar xj" operation.
 */
//--------------------------------------------------------------------------------------------------
static void UntarDone
(
    pipeline_Ref_t pipeline,
    int status
)
//--------------------------------------------------------------------------------------------------
{
    pipeline_Delete(Pipeline);
    Pipeline = NULL;

    if (!WIFEXITED(status) || (WEXITSTATUS(status) != EXIT_SUCCESS))
    {
        if (WIFEXITED(status))
        {
            LE_ERROR("Payload unpack pipeline failed with exit code: %d", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            LE_ERROR("Payload unpack pipeline killed by signal: %d", WTERMSIG(status));
        }
        else
        {
            LE_ERROR("Payload unpack pipeline died for unknown reason (status: %d)", status);
        }

        HandleInternalError();
        return;
    }

    // If this update pack contains changes to individual apps,
    if (Type == TYPE_APP_UPDATE)
    {
        AppUnpackDone();
    }

    // If this update pack contains a system update,
    else
    {
        LE_ASSERT(Type == TYPE_SYSTEM_UPDATE);

        // After the unpack of the system part, we make space for all the app changes
        // by removing all the apps that aren't needed by the new system or any of the old
        // systems.
        if (strcmp(Command, "updateSystem") == 0)
        {
            system_RemoveUnusedApps();
        }
        // After the unpack of one of the apps, we rename the app to the appropriate location
        // and copy over any writable files that may have been inherited from an earlier version
        // of the same app in the previous system.
        else
        {
            PercentDone = 100;
            ReportProgress();
        }

        // There could be more after this payload, so look for another JSON header.
        StartParsing();
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Completion callback for skip forward operation that is done instead of an app unpack + install
 * in the case that an app is already installed.
 */
//--------------------------------------------------------------------------------------------------
static void SkipForwardDone
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    PercentDone = 100;
    ReportProgress();

    // Even if we skip the payload we still need to process the install.
    if (Type == TYPE_APP_UPDATE)
    {
        AppUnpackDone();
    }
    else if (Type == TYPE_SYSTEM_UPDATE)
    {
        // There could be more after this payload, so look for another JSON header.
        StartParsing();
    }
    else
    {
        LE_CRIT("Unexpected update type: %d.", Type);
        HandleInternalError();
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Copy bytes from the input fd to the pipeline's input fd until the input fd's read buffer is
 * empty or we have copied all the payload bytes.
 */
//--------------------------------------------------------------------------------------------------
static void CopyBytesToPipeline
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char buffer[1024];

    // Keep copying as much as we can until we've copied all the payload.
    while (PayloadBytesCopied < PayloadSize)
    {
        // Compute the number of bytes to read.
        size_t bytesToRead = PayloadSize - PayloadBytesCopied;
        if (bytesToRead > sizeof(buffer))
        {
            bytesToRead = sizeof(buffer);
        }

        // Read the bytes, retrying if interrupted by a signal.
        ssize_t readResult;
        do
        {
            readResult = read(InputFd, buffer, bytesToRead);
        }
        while ((readResult == -1) && (errno == EINTR));

        // Handle errors
        if (readResult == -1)
        {
            // EWOULDBLOCK indicates that there are currently no more bytes available to be
            // read from the fd, but more will probably become available later.
            if (errno == EWOULDBLOCK)
            {
                // Break out of the loop and let the FD Monitor call us back when there's
                // more to read.
                break;
            }

            LE_ERROR("Failed to read from input stream (%m).");
            goto error;
        }

        // Handle end of file.
        if (readResult == 0)
        {
            LE_ERROR("Unexpected early end of input after %zu bytes of %zu.",
                     PayloadBytesCopied,
                     PayloadSize);
            goto error;
        }

        // Write the bytes that we read.
        ssize_t bytesWritten = 0;
        ssize_t writeResult;
        do
        {
            writeResult = write(PipelineFd, buffer + bytesWritten, readResult - bytesWritten);

            // If some bytes were written, remember how many bytes, so we don't try to write the
            // same bytes again if we have more to write.
            if (writeResult > 0)
            {
                bytesWritten += writeResult;
            }
        }
        while (   ((writeResult == -1) && (errno == EINTR)) // Retry if interrupted by a signal
               || ((writeResult != -1) && (bytesWritten < readResult))  ); // Continue if not done

        // Check for errors.
        if (writeResult == -1)
        {
            LE_ERROR("Failed to write to output stream (%m)");
            goto error;
        }

        // Update the static progress variables and report progress to the client.
        PayloadBytesCopied += readResult;
        PercentDone = (100 * PayloadBytesCopied) / PayloadSize;
        ReportProgress();
    }

    // If we have copied all the payload bytes to the pipeline's input, then we can stop
    // monitoring the input fd now, close the pipeline input write pipe, and wait for the pipeline
    // completion callback (UntarDone()).
    LE_INFO("Payload copied: %zu/%zu", PayloadBytesCopied, PayloadSize);
    LE_ASSERT(PayloadBytesCopied <= PayloadSize);
    if (PayloadBytesCopied == PayloadSize)
    {
        DeleteFdMonitor();
        fd_Close(PipelineFd);
        PipelineFd = -1;
    }
    return;

error:

    HandleInternalError();
}


//--------------------------------------------------------------------------------------------------
/**
 * Read and discard payload bytes from the input stream until we have discarded all the payload
 * bytes or the input stream doesn't have any more bytes for us right now.
 **/
//--------------------------------------------------------------------------------------------------
static void DiscardPayloadBytes
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char buffer[1024];

    // Keep reading as much as we can until we've read all the payload.
    while (PayloadBytesCopied < PayloadSize)
    {
        // Compute the number of bytes to read.
        size_t bytesToRead = PayloadSize - PayloadBytesCopied;
        if (bytesToRead > sizeof(buffer))
        {
            bytesToRead = sizeof(buffer);
        }

        // Read the bytes, retrying if interrupted by a signal.
        ssize_t readResult;
        do
        {
            readResult = read(InputFd, buffer, bytesToRead);
        }
        while ((readResult == -1) && (errno == EINTR));

        // Handle errors
        if (readResult == -1)
        {
            // EWOULDBLOCK indicates that there are currently no more bytes available to be
            // read from the fd, but more will probably become available later.
            if (errno == EWOULDBLOCK)
            {
                // Break out of the loop and let the FD Monitor call us back when there's
                // more to read.
                break;
            }

            LE_ERROR("Failed to read from input stream (%m).");
            HandleInternalError();
            break;
        }

        if (0 == readResult)
        {
            LE_ERROR("Unexpected early end of input after %zu bytes of %zu.",
                     PayloadBytesCopied,
                     PayloadSize);
            HandleInternalError();
            break;
        }

        // Update the static progress variables and report progress to the client.
        PayloadBytesCopied += readResult;
        PercentDone = (100 * PayloadBytesCopied) / PayloadSize;
        ReportProgress();
    }

    // If we have read all the payload bytes, then we can stop monitoring the input fd for now
    // and wrap up this app.
    LE_INFO("Payload discarded: %zu/%zu", PayloadBytesCopied, PayloadSize);
    LE_ASSERT(PayloadBytesCopied <= PayloadSize);
    if (PayloadBytesCopied == PayloadSize)
    {
        DeleteFdMonitor();
        SkipForwardDone();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler for the input fd when copying bytes to a pipeline.
 */
//--------------------------------------------------------------------------------------------------
static void InputFdEventHandler
(
    int fd,
    short events
)
//--------------------------------------------------------------------------------------------------
{
    if (events & POLLIN)
    {
        if (State == STATE_UNPACKING_PAYLOAD)
        {
            CopyBytesToPipeline();
        }
        else if (State == STATE_SKIPPING_PAYLOAD)
        {
            DiscardPayloadBytes();
        }
        else
        {
            LE_CRIT("Unexpected state %d.", State);
        }
    }
    else
    {
        LE_ERROR("Error on read file descriptor.");
        HandleInternalError();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that runs in the unpack pipeline's "tar" process.
 **/
//--------------------------------------------------------------------------------------------------
static int Untar
(
    void* param
)
//--------------------------------------------------------------------------------------------------
{
    const char* unpackDir = param;

    // Close all open file descriptors except for stdin, stdout, and stderr.
    // This ensures that we don't keep copies of things like the pipeline input write pipe open.
    fd_CloseAllNonStd();

    // Try bsdtar first.  If that fails, fallback to tar.
    execl("/usr/bin/bsdtar", "bsdtar", "xjmop", "-f", "-", "-C", unpackDir, (char*)NULL);
    execl("/bin/tar", "tar", "xjop", "-C", unpackDir, (char*)NULL);

    LE_FATAL("Failed to exec tar (%m)");
}


//--------------------------------------------------------------------------------------------------
/**
 * Start unpacking a tarball.
 */
//--------------------------------------------------------------------------------------------------
static void StartUntar
(
    const char* dirPath ///< Path to the directory to unpack the tarball into.
)
//--------------------------------------------------------------------------------------------------
{
    State = STATE_UNPACKING_PAYLOAD;

    PayloadBytesCopied = 0;

    // Create a pipeline: PipelineFd -> tar
    Pipeline = pipeline_Create();
    PipelineFd = pipeline_CreateInputPipe(Pipeline);
    pipeline_Append(Pipeline, Untar, (void*)dirPath);
    pipeline_Start(Pipeline, UntarDone);

    fd_SetNonBlocking(InputFd);

    // Create FD Monitor for the Input FD.
    InputFdMonitor = le_fdMonitor_Create("unpack", InputFd, InputFdEventHandler, POLLIN);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start reading and throwing away payload bytes from the input stream.
 */
//--------------------------------------------------------------------------------------------------
static void StartSkipForward
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    State = STATE_SKIPPING_PAYLOAD;

    PayloadBytesCopied = 0;

    fd_SetNonBlocking(InputFd);

    // Create FD Monitor for the Input FD.
    InputFdMonitor = le_fdMonitor_Create("skip", InputFd, InputFdEventHandler, POLLIN);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do a firmware update.
 */
//--------------------------------------------------------------------------------------------------
static void StartFirmwareUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    PayloadBytesCopied = 0;

    LE_INFO("Starting firmware update.");

    if (le_fwupdate_TryConnectService() != LE_OK)
    {
        LE_ERROR("Unable to connect to fwupdate service.");
        HandleInternalError();
        return;
    }

    le_result_t result = le_fwupdate_Download(InputFd);
    // le_fwupdate_Download would close InputFd, so it shouldn't be closed again.
    InputFdClosed = true;

    if ( result == LE_OK )
    {
        LE_INFO("Firmware update download successful. Waiting for modem to reset.");

        ProgressFunc(UPDATE_UNPACK_STATUS_DONE, 100);
        Reset();
    }
    else
    {
        LE_ERROR("Firmware update download failed.");

        HandleInternalError();
    }

    le_fwupdate_DisconnectService();
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle the end of a JSON header.
 */
//--------------------------------------------------------------------------------------------------
static void JsonDone
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(Command, "updateSystem") == 0)
    {
        // System update header MUST be the first thing in a system update pack.
        // So, we shouldn't have seen any type of JSON header yet.
        if (Type != TYPE_UNKNOWN)
        {
            LE_ERROR("Malformed update pack (system update can't be mixed with other types)");
            HandleFormatError();
        }
        // Required fields are "md5" and "size"
        else if (Md5[0] == '\0')
        {
            LE_ERROR("Malformed update pack (system's MD5 hash missing)");
            HandleFormatError();
        }
        else if (PayloadSize == 0)
        {
            LE_ERROR("Malformed update pack (system update payload missing)");
            HandleFormatError();
        }
        // If everything looks good...
        else
        {
            Type = TYPE_SYSTEM_UPDATE;
            State = STATE_UNPACKING_PAYLOAD;

            // Make space by removing extra systems.
            system_RemoveUnneeded();

            // Delete any old unpack junk from previous incomplete/failed updates.
            system_PrepUnpackDir();

            // Unpack the system tarball.
            // This is asynchronous and will call UntarDone() when finished.
            StartUntar(system_UnpackPath);
        }
    }
    else if (strcmp(Command, "updateApp") == 0)
    {
        if (Type == TYPE_FIRMWARE_UPDATE)
        {
            LE_ERROR("Malformed update pack (app update can't be mixed with firmware update)");
            HandleFormatError();
        }
        // Required fields are "name", "md5" and "size".
        else if (Md5[0] == '\0')
        {
            LE_ERROR("Malformed update pack (app's MD5 hash missing from app update section)");
            HandleFormatError();
        }
        else if (AppName[0] == '\0')
        {
            LE_ERROR("Malformed update pack (app name missing from app update section)");
            HandleFormatError();
        }
        else if (PayloadSize == 0)
        {
            LE_ERROR("Malformed update pack (app update payload missing)");
            HandleFormatError();
        }
        else
        {
            if (Type == TYPE_UNKNOWN)
            {
                Type = TYPE_APP_UPDATE;

                // Make space by removing extra systems.
                system_RemoveUnneeded();

                // Make space by removing unneeded apps.
                system_RemoveUnusedApps();
            }

            if (app_Exists(Md5) == false)
            {
                LE_INFO("App with MD5 sum %s being unpacked.", Md5);

                State = STATE_UNPACKING_PAYLOAD;

                if (Type == TYPE_APP_UPDATE)
                {
                    // UnpackPath = appUnpack_Path
                    // Prepare the directory to unpack into.
                    app_PrepUnpackDir();
                    // Unpack the app tarball.
                    // This is asynchronous and will call UntarDone() when finished.
                    StartUntar(app_UnpackPath);
                }
                else
                {
                    char unpackPath[LIMIT_MAX_PATH_BYTES] = "";
                    // UnpackPath = app_unpack+Md5 hash
                    le_path_Concat("/", unpackPath, sizeof(unpackPath), app_UnpackPath, Md5, NULL);
                    LE_FATAL_IF(le_dir_RemoveRecursive(unpackPath) != LE_OK,
                                "Failed to recursively delete '%s'.",
                                unpackPath);
                    // This is system update. Create a directory in /legato/apps/unpack/<Md5>
                    LE_FATAL_IF(LE_OK != le_dir_MakePath(unpackPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH),
                                "Failed to create directory '%s'.",
                                unpackPath);
                    // Untar the app tarball. Will call UntarDone() when finished.
                    StartUntar(unpackPath);
                }

            }
            else
            {
                LE_INFO("App with MD5 sum %s already exists on target. Skipping.", Md5);

                // Read all the payload bytes out of the input stream and throw them away.
                // This is asynchronous and will call SkipForwardDone() when finished.
                StartSkipForward();
            }
        }
    }
    else if (strcmp(Command, "removeApp") == 0)
    {
        if (Type == TYPE_FIRMWARE_UPDATE)
        {
            LE_ERROR("Malformed update pack (app remove can't be mixed with firmware update)");
            HandleFormatError();
        }
        // Only required field is "name".
        else if (AppName[0] == '\0')
        {
            LE_ERROR("Malformed update pack (app name missing from app remove section)");
            HandleFormatError();
        }
        else
        {
            if (Type == TYPE_UNKNOWN)
            {
                Type = TYPE_APP_REMOVE;

                // Make space by removing extra systems and apps.
                system_RemoveUnneeded();
                system_RemoveUnusedApps();
            }

            AppUnpackDone();
        }
    }
    else if (strcmp(Command, "updateFirmware") == 0)
    {
        if (Type != TYPE_UNKNOWN)
        {
            LE_ERROR("Malformed update pack (firmware update can't be mixed with other types)");
            HandleFormatError();
        }
        // "size" is the only required field.
        else if (PayloadSize == 0)
        {
            LE_ERROR("Malformed update pack (firmware update payload missing)");
            HandleFormatError();
        }
        else
        {
            Type = TYPE_FIRMWARE_UPDATE;
            State = STATE_UNPACKING_PAYLOAD;
            PercentDone = 0;
            ReportProgress();

            StartFirmwareUpdate();
        }
    }
    else if (Command[0] == '\0')
    {
        LE_ERROR("Malformed update pack (command missing)");
        HandleFormatError();
    }
    else
    {
        LE_ERROR("Malformed update pack (unrecognized command '%s')", Command);
        HandleFormatError();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * String member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void StringMemberEventHandler
(
    le_json_Event_t event,
    char* dest,     ///< Pointer to the buffer where the string should be stored.
    size_t size,    ///< Size of the buffer pointed to by dest (bytes).
    const char* memberName ///< Human-readable string naming the member for error messages.
)
//--------------------------------------------------------------------------------------------------
{
    if (event != LE_JSON_STRING)
    {
        LE_ERROR("Malformed update pack (expected %s to be a string; got %s).",
                 memberName,
                 le_json_GetEventName(event));
        HandleFormatError();
    }
    else if (LE_OVERFLOW == le_utf8_Copy(dest, le_json_GetString(), size, NULL))
    {
        LE_ERROR("Malformed update pack (%s too long).", memberName);
        HandleFormatError();
    }
    else
    {
        LE_DEBUG("%s: '%s'", memberName, dest);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * "command" member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void CommandEventHandler
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    StringMemberEventHandler(event, Command, sizeof(Command), "command");
}


//--------------------------------------------------------------------------------------------------
/**
 * "name" member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void NameEventHandler
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    StringMemberEventHandler(event, AppName, sizeof(AppName), "app name");
}


//--------------------------------------------------------------------------------------------------
/**
 * "md5" member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void Md5EventHandler
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    StringMemberEventHandler(event, Md5, sizeof(Md5), "MD5 hash");
}


//--------------------------------------------------------------------------------------------------
/**
 * "version" member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void VersionEventHandler
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    if (event != LE_JSON_STRING)
    {
        LE_ERROR("Malformed update pack (expected version to be a string; got %s).",
                 le_json_GetEventName(event));
        HandleFormatError();
    }
    // We don't actually need the version.
    else
    {
        char version[256];
        le_utf8_Copy(version, le_json_GetString(), sizeof(version), NULL);
        LE_DEBUG("Version: '%s'", version);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * "size" member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void SizeEventHandler
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    if (event != LE_JSON_NUMBER)
    {
        LE_ERROR("Malformed update pack (expected size to be a number; got %s).",
                 le_json_GetEventName(event));
        HandleFormatError();
    }
    else
    {
        double number = le_json_GetNumber();

        PayloadSize = (size_t)number;

        if (number != (double)PayloadSize)
        {
            LE_ERROR("Malformed update pack (invalid payload size: %f).", number);
            HandleFormatError();
        }
        else
        {
            LE_DEBUG("Size: '%zu'", PayloadSize);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called by the JSON parser when it encounters things during parsing.
 */
//--------------------------------------------------------------------------------------------------
static void JsonEventHandler
(
    le_json_Event_t event
)
//--------------------------------------------------------------------------------------------------
{
    // Ignore this unless in the PARSING_JSON state.
    if (State != STATE_PARSING_JSON)
    {
        return;
    }

    switch (event)
    {
        case LE_JSON_OBJECT_START:

            // We expect this at the start of the document.
            // Can be ignored.
            break;

        case LE_JSON_OBJECT_END:

            // Ignore this.
            break;

        case LE_JSON_DOC_END:

            // Confirm we have everything we need and move to the APPLYING state.
            JsonDone();
            break;

        case LE_JSON_OBJECT_MEMBER:
        {
            const char* memberName = le_json_GetString();

            if (strcmp(memberName, "command") == 0)
            {
                le_json_SetEventHandler(CommandEventHandler);
            }
            else if (strcmp(memberName, "md5") == 0)
            {
                le_json_SetEventHandler(Md5EventHandler);
            }
            else if (strcmp(memberName, "name") == 0)
            {
                le_json_SetEventHandler(NameEventHandler);
            }
            else if (strcmp(memberName, "version") == 0)
            {
                le_json_SetEventHandler(VersionEventHandler);
            }
            else if (strcmp(memberName, "size") == 0)
            {
                le_json_SetEventHandler(SizeEventHandler);
            }
            else
            {
                LE_ERROR("Malformed update pack (unexpected object member '%s').", memberName);
                HandleFormatError();
            }
            break;
        }

        case LE_JSON_STRING:
        case LE_JSON_NUMBER:
        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:

            LE_ERROR("Malformed update pack (unexpected '%s' encountered).",
                     le_json_GetEventName(event));
            HandleFormatError();
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts processing an update pack.
 *
 * Reads the update pack from a given file descriptor.
 *
 * Calls a callback function to report progress.
 */
//--------------------------------------------------------------------------------------------------
void updateUnpack_Start
(
    int fd,             ///< File descriptor to read the update pack from.
    updateUnpack_ProgressHandler_t progressFunc  ///< Progress reporting callback.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(State == STATE_IDLE);

    InputFd = fd;
    InputFdClosed = false; // reset InputFdClosed since it's initialized.
    ProgressFunc = progressFunc;
    PercentDone = 0;

    ProgressFunc(UPDATE_UNPACK_STATUS_UNPACKING, 0);

    Type = TYPE_UNKNOWN;

    StartParsing();
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the type of the update pack (available when 100% done).
 *
 * @return The type of update (firmware, app, or system).
 **/
//--------------------------------------------------------------------------------------------------
updateUnpack_Type_t updateUnpack_GetType
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return Type;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the app being changed (valid for app update or remove).
 *
 * @return The name of the app (valid until next unpack is started).
 **/
//--------------------------------------------------------------------------------------------------
const char* updateUnpack_GetAppName
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return AppName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the MD5 sum of the app being updated (valid for app update only).
 *
 * @return The MD5 sum of the app, as a string (valid until next unpack is started).
 **/
//--------------------------------------------------------------------------------------------------
const char* updateUnpack_GetAppMd5
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return Md5;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop unpacking an update pack.
 */
//--------------------------------------------------------------------------------------------------
void updateUnpack_Stop
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    switch (State)
    {
        case STATE_IDLE:
        case STATE_PARSING_JSON:
        case STATE_UNPACKING_PAYLOAD:
        case STATE_SKIPPING_PAYLOAD:
            Reset();
            return;
    }

    LE_FATAL("Invalid state %d", State);
}

