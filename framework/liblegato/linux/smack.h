/** @file smack.h
 *
 * Simplified Mandatory Access Control Kernel (SMACK) provides a simple solution
 * for mandatory access control (MAC). MAC provides the ability for a centralized entity to
 * set access policy for system resources.
 *
 * Linux's default access control policy is governed by permission bits on system resources
 * (files, directories, devices, etc.). Permission bits can be modified by the resource owner
 * (process with the same user ID as the resource). The access control policy is at
 * the discretion of the resource owner; this system is classified as DAC (discretionary
 * access control).  With DAC, policies are set in a distributed manner as there
 * are often many system users, each setting the access policy for its own resources.
 *
 * In contrast, MAC policies are set for all system resources by a centralized entity.
 *
 * Linux's DAC has known weaknesses that can lead to security leaks. MAC is often used to
 * overcome some of the short comings of DAC for systems that require a higher level of security.
 *
 * SMACK isn't the only MAC solution available. Because it's a simple solution, it's not flexible enough
 * to handle all use cases. For the majority of use cases, it will be easier to setup and maintain.
 *
 * SMACK supplements Linux's DAC system. DAC permissions are checked first; if access is granted,
 * SMACK permissions are then checked. Consequently, SMACK can only limit access,
 * it can't grant access beyond DAC permissions.
 *
 * SMACK uses 'labels' on resources (objects in SMACK terminology) and processes (subjects) to
 * determine access.  Labels on resources can only be set by a privileged process.  A privileged
 * process can only set it's own label but not labels of other processes.
 *
 * There are a number of single character labels ("_", "^", "*", "?", "@") that have special
 * meanings. SMACK restricts read/write/execute access based on the subject label and the object
 * label according to the following rules:
 *
 * 1. Any access requested by a task labelled "*" is denied.
 * 2. A read or execute access requested by a task labelled "^" is permitted.
 * 3. A read or execute access requested on an object labelled "_" is permitted.
 * 4. Any access requested on an object labelled "*" is permitted.
 * 5. Any access requested by a task on an object with the same label is permitted.
 * 6. Any access requested that is explicitly defined in the loaded rule set is permitted.
 * 7. Any other access is denied.
 *
 * Rule 6 lets us use explicit rules through adding specific labels. Explicit rules
 * define the access rights a subject label can have on an object label. See the
 * @ref c_smack_setRules section for details.  Only privileged processes can set rules.
 *
 * @section c_smack_privProcess Privileged Processess
 *
 * Privileged processes use the CAP_MAC_OVERRIDE capability. It's also possible to configure
 * the system so the CAP_MAC_OVERRIDE is honoured only for processes with a specific label. This
 * configuration allows the system to restrict root processes (have CAP_MAC_OVERRIDE) that don't
 * have the proper SMACK label.
 *
 * @section c_smack_assignLabels Assigning Labels
 *
 * Use smack_SetMyLabel() to set the SMACK label for the calling process. The calling process must
 * be a privileged process. Setting SMACK labels for other processes isn't possible.
 *
 * To set the SMACK label for file system objects use smack_SetLabel(), again the calling process
 * must be privileged.
 *
 * @section c_smack_setRules Setting Rules
 *
 * Use smack_SetRule() to set an explicit SMACK rule that gives a specified subject access to a
 * specified object.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_SMACK_INCLUDE_GUARD
#define LEGATO_SRC_SMACK_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Application prefix for SMACK labels.
 */
//--------------------------------------------------------------------------------------------------
#define SMACK_APP_PREFIX          "app."


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
);


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
);


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
);


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
);


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
);



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
);



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
);


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
);


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
);


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
);


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
);


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
);


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
);

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
);

#endif // LEGATO_SRC_SMACK_INCLUDE_GUARD
