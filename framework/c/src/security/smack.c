/** @file smack.c
 *
 * This is the implementation of the SMACK API.
 *
 * @todo Currently, this implementation writes directly into the smackfs files but we really should
 *       use a third party smack library so that we don't need to maintain this implementation when
 *       the smackfs file interface changes.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "smack.h"
#include "legato.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "user.h"
#include "fileSystem.h"
#include "le_cfg_interface.h"


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
#define SMACK_FS_DIR                        "/opt/legato/smack"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK load file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_LOAD_FILE                     SMACK_FS_DIR "/load"


//--------------------------------------------------------------------------------------------------
/**
 * SMACK access file location.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_ACCESS_FILE                   SMACK_FS_DIR "/access"


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
 * A process's own attribute file that stores the SMACK label.
 */
//--------------------------------------------------------------------------------------------------
#define PROC_SMACK_FILE                     "/proc/self/attr/current"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the config tree used for storing SMACK labels.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_SMACK_TREE                      "smack:"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the smack config tree that contains all the smack labels for apps.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_APPS                       "apps"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the smack config tree that contains the next value to use for an app
 * label.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_NEXT_VALUE                 "nextValue"


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
 * Set SMACK netlabel exception to grant applications permission to communicate with the Internet.
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
 * Initializes the SMACK system.  Mounts the smack file system.
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
    // Create the smack root directory.
    LE_FATAL_IF(le_dir_Make(SMACK_FS_DIR, S_IRUSR | S_IWUSR) == LE_FAULT,
                "Could not create SMACK file system directory.");

    // Mount the SMACKFS.
    if (!fs_IsMounted(SMACK_FS, SMACK_FS_DIR))
    {
        LE_FATAL_IF(mount(SMACK_FS, SMACK_FS_DIR, "smackfs", 0, NULL) != 0,
                    "Could not mount SMACK file system.  %m.");
    }

    // Set smack network exceptions
    SetSmackNetlabelExceptions();
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the given label is a valid SMACK label.
 *
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void CheckLabel
(
    const char* labelPtr            ///< [IN] The label to check.
)
{
    // Check lengths.
    size_t labelSize = strlen(labelPtr);

    LE_FATAL_IF(labelSize == 0, "SMACK label cannot be empty.");

    LE_FATAL_IF(labelSize > LIMIT_MAX_SMACK_LABEL_LEN,
                "SMACK label length, %zd chars, is too long.  Labels must be less than %d chars",
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
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void MakeSmackModeStr
(
    const char* modeStr,            ///< [IN] The user provided mode string.  This string can be any
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
    int n = snprintf(bufPtr, bufSize, "%-23s %-23s %5s", subjectLabelPtr, objectLabelPtr, modeStr);

    LE_FATAL_IF(n >= bufSize, "SMACK rule buffer is too small.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the smack label of the calling process.  The calling process must be a privileged process.
 *
 * @note If there is an error this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void smack_SetMyLabel
(
    const char* labelPtr            ///< [IN] The label to set the calling process to.
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
 * Sets the smack label of a file system object.  The calling process must be a privileged process.
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
 * Sets an explicit smack rule.
 *
 * An explicit smack rule defines a subject's access to an object.  The access mode can be any
 * combination of the following.
 *
 *      r: indicates that read access should be granted.
 *      w: indicates that write access should be granted.
 *      x: indicates that execute access should be granted.
 *      a: indicates that append access should be granted.
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
 * Get's the application's smack label.
 *
 * @note This function will kill the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetAppLabel
(
    const char* appNamePtr,         ///< [IN] Application name.
    char* bufPtr,                   ///< [OUT] Buffer to store the app's SMACK label.
    size_t bufSize                  ///< [IN] Size of the buffer.
)
{
    // NOTE: Numerical values are used as the app's SMACK label.  The SMACK labels are stored in the
    //       config and assigned in such a way as to guarantee they do not overlap.  The labels are
    //       stored in a separate config tree.
    //       We do this instead of using the app's UID because we need the SMACK labels for apps
    //       that have not been installed yet when we configure the SMACK rules for IPC bindings.
    //

    // TODO: The most reasonable thing to base an application's SMACK label on is the application
    //       name because it is gauranteed to be unique and is always associated with the app.
    //       However, because there is currently a limit of 23 characters for the SMACK label this
    //       can result in a truncation causing collisions.  The 23 character limit may be removed
    //       in future versions of SMACK so the application name can be used.

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateWriteTxn(CFG_SMACK_TREE);

    // Attempt to read the app's label from the config.
    le_cfg_GoToNode(cfgIter, CFG_NODE_APPS);

    if (!le_cfg_IsEmpty(cfgIter, appNamePtr))
    {
        LE_FATAL_IF(le_cfg_GetString(cfgIter, appNamePtr, bufPtr, bufSize, "") != LE_OK,
                    "Buffer for SMACK label '%s...' is too small.", bufPtr);

        if (strcmp(bufPtr, "") != 0)
        {
            le_cfg_CancelTxn(cfgIter);
            return;
        }
    }

    // The app does not have a SMACK label yet, we need to create one.
    LE_ASSERT(le_cfg_GoToParent(cfgIter) == LE_OK);

    // Read what the next number to use as the SMACK label is.
    int nextValue = le_cfg_GetInt(cfgIter, CFG_NODE_NEXT_VALUE, 1);

    // Increment that next label value.
    le_cfg_SetInt(cfgIter, CFG_NODE_NEXT_VALUE, nextValue + 1);

    // Convert the value to the label.
    LE_FATAL_IF(snprintf(bufPtr, bufSize, "%d", nextValue) >= bufSize,
                "Buffer for SMACK label '%d' is too small.", nextValue);

    // Write the label into the apps label node in the config.
    le_cfg_GoToNode(cfgIter, CFG_NODE_APPS);
    le_cfg_SetString(cfgIter, appNamePtr, bufPtr);

    // Commit the transaction.
    le_cfg_CommitTxn(cfgIter);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's the application's smack label with the access mode appended to it as a string, ie. "r",
 * "rw", etc.  If the accessMode is 0 then "-" will be appended to the app's smack label.
 *
 * @note This function will kill the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void smack_GetAppAccessLabel
(
    const char* appNamePtr,         ///< [IN] Application name.
    mode_t accessMode,              ///< [IN] Access mode.
    char* bufPtr,                   ///< [OUT] Buffer to store the app's SMACK label.
    size_t bufSize                  ///< [IN] Size of the buffer.
)
{
    // Get the app label.
    char label[LIMIT_MAX_SMACK_LABEL_LEN];
    smack_GetAppLabel(appNamePtr, label, sizeof(label));

    // Translate the accessMode to a string.
    char modeStr[4] = "-";
    int index = 0;

    if ((accessMode & S_IROTH) > 0)
    {
        modeStr[index++] = 'r';
    }
    if ((accessMode & S_IWOTH) > 0)
    {
        modeStr[index++] = 'w';
    }
    if ((accessMode & S_IXOTH) > 0)
    {
        modeStr[index++] = 'x';
    }

    modeStr[index] = '\0';

    // Create the access label.
    LE_FATAL_IF(snprintf(bufPtr, bufSize, "%s%s", label, modeStr) >= bufSize,
                "Buffer for SMACK label '%s...' is too small.", bufPtr);
}
