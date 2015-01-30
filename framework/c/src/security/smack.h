/** @file smack.h
 *
 * @ref c_smack_intro <br>
 * @ref c_smack_assignLabels <br>
 * @ref c_smack_setRules <br>
 *
 * @section c_smack_intro Introduction
 *
 * SMACK (Simplified Mandatory Access Control Kernel) is a kernel feature that provides mandatory
 * access control.
 *
 * The key feature of MAC (mandatory access control) is the ability of a centralized entity to set
 * access policy to system resources.  The easiest way to understand MAC is to compare it to Linux's
 * default access control system.
 *
 * Linux's default access control policy is governed by the permission bits on system resources (ie.
 * files, directories, devices, etc.).  The permission bits on files may be modified by the owner of
 * the resource, a process with the same user ID as the resource.  The access control policy is at
 * the discretion of the resource owner, hence this system is classified as DAC (discretionary
 * access control).  Note also that with DAC, policies are set in a distributed manner because there
 * are often many users in a system each setting the access policy for its own resources.
 *
 * In contrast MAC policies are set for all resources on the system by a centralized entity.
 *
 * Linux's DAC has many known weaknesses that can lead to security leaks.  So MAC is often used to
 * overcome some of the short comings of DAC for systems that require a higher level of security.
 *
 * SMACK is not the only MAC solution available.  It is, however, a solution that was built with
 * simplicity as one of its main objectives.  Its simplicity means that it is not flexible enough to
 * handle all use cases but it also means that for the majority of use cases it will be easier to
 * setup and maintain.
 *
 * SMACK supplements Linux's DAC system, that is to say, DAC permissions are checked first and only
 * if access is granted SMACK permissions are then checked.  As a result SMACK can only limit access
 * it cannot grant access beyond DAC permissions.
 *
 * SMACK uses 'labels' on resources (objects in SMACK terminology) and processes (subjects) to
 * determine access.  Labels on resources can only be set by a privileged process.  A privileged
 * process can only set it's own label but not labels of other processes.
 *
 * There are a number of single character labels ("_", "^", "*", "?", "@") that have special
 * meanings.
 *
 * SMACK restricts read/write/execute access based on the label of the subject and the label of the
 * object according to the following rules.
 *
 *      1. Any access requested by a task labelled "*" is denied.
 *      2. A read or execute access requested by a task labelled "^" is permitted.
 *      3. A read or execute access requested on an object labelled "_" is permitted.
 *      4. Any access requested on an object labelled "*" is permitted.
 *      5. Any access requested by a task on an object with the same label is permitted.
 *      6. Any access requested that is explicitly defined in the loaded rule set is permitted.
 *      7. Any other access is denied.
 *
 * As alluded to in rule 6 above explicit rules can be added for specific labels.  Explicit rules
 * define the access rights that a subject label can have on an object label.  See the
 * c_smack_setRules section for details.  Only privileged processes can set rules.
 *
 * Generally, privileged processes are processes with the CAP_MAC_OVERRIDE capability.  However,
 * it is also possible to configure the system such that the CAP_MAC_OVERRIDE is honoured only for
 * processes with a certain label.  This configuration allows the system to restrict processes which
 * are root (have CAP_MAC_OVERRIDE) but do not have the proper SMACK label.
 *
 * @section c_smack_assignLabels Assigning SMACK Labels
 *
 * Use smack_SetMyLabel() to set the SMACK label for the calling process.  The calling process must
 * be a privileged process.  Setting SMACK labels for other processes is not possible.
 *
 * To set the SMACK label for file system objects use smack_SetLabel(), again the calling process
 * must be privileged.
 *
 * @section c_smack_setRules Setting SMACK Rules
 *
 * Use smack_SetRule() to set an explicit SMACK rule that gives a specified subject access to a
 * specified object.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_SRC_SMACK_INCLUDE_GUARD
#define LEGATO_SRC_SMACK_INCLUDE_GUARD

#include "legato.h"


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets an explicit smack rule.
 *
 * An explicit smack rule defines a subject's access to an object.  The access mode can be any
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
);


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
);


#endif // LEGATO_SRC_SMACK_INCLUDE_GUARD
