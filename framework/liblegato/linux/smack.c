/** @file smack.c
 *
 * This is the implementation of the SMACK API.
 *
 * @todo Currently, this implementation writes directly into the smackfs files but we really should
 *       use a third party smack library so that we don't need to maintain this implementation when
 *       the smackfs file interface changes.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "smack.h"
#include "legato.h"
#include "limit.h"
#include "fileSystem.h"
#include "fileDescriptor.h"
#include "file.h"


//--------------------------------------------------------------------------------------------------
/**
 * Name of the SMACK file system.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_FS                            "smack"


//--------------------------------------------------------------------------------------------------
/**
 * Location of the SMACK file system.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_FS_DIR                        "/legato/smack"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK load file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_LOAD_FILE                     SMACK_FS_DIR "/load2"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK access file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_ACCESS_FILE                   SMACK_FS_DIR "/access2"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK revoke subject file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_REVOKE_FILE                   SMACK_FS_DIR "/revoke-subject"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK netlabel file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_NETLABEL_FILE                 SMACK_FS_DIR "/netlabel"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK ipv6host file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_IPV6HOST_FILE                 SMACK_FS_DIR "/ipv6host"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK onlycap file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_ONLYCAP_FILE                  SMACK_FS_DIR "/onlycap"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK unconfined file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_UNCONFINED_FILE                  SMACK_FS_DIR "/unconfined"


//--------------------------------------------------------------------------------------------------
/**
 * A process's own attribute file that stores the SMACK label.
 */
//--------------------------------------------------------------------------------------------------
#define PROC_SMACK_FILE                     "/proc/self/attr/current"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum access mode string size.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_ACCESS_MODE_LEN                 5
#define MAX_ACCESS_MODE_BYTES               MAX_ACCESS_MODE_LEN + 1


//--------------------------------------------------------------------------------------------------
/**
 * SMACK rule string storage size.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_RULE_STR_BYTES                 2*LIMIT_MAX_SMACK_LABEL_LEN + MAX_ACCESS_MODE_LEN + 3


//--------------------------------------------------------------------------------------------------
/**
 * SMACK default load2 rules.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_DEFAULT_ACCESS_FILE "/etc/smack/accesses"


//********  SMACK is enabled.  *******************************************************************//
#if LE_CONFIG_ENABLE_SMACK

//--------------------------------------------------------------------------------------------------
/**
 * Set SMACK netlabel exception to grant applications permission to communicate with the Internet
 * via IPv4.
 */
//--------------------------------------------------------------------------------------------------
static void SetSmackNetlabelExceptions
(
    void
)
{
    // Open the calling process's smack file.
    int fd;

    do
    {
        fd = open(SMACK_NETLABEL_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_NETLABEL_FILE);

    // Write netlabel to the file.
    int result;
    size_t netlabelExceptionSize;

    do
    {
        netlabelExceptionSize = strlen("127.0.0.1 -CIPSO");
        result = write(fd, "127.0.0.1 -CIPSO", netlabelExceptionSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result != netlabelExceptionSize,
             "Could not write to %s.  %m.\n", SMACK_NETLABEL_FILE);

    do
    {
        netlabelExceptionSize = strlen("0.0.0.0/0 @");
        result = write(fd, "0.0.0.0/0 @", netlabelExceptionSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result != netlabelExceptionSize,
             "Could not write to %s.  %m.\n", SMACK_NETLABEL_FILE);

    fd_Close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set SMACK ipv6host exception to grant applications permission to communicate with the Internet.
 * via IPv6.
 */
//--------------------------------------------------------------------------------------------------
static void SetSmackIPv6HostExceptions
(
    void
)
{
    // Open the calling process's smack file.
    int fd;

    do
    {
        fd = open(SMACK_IPV6HOST_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if (fd == -1)
    {
        LE_WARN("Could not open %s.  %m.\n", SMACK_IPV6HOST_FILE);
        return;
    }

    // Write ipv6 to the file.
    int result;
    size_t ipv6hostExceptionSize;

    do
    {
        ipv6hostExceptionSize = strlen("0:0:0:0:0:0:0:0/0 @");
        result = write(fd, "0:0:0:0:0:0:0:0/0 @", ipv6hostExceptionSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_WARN_IF(result != ipv6hostExceptionSize,
        "Could not write to %s.  %m.\n", SMACK_IPV6HOST_FILE);

    fd_Close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the given label is a valid SMACK label.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void CheckLabel
(
    const char* labelPtr            ///< [IN] Label to check.
)
{
    // Check lengths.
    size_t labelSize = strlen(labelPtr);

    LE_FATAL_IF(labelSize == 0, "SMACK label cannot be empty.");

    LE_FATAL_IF(labelSize > LIMIT_MAX_SMACK_LABEL_LEN,
                "SMACK label length, %"PRIdS" chars, is too long.  Labels must be less than %d chars",
                labelSize, LIMIT_MAX_SMACK_LABEL_LEN);

    // Check for invalid characters.
    LE_FATAL_IF(labelPtr[0] == '-',
                "SMACK label '%s' is invalid because it begins with '-'.", labelPtr);

    int i;
    for (i = 0; i < labelSize; i++)
    {
        char c = labelPtr[i];

        if ( !isprint(c) || !isascii(c) || (c == '/') || (c == '\\') || (c == '\'') || (c == '"') )
        {
            LE_FATAL("SMACK label '%s' contain invalid character(s).", labelPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Given a user provided mode string create a mode string that conforms to what SMACK expects.
 *
 * @note If there'is an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void MakeSmackModeStr
(
    const char* modeStr,            ///< [IN] User provided mode string. This string can be any
                                    ///       combination of mode characters including duplicates.
    char* bufPtr,                   ///< [OUT] Buffer to hold the mode string that conforms to what
                                    ///        smack expects.
    size_t bufSize                  ///< [IN] Size of the buffer must be more than
                                    ///       MAX_ACCESS_MODE_LEN bytes.
)
{
    LE_ASSERT(bufSize > MAX_ACCESS_MODE_LEN);

    strncpy(bufPtr, "-----", bufSize);

    int i;
    for (i = 0; modeStr[i] != '\0'; i++)
    {
        switch(modeStr[i])
        {
            case 'r':
            case 'R':
                bufPtr[0] = 'r';
                break;

            case 'w':
            case 'W':
                bufPtr[1] = 'w';
                break;

            case 'x':
            case 'X':
                bufPtr[2] = 'x';
                break;

            case 'a':
            case 'A':
                bufPtr[3] = 'a';
                break;

            case 't':
            case 'T':
                bufPtr[4] = 't';

            case '-':
                break;

            default:
                LE_FATAL("SMACK mode string contains invalid characters.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a SMACK rule string that conforms to the what SMACK expects.
 *
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void MakeRuleStr
(
    const char* subjectLabelPtr,    ///< [IN] Subject label.
    const char* accessModePtr,      ///< [IN] Access mode.
    const char* objectLabelPtr,     ///< [IN] Object label.
    char* bufPtr,                   ///< [OUT] Buffer to store the created rule.
    size_t bufSize                  ///< [IN] Buffer size.  Must be atleast SMACK_RULE_STR_BYTES.
)
{
    // Create the SMACK mode string.
    char modeStr[MAX_ACCESS_MODE_BYTES];

    MakeSmackModeStr(accessModePtr, modeStr, sizeof(modeStr));

    // Create the SMACK rule.
    int n = snprintf(bufPtr, bufSize, "%s %s %s", subjectLabelPtr, objectLabelPtr, modeStr);

    LE_FATAL_IF(n >= bufSize, "SMACK rule buffer is too small.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Shows whether SMACK is enabled or disabled in the Legato Framework.
 *
 * @return
 *      true if SMACK is enabled.
 *      false if SMACK is disabled.
 */
//--------------------------------------------------------------------------------------------------
bool smack_IsEnabled
(
    void
)
{
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the SMACK system. Mounts the smack file system.
 *
 * @note Should be called once for the entire system, subsequent calls to this function will have no
 *       effect. Must be called before any of the other functions in this API is called.
 *
 * @note Failures will cause the calling process to exit.
 */
//--------------------------------------------------------------------------------------------------
void smack_Init
(
    void
)
{
    // Create the smack root directory.
    LE_FATAL_IF(le_dir_Make(SMACK_FS_DIR, S_IRUSR | S_IWUSR) == LE_FAULT,
                "Could not create SMACK file system directory.");

    // Mount the SMACKFS.
    if (!fs_IsMounted(SMACK_FS, SMACK_FS_DIR))
    {
        LE_FATAL_IF(mount(SMACK_FS, SMACK_FS_DIR, "smackfs", 0, NULL) != 0,
                    "Could not mount SMACK file system.  %m.");
    }
    else
    {
        LE_DEBUG("SMACK file system already mounted at %s.", SMACK_FS_DIR);
    }

    // Set smack network exceptions
    SetSmackNetlabelExceptions();
    SetSmackIPv6HostExceptions();
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label of the calling process. The calling process must be a privileged process.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetMyLabel
(
    const char* labelPtr            ///< [IN] Label to set the calling process to.
)
{
    CheckLabel(labelPtr);

    // Open the calling process's smack file.
    int fd;

    do
    {
        fd = open(PROC_SMACK_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", PROC_SMACK_FILE);

    // Write the label to the file.
    size_t labelSize = strlen(labelPtr);

    int result;

    do
    {
        result = write(fd, labelPtr, labelSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result != labelSize,
                "Could not write to %s.  %m.\n", PROC_SMACK_FILE);

    fd_Close(fd);

    LE_DEBUG("Setting process' SMACK label to '%s'.", labelPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the smack label of the calling process.
 *
 * @note If there's an error reading the SMACK label file, this function will
 *      kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetMyLabel
(
    char* labelPtr,         ///< [OUT] Pointer to the buffer to get the calling process's label
    uint32_t labelLen       ///< [IN] Size of the buffer pointed by labelPtr
)
{
    // Open the calling process's smack file.
    int fd;

    if (!labelPtr)
    {
        LE_ERROR("Recevied NULL pointer for output.");
        return;
    }

    do
    {
        fd = open(PROC_SMACK_FILE, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", PROC_SMACK_FILE);

    int result;

    do
    {
        result = read(fd, labelPtr, labelLen);
    }
    while ( (result == -1) && (errno == EINTR) );

    fd_Close(fd);

    LE_DEBUG("Retrieved process' SMACK label as '%s'.", labelPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's a process's smack label.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the supplied buffer is too small to hold the smack label.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_GetProcLabel
(
    pid_t pid,                      ///< [IN] PID of the process.
    char* bufPtr,                   ///< [OUT] Buffer to store the proc's SMACK label.
    size_t bufSize                  ///< [IN] Size of the buffer.
)
{
    // Get the process's smack file name.
    char smackFile[LIMIT_MAX_PATH_BYTES];
    LE_ASSERT(snprintf(smackFile, sizeof(smackFile), "/proc/%d/attr/current", pid) < sizeof(smackFile));

    // Open the process's smack file.
    int fd;

    do
    {
        fd = open(smackFile, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if (fd == -1)
    {
        LE_ERROR("Could not open %s.  %m.\n", smackFile);
        return LE_FAULT;
    }

    // Read the smack label.
    le_result_t result = fd_ReadLine(fd, bufPtr, bufSize);

    fd_Close(fd);

    if ( (result == LE_OUT_OF_RANGE) || (result == LE_FAULT) )
    {
        return LE_FAULT;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label of a file system object. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetLabel
(
    const char* objPathPtr,         ///< [IN] Path to the object.
    const char* labelPtr            ///< [IN] Label to set the object to.
)
{
    CheckLabel(labelPtr);

    if (setxattr(objPathPtr, "security.SMACK64", labelPtr, strlen(labelPtr), 0) == -1)
    {
        LE_ERROR("Could not set SMACK label for '%s'.  %m.", objPathPtr);
        return LE_FAULT;
    }

    LE_DEBUG("Set SMACK label to '%s' for %s.", labelPtr, objPathPtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack execute label of a file system object. The calling process must be a privileged
 * process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetLabelExec
(
    const char* objPathPtr,         ///< [IN] Path to the object.
    const char* labelPtr            ///< [IN] Label to set the object to.
)
{
    CheckLabel(labelPtr);

    if (setxattr(objPathPtr, "security.SMACK64EXEC", labelPtr, strlen(labelPtr), 0) == -1)
    {
        LE_ERROR("Could not set SMACK EXEC label for '%s'.  %m.", objPathPtr);
        return LE_FAULT;
    }

    LE_DEBUG("Set SMACK EXEC label to '%s' for %s.", labelPtr, objPathPtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable the SMACK64TRANSMUTE label for a directory.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetTransmuteLabel
(
    const char* objPathPtr         ///< [IN] Path to the object.
)
{
    if (setxattr(objPathPtr, "security.SMACK64TRANSMUTE", "TRUE", strlen("TRUE"), 0) == -1)
    {
        LE_ERROR("Could not set SMACK transmute label for '%s'.  %m.", objPathPtr);
        return LE_FAULT;
    }

    LE_DEBUG("Set SMACK transmute label to \"TRUE\"for %s.", objPathPtr);

    return LE_OK;

}

//--------------------------------------------------------------------------------------------------
/**
 * Disable the SMACK64TRANSMUTE label for a directory.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_UnsetTransmuteLabel
(
    const char* objPathPtr         ///< [IN] Path to the object.
)
{
    if (removexattr(objPathPtr, "security.SMACK64TRANSMUTE") == -1)
    {
        LE_ERROR("Could not set SMACK transmute label for '%s'.  %m.", objPathPtr);
        return LE_FAULT;
    }

    LE_DEBUG("Delete SMACK transmute label for '%s'.", objPathPtr);

    return LE_OK;

}


//--------------------------------------------------------------------------------------------------
/**
 * Get the smack labels of a socket object. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_GetSocketLabels
(
    int fd,                         ///< [IN] File descriptor of the object.
    char* labelInBufPtr,            ///< [OUT] Label to get the object to.
    size_t labelInBufSize,          ///< [IN] Size of the label input buffer.
    char* labelOutBufPtr,           ///< [OUT] Label to set the object to.
    size_t labelOutBufSize          ///< [IN] Size of the label output buffer.
)
{
    if ((labelInBufPtr != NULL) && (labelInBufSize > 0))
    {
        if (fgetxattr(fd, "security.SMACK64IPIN", labelInBufPtr, labelInBufSize) < 0)
        {
            labelInBufPtr[0] = '\0';
        }

        LE_DEBUG("Socket label SMACK64IPIN[%d]: '%s'", fd,
                                                       labelInBufPtr);
    }

    if ((labelOutBufPtr != NULL) && (labelOutBufSize > 0))
    {
        if (fgetxattr(fd, "security.SMACK64IPOUT", labelOutBufPtr, labelOutBufSize) < 0)
        {
            labelOutBufPtr[0] = '\0';
        }

        LE_DEBUG("Socket label SMACK64IPOUT[%d]: '%s'", fd,
                                                        labelOutBufPtr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label of a socket object. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetSocketLabels
(
    const int fd,                   ///< [IN] File descriptor of the object.
    const char* labelInPtr,         ///< [IN] Label to set the object to.
    const char* labelOutPtr         ///< [IN] Label to set the object to.
)
{
    if (labelInPtr != NULL)
    {
        CheckLabel(labelInPtr);

        if (fsetxattr(fd, "security.SMACK64IPIN", labelInPtr, strlen(labelInPtr), 0) == -1)
        {
            LE_ERROR("Could not set SMACK label SMACK64IPIN for socket %d to %s.  %m.",
                     fd, labelInPtr);
            return LE_FAULT;
        }
    }

    if (labelOutPtr != NULL)
    {
        CheckLabel(labelOutPtr);

        if (fsetxattr(fd, "security.SMACK64IPOUT", labelOutPtr, strlen(labelOutPtr), 0) == -1)
        {
            LE_ERROR("Could not set SMACK label SMACK64IPOUT for socket %d to %s.  %m.",
                     fd, labelOutPtr);
            return LE_FAULT;
        }
    }

    LE_DEBUG("Set SMACK labels to in['%s'] out['%s'] for socket %d.", labelInPtr,
                                                                      labelOutPtr,
                                                                      fd);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets an explicit smack rule.
 *
 * An explicit smack rule defines a subject's access to an object. The access mode can be any
 * combination of the following.
 *
 *      r: read access should be granted.
 *      w: write access should be granted.
 *      x: execute access should be granted.
 *      a: append access should be granted.
 *      -: used as a place holder.
 *
 * For example:
 *      "rx" means read and execute access should be granted.
 *      "-" means that no access should be granted.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetRule
(
    const char* subjectLabelPtr,    ///< [IN] Subject label.
    const char* accessModePtr,      ///< [IN] Access mode. See the function description for details.
    const char* objectLabelPtr      ///< [IN] Object label.
)
{
    CheckLabel(subjectLabelPtr);
    CheckLabel(objectLabelPtr);

    // Create the SMACK rule.
    char rule[SMACK_RULE_STR_BYTES];
    MakeRuleStr(subjectLabelPtr, accessModePtr, objectLabelPtr, rule, sizeof(rule));

    // Open the SMACK load file.
    int fd;

    do
    {
        fd = open(SMACK_LOAD_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_LOAD_FILE);

    // Write the rule to the SMACK load file.
    int numBytes = 0;
    size_t ruleLength = strlen(rule);

    do
    {
        numBytes = write(fd, rule, ruleLength);
    }
    while ( (numBytes == -1) && (errno == EINTR) );

    LE_FATAL_IF(numBytes != ruleLength, "Could not write SMACK rule '%s'.  %m.", rule);

    fd_Close(fd);

    LE_DEBUG("Set SMACK rule '%s'.", rule);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a subject has the specified access mode for an object.
 *
 * @return
 *      true if the subject has the specified access mode for the object.
 *      false if the subject doesn't have the specified access mode for the object.
 */
//--------------------------------------------------------------------------------------------------
bool smack_HasAccess
(
    const char* subjectLabelPtr,    ///< [IN] Subject label.
    const char* accessModePtr,      ///< [IN] Access mode.
    const char* objectLabelPtr      ///< [IN] Object label.
)
{
    CheckLabel(subjectLabelPtr);
    CheckLabel(objectLabelPtr);

    // Create the SMACK rule.
    char rule[SMACK_RULE_STR_BYTES];
    MakeRuleStr(subjectLabelPtr, accessModePtr, objectLabelPtr, rule, sizeof(rule));

    // Open the SMACK access file.
    int fd;

    do
    {
        fd = open(SMACK_ACCESS_FILE, O_RDWR);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_ACCESS_FILE);

    // Write the rule to the SMACK access file.
    int numBytes = 0;

    do
    {
        numBytes = write(fd, rule, sizeof(rule)-1);
    }
    while ( (numBytes == -1) && (errno == EINTR) );

    LE_FATAL_IF(numBytes < 0, "Could not write SMACK rule '%s'.  %m.", rule);

    // Read the SMACK access file to see if access would be granted.
    char a;
    do
    {
        numBytes = read(fd, &a, 1);
    }
    while ( (numBytes == -1) && (errno == EINTR) );

    LE_FATAL_IF(numBytes <= 0, "Could not read '%s'.  %m.", SMACK_ACCESS_FILE);

    fd_Close(fd);

    return (a == '1');
}


//--------------------------------------------------------------------------------------------------
/**
 * Revokes all the access rights for a subject that was given by explicit SMACK rules.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_RevokeSubject
(
    const char* subjectLabelPtr     ///< [IN] Subject label.
)
{
    // Open the SMACK revoke file.
    int fd;

    do
    {
        fd = open(SMACK_REVOKE_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_REVOKE_FILE);

    // Write the label to the SMACK revoke file.
    int numBytes = 0;

    do
    {
        numBytes = write(fd, subjectLabelPtr, strlen(subjectLabelPtr));
    }
    while ( (numBytes == -1) && (errno == EINTR) );

    LE_FATAL_IF(numBytes < 0, "Could not revoke SMACK label '%s'.  %m.", subjectLabelPtr);

    fd_Close(fd);

    LE_DEBUG("Revoked SMACK label '%s'.", subjectLabelPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's SMACK label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 *
 * @warning
 *      This function kills the calling process if there is an error such as if the buffer is too
 *      small.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetAppLabel
(
    const char* appNamePtr,     ///< [IN] Name of the application.
    char* bufPtr,               ///< [OUT] Buffer to store the proc's SMACK label.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    LE_FATAL_IF(appNamePtr[0] == '\0', "App name should not be empty.");

    LE_FATAL_IF(snprintf(bufPtr, bufSize, "%s%s", SMACK_APP_PREFIX, appNamePtr) >= bufSize,
                "Buffer is too small to hold SMACK label for app %s.",
                appNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's the application's smack label with the user's access mode appended to it as a string.  For
 * example, if the accessMode is S_IRUSR | S_IWUSR then "rw" will be appended to the application's
 * smack label.  The groups and other bits of accessMode are ignored.  If the user's accessMode is 0
 * (empty) then "-" will be appended to the app's smack label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 *
 * @warning
 *      This function kills the calling process if there is an error such as if the buffer is too
 *      small.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetAppAccessLabel
(
    const char* appNamePtr,     ///< [IN] Name of the application.
    mode_t accessMode,          ///< [IN] Access mode.
    char* bufPtr,               ///< [OUT] Buffer to store the proc's SMACK label.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    // Get the app label.
    char appLabel[LIMIT_MAX_SMACK_LABEL_LEN];
    smack_GetAppLabel(appNamePtr, appLabel, sizeof(appLabel));

    // Translate the accessMode to a string.
    char modeStr[4] = "-";
    int index = 0;

    if ((accessMode & S_IRUSR) > 0)
    {
        modeStr[index++] = 'r';
    }
    if ((accessMode & S_IWUSR) > 0)
    {
        modeStr[index++] = 'w';
    }
    if ((accessMode & S_IXUSR) > 0)
    {
        modeStr[index++] = 'x';
    }

    modeStr[index] = '\0';

    // Create the access label.
    LE_FATAL_IF(snprintf(bufPtr, bufSize, "%s%s", appLabel, modeStr) >= bufSize,
                "Buffer is too small to hold SMACK access label %s for app %s.",
                appLabel, appNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a SMACK label for a device file from the device ID.
 *
 * @return
 *      LE_OK if the label was successfully returned in the provided buffer.
 *      LE_OVERFLOW if the label could not fit in the provided buffer.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_GetDevLabel
(
    dev_t devId,            ///< [IN] Device ID.
    char* bufPtr,           ///< [OUT] Buffer to hold the SMACK label for the device.
    size_t bufSize          ///< [IN] Size of the buffer.
)
{
    int n = snprintf(bufPtr, bufSize, "dev.%x%x", major(devId), minor(devId));

    if (n < 0)
    {
        LE_ERROR("Could not generate SMACK label for device (%d, %d).  %m.", major(devId), minor(devId));
        return LE_FAULT;
    }

    if (n >= bufSize)
    {
        return LE_OVERFLOW;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label in unconfined. This contains the label processes whose access violations
 * will be logged but not prohibited.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetUnconfined
(
    const char* labelPtr            ///< [IN] Label to set the calling process to.
)
{
    CheckLabel(labelPtr);

    // Open the SMACK onlycap file
    int fd;

    do
    {
        fd = open(SMACK_UNCONFINED_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_UNCONFINED_FILE);

    // Write the label to the file.
    size_t labelSize = strlen(labelPtr);

    int result;

    do
    {
        result = write(fd, labelPtr, labelSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result != labelSize,
                "Could not write to %s.  %m.\n", PROC_SMACK_FILE);

    fd_Close(fd);

    LE_INFO("Set SMACK label '%s' unconfined.", labelPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label in onlycap. This contains the label processes must have for CAP_MAC_ADMIN
 * and CAP_MAC_OVERRIDE.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetOnlyCap
(
    const char* labelPtr            ///< [IN] Label to set the calling process to.
)
{
    // Don't need to check if it's a valid label if we're removing the OnlyCap settings
    if(strcmp(labelPtr, "-") != 0)
    {
        CheckLabel(labelPtr);
    }

    // Open the SMACK onlycap file
    int fd;

    do
    {
        fd = open(SMACK_ONLYCAP_FILE, O_WRONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_ONLYCAP_FILE);

    // Write the label to the file.
    size_t labelSize = strlen(labelPtr);

    int result;

    do
    {
        result = write(fd, labelPtr, labelSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result != labelSize,
                "Could not write to %s.  %m.\n", SMACK_ONLYCAP_FILE);

    fd_Close(fd);

    LE_INFO("Set SMACK label '%s' onlycap.", labelPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks if the SMACK FS has been mounted.
 *
 * @return
 *      true if has been mounted.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool smack_IsMounted
(
    void
)
{
    return fs_IsMounted(SMACK_FS, SMACK_FS_DIR);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks if the smack label in onlycap is set.
 *
 * @return
 *      true if at least one smack onlycap label has been set.
 *      false if there isn't any smack onlycap label set.
 */
//--------------------------------------------------------------------------------------------------
bool smack_IsOnlyCapSet
(
    void
)
{
    int fd;

    do
    {
        fd = open(SMACK_ONLYCAP_FILE, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", SMACK_ONLYCAP_FILE);

    char labelPtr[LIMIT_MAX_SMACK_LABEL_LEN];

     // Read the smack ONLYCAP label.
    le_result_t result = fd_ReadLine(fd, labelPtr, sizeof(labelPtr));

    fd_Close(fd);

    if ( (result == LE_OUT_OF_RANGE) || (result == LE_FAULT) )
    {
        LE_DEBUG("No SMACK ONLYCAP label set");
        return false;
    }
    else
    {
        LE_DEBUG("SMACK ONLYCAP label(s) set: '%s'", labelPtr);
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the smack labels of a file descriptor. The calling process must be a privileged process.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetFdSmackLabel
(
    int fd,                         ///< [IN] File descriptor.
    char *labelPtr,                 ///< [IN/OUT] Label of the FD.
    size_t labelLens                ///< [IN] Size of labelPtr.
)
{
    if (fgetxattr(fd, "security.SMACK64", labelPtr, labelLens) < 0)
    {
        labelPtr[0] = '\0';
    }
    LE_DEBUG("File Descriptor label SMACK64[%d]: '%s'", fd,
                labelPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the smack labels to a file descriptor. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_BAD_PARAMETER if labelPtr is NULL.
 *      LE_FAULT if unable to set the label to file.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetFdSmackLabel
(
    int fd,                         ///< [IN] File Descriptor.
    const char *labelPtr            ///< [IN] SMACK label to set FD to.
)
{
    if (labelPtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    CheckLabel(labelPtr);

    if (fsetxattr(fd, "security.SMACK64", labelPtr, strlen(labelPtr), 0) == -1)
    {
        LE_ERROR("Could not set SMACK label SMACK64 for socket %d to %s.  %m.",
                    fd, labelPtr);
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Append the default smack rules in the target mount directory.
 */
//--------------------------------------------------------------------------------------------------
void smack_LoadDefaultRules()
{
}

//********  SMACK is disabled.  ******************************************************************//
#else

//--------------------------------------------------------------------------------------------------
/**
 * Shows whether SMACK is enabled or disabled in the Legato Framework.
 *
 * @return
 *      true if SMACK is enabled.
 *      false if SMACK is disabled.
 */
//--------------------------------------------------------------------------------------------------
bool smack_IsEnabled
(
    void
)
{
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the SMACK system.  Mounts the SMACK file system.
 *
 * @note Should be called once for the entire system, subsequent calls to this function will have no
 *       effect.  Must be called before any of the other functions in this API is called.
 *
 * @note Failures will cause the calling process to exit.
 */
//--------------------------------------------------------------------------------------------------
void smack_Init
(
    void
)
{
    LE_INFO("********* SMACK policy settings are disabled in the Legato Framework ONLY. *********");
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the SMACK label of the calling process. The calling process must be a privileged process.
 *
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetMyLabel
(
    const char* labelPtr            ///< [IN] Label to set the calling process to.
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the smack label of the calling process.
 *
 * @note If there's an error reading the SMACK label file, this function will
 *      kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetMyLabel
(
    char* labelPtr,         ///< [OUT] Pointer to the buffer to get the calling process's label
    uint32_t labelLen       ///< [IN] Size of the buffer pointed by labelPtr
    )
{
    if (labelLen > 0)
    {
        labelPtr[0] = '\0';
    }

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's a process's SMACK label.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the supplied buffer is too small to hold the SMACK label.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_GetProcLabel
(
    pid_t pid,                      ///< [IN] PID of the process.
    char* bufPtr,                   ///< [OUT] Buffer to store the proc's SMACK label.
    size_t bufSize                  ///< [IN] Size of the buffer.
)
{
    if (bufSize > 0)
    {
        bufPtr[0] = '\0';
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the SMACK label of a file system object. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetLabel
(
    const char* objPathPtr,         ///< [IN] Path to the object.
    const char* labelPtr            ///< [IN] The label to set the object to.
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack execute label of a file system object. The calling process must be a privileged
 * process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetLabelExec
(
    const char* objPathPtr,         ///< [IN] Path to the object.
    const char* labelPtr            ///< [IN] Label to set the object to.
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the smack labels of a socket object. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_GetSocketLabels
(
    int fd,                         ///< [IN] File descriptor of the object.
    char* labelInBufPtr,            ///< [OUT] Label to set the object to.
    size_t labelInBufSize,          ///< [IN] Size of the label input buffer.
    char* labelOutBufPtr,           ///< [OUT] Label to set the object to.
    size_t labelOutBufSize          ///< [IN] Size of the label output buffer.
)
{
    if ((labelInBufPtr != NULL) && (labelInBufSize > 0))
    {
        labelInBufPtr[0] = '\0';
    }

    if ((labelOutBufPtr != NULL) && (labelOutBufSize > 0))
    {
        labelOutBufPtr[0] = '\0';
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label of a socket object. The calling process must be a privileged process.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetSocketLabels
(
    const int fd,                   ///< [IN] File descriptor of the object.
    const char* labelInPtr,         ///< [IN] Label to set the object to.
    const char* labelOutPtr         ///< [IN] Label to set the object to.
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable the SMACK64TRANSMUTE label for a directory.
 *
 * @return
 *      LE_OK if the label was set correctly.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetTransmuteLabel
(
    const char* objPathPtr         ///< [IN] Path to the object.
)
{
    LE_INFO("Setting SMACK Transmute label is not supported in this build.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Disable the SMACK64TRANSMUTE label for a directory.
 *
 * @return
 *      LE_OK if the label was set correctly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_UnsetTransmuteLabel
(
    const char* objPathPtr         ///< [IN] Path to the object.
)
{
    LE_INFO("Setting SMACK Transmute label is not supported in this build.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets an explicit SMACK rule.
 *
 * An explicit SMACK rule defines a subject's access to an object. The access mode can be any
 * combination of the following.
 *
 *      a: indicates that append access should be granted.
 *      r: indicates that read access should be granted.
 *      w: indicates that write access should be granted.
 *      x: indicates that execute access should be granted.
 *      t: indicates that the rule requests transmutation.
 *      -: is used as a place holder.
 *
 * For example:
 *      "rx" means read and execute access should be granted.
 *      "-" means that no access should be granted.
 *
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetRule
(
    const char* subjectLabelPtr,    ///< [IN] Subject label.
    const char* accessModePtr,      ///< [IN] Access mode. See the function description for details.
    const char* objectLabelPtr      ///< [IN] Object label.
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a subject has the specified access mode for an object.
 *
 * @return
 *      true if the subject has the specified access mode for the object.
 *      false if the subject does not have the specified access mode for the object.
 */
//--------------------------------------------------------------------------------------------------
bool smack_HasAccess
(
    const char* subjectLabelPtr,    ///< [IN] Subject label.
    const char* accessModePtr,      ///< [IN] Access mode.
    const char* objectLabelPtr      ///< [IN] Object label.
)
{
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Revokes all the access rights for a subject that were given by explicit SMACK rules.
 *
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_RevokeSubject
(
    const char* subjectLabelPtr     ///< [IN] Subject label.
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's SMACK label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 *
 * @warning
 *      This function kills the calling process if there is an error such as if the buffer is too
 *      small.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetAppLabel
(
    const char* appNamePtr,     ///< [IN] Name of the application.
    char* bufPtr,               ///< [OUT] Buffer to store the proc's SMACK label.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    if (bufSize > 0)
    {
        bufPtr[0] = '\0';
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's the application's smack label with the user's access mode appended to it as a string.  For
 * example, if the accessMode is S_IRUSR | S_IWUSR then "rw" will be appended to the application's
 * smack label.  The groups and other bits of accessMode are ignored.  If the user's accessMode is 0
 * (empty) then "-" will be appended to the app's smack label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 *
 * @warning
 *      This function kills the calling process if there is an error such as if the buffer is too
 *      small.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetAppAccessLabel
(
    const char* appNamePtr,     ///< [IN] Name of the application.
    mode_t accessMode,          ///< [IN] Access mode.
    char* bufPtr,               ///< [OUT] Buffer to store the proc's SMACK label.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    if (bufSize > 0)
    {
        bufPtr[0] = '\0';
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a SMACK label for a device file from the device ID.
 *
 * @return
 *      LE_OK if the label was successfully returned in the provided buffer.
 *      LE_OVERFLOW if the label could not fit in the provided buffer.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_GetDevLabel
(
    dev_t devId,            ///< [IN] Device ID.
    char* bufPtr,           ///< [OUT] Buffer to hold the SMACK label for the device.
    size_t bufSize          ///< [IN] Size of the buffer.
)
{
    if (bufSize > 0)
    {
        bufPtr[0] = '\0';
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label in onlycap. This contains the label processes must have for CAP_MAC_ADMIN
 * and CAP_MAC_OVERRIDE.
 *
 * @note If there's an error, this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetOnlyCap
(
    const char* labelPtr            ///< [IN] Label to set the calling process to.
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the SMACK FS has been mounted.
 *
 * @return
 *      true if has been mounted.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool smack_IsMounted
(
    void
)
{
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the smack label in onlycap is set.
 *
 * @return
 *      true if at least one smack onlycap label has been set.
 *      false if there isn't any smack onlycap label set.
 */
//--------------------------------------------------------------------------------------------------
bool smack_IsOnlyCapSet
(
    void
)
{
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the smack labels of a file descriptor. The calling process must be a privileged process.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetFdSmackLabel
(
    int fd,                         ///< [IN] File descriptor.
    char *labelPtr,                 ///< [IN/OUT] Label of the FD.
    size_t labelLens                ///< [IN] Size of labelPtr.
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the smack labels to a file descriptor. The calling process must be a privileged process.
 *
 *      LE_OK if the label was set correctly.
 *      LE_BAD_PARAMETER if labelPtr is NULL.
 *      LE_FAULT if unable to set the label to file.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smack_SetFdSmackLabel
(
    int fd,                         ///< [IN] File Descriptor.
    const char *labelPtr            ///< [IN] SMACK label to set FD to.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Append the default smack rules in the target mount directory.
 */
//--------------------------------------------------------------------------------------------------
void smack_LoadDefaultRules()
{
    // Default rules are quite small and very general.
    char readBuffer[256];
    int numBytesRead = file_ReadStr(SMACK_DEFAULT_ACCESS_FILE, readBuffer, sizeof(readBuffer));
    if (numBytesRead  == -1 || numBytesRead > sizeof(readBuffer))
    {
        LE_ERROR("Error when reading from: '%s', the number of bytes read: %d",
                    SMACK_DEFAULT_ACCESS_FILE, numBytesRead);
    }

    file_WriteStr(SMACK_LOAD_FILE, readBuffer, 0);
}
#endif
