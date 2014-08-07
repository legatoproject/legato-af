//--------------------------------------------------------------------------------------------------
/**
 * Definitions of functions needed by both the Component Parser and the Application Parser.
 *
 * Not to be shared outside the parser.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
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
 * Checks whether a given required file's on-target file system path (outside the app's runtime
 * environment) is valid.
 *
 * @throw legato::Exception if not.
 */
//--------------------------------------------------------------------------------------------------
void yy_CheckRequiredFilePathValidity
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given required directory's on-target file system path (outside the app's runtime
 * environment) is valid.
 *
 * @throw legato::Exception if not.
 */
//--------------------------------------------------------------------------------------------------
void yy_CheckRequiredDirPathValidity
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given bundled or required file or directory destination file system path
 * (inside the app's runtime environment) is valid.
 *
 * @throw legato::Exception if not.
 */
//--------------------------------------------------------------------------------------------------
void yy_CheckBindMountDestPathValidity
(
    const std::string& path
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
    const char* permissions,///< String representing the permissions required ("[rwx]").
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
    const char* permissions,///< String representing the permissions required ("[rwx]").
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
    const char* sourcePath, ///< The directory path in the build host file system.
    const char* destPath,   ///< The directory path in the target file system, inside sandbox.
    const legato::BuildParams_t& buildParams ///< Build parameters in effect.
);


#endif // PARSER_COMMON_H_INCLUDE_GUARD
