//--------------------------------------------------------------------------------------------------
/**
 * Definitions of functions needed by both the Component Parser and the Application Parser.
 *
 * Not to be shared outside the parser.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PARSER_COMMON_H_INCLUDE_GUARD
#define PARSER_COMMON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * File permissions flags translation function.  Converts text like "[rwx]" into a number which
 * is a set of bit flags.
 *
 * @return  The corresponding permissions flags (defined in Permissions.h) or'd together.
 **/
//--------------------------------------------------------------------------------------------------
int yy_GetPermissionFlags
(
    const char* stringPtr   // Ptr to the string representation of the permissions.
);


//--------------------------------------------------------------------------------------------------
/**
 * Number translation function.  Converts a string representation of a number into an actual number.
 *
 * @return  The number.
 */
//--------------------------------------------------------------------------------------------------
int yy_GetNumber
(
    const char* stringPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Strips any quotation marks out of a given string.
 *
 * @return  The new string.
 */
//--------------------------------------------------------------------------------------------------
std::string yy_StripQuotes
(
    const std::string& string
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileMapping object for a given "required" file.  This is a file that is to be
 * bind-mounted into the application sandbox from the target's unsandboxed file system.
 *
 * @return The FileMapping object.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
legato::FileMapping yy_CreateRequiredFileMapping
(
    const char* sourcePath, ///< The file path in the target file system, outside sandbox.
    const char* destPath,   ///< The file path in the target file system, inside sandbox.
    const legato::BuildParams_t& buildParams ///< Build parameters in effect.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileMapping object for a given "required" directory.  This is a directory that is
 * to be bind-mounted into the application sandbox from the target's unsandboxed file system.
 *
 * @return The FileMapping object.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
legato::FileMapping yy_CreateRequiredDirMapping
(
    const char* sourcePath, ///< The directory path in the target file system, outside sandbox.
    const char* destPath,   ///< The directory path in the target file system, inside sandbox.
    const legato::BuildParams_t& buildParams ///< Build parameters in effect.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileMapping object for a given "bundled" file.  This is a file that is to be
 * copied into the application bundle from the build host's file system.
 *
 * @return The FileMapping object.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
legato::FileMapping yy_CreateBundledFileMapping
(
    const char* permissions,///< String representing the permissions required ("[rwx]").
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath,   ///< The file path in the target file system, inside sandbox.
    const legato::BuildParams_t& buildParams ///< Build parameters in effect.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileMapping object for a given "bundled" directory.  This is a directory that is to be
 * copied into the application bundle from the build host's file system.
 *
 * @return The FileMapping object.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
legato::FileMapping yy_CreateBundledDirMapping
(
    const char* permissions,///< String representing permissions to be applied to files in the dir.
    const char* sourcePath, ///< The directory path in the build host file system.
    const char* destPath,   ///< The directory path in the target file system, inside sandbox.
    const legato::BuildParams_t& buildParams ///< Build parameters in effect.
);


//--------------------------------------------------------------------------------------------------
/**
 * Check that there's no illegal characters in an interface specification.
 *
 * @note This is necessary because the interface specifications are tokenized as FILE_PATH
 * tokens, which can have some characters that are not valid as parts of an interface
 * specification.
 *
 * @throw legato::Exception if there's a bad character.
 */
//--------------------------------------------------------------------------------------------------
void yy_CheckForBadCharsInInterfaceSpec
(
    const char* interfaceSpec
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given interface specifier is well formed.
 *
 * @throw legato::Exception if not.
 *
 * @return The number of parts it has (2 = "app.interface", 3 = "exe.comp.interface").
 **/
//--------------------------------------------------------------------------------------------------
size_t yy_CheckInterfaceSpec
(
    const std::string& interfaceSpec    ///< Specifier of the form "exe.component.interface"
);


//--------------------------------------------------------------------------------------------------
/**
 * Prints a warning message to stderr about realtime processes and the cpuShare limit.
 **/
//--------------------------------------------------------------------------------------------------
void yy_WarnAboutRealTimeAndCpuShare
(
    void
);


#endif // PARSER_COMMON_H_INCLUDE_GUARD
