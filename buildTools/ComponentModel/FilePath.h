//--------------------------------------------------------------------------------------------------
/**
 * File system path handling functions.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef FILE_PATH_H_INCLUDE_GUARD
#define FILE_PATH_H_INCLUDE_GUARD


extern "C"
{
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
}


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * @return true if the path is valid (not empty and doesn't contain ".." elements that
 *              take it above its starting point if it is an absolute path).
 */
//--------------------------------------------------------------------------------------------------
bool IsValidPath
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * @return true if the path is absolute (starts with a '/').
 */
//--------------------------------------------------------------------------------------------------
bool IsAbsolutePath
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Concatenate two file system paths together.
 *
 * @return  The combined path.
 */
//--------------------------------------------------------------------------------------------------
std::string CombinePath
(
    const std::string& base,  ///< The left-hand part of the path.
    const std::string& add    ///< The right-hand part of the path.
);


//--------------------------------------------------------------------------------------------------
/**
 * Make a file system path into an absolute path.
 *
 * @return  The absolute path.
 */
//--------------------------------------------------------------------------------------------------
std::string AbsolutePath
(
    const std::string& path     ///< The original path, which could be absolute or relative.
);


//--------------------------------------------------------------------------------------------------
/**
 * Make a file system path into a relative path by stripping off leading separators.
 *
 * @return  The relative path.
 */
//--------------------------------------------------------------------------------------------------
std::string MakeRelativePath
(
    const std::string& path     ///< The original path, which could be absolute or relative.
);


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether or not a given path refers to a directory in the local file system.
 *
 * @return true if the directory can be seen to exist (but it may not be accessible).
 */
//--------------------------------------------------------------------------------------------------
bool DirectoryExists
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether or not a given path refers to a regular file in the local file system.
 *
 * @return true if the file can be seen to exist (but it may not be accessible).
 */
//--------------------------------------------------------------------------------------------------
bool FileExists
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * @return The path of the directory containing this path, or "" if that can't be determined.
 */
//--------------------------------------------------------------------------------------------------
std::string GetContainingDir
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * @return The last part of a file path (e.g., just the name of a file, with no directories
 * or slashes in front of it).
 */
//--------------------------------------------------------------------------------------------------
std::string GetLastPathNode
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given path has one of the suffixes in a given list of suffixes.
 *
 * @return  true if the path's suffix matches one of the suffixes in the list.
 */
//--------------------------------------------------------------------------------------------------
bool HasSuffix
(
    const std::string& path,
    const std::list<std::string>& suffixList
);


//--------------------------------------------------------------------------------------------------
/**
 * Searches for a file.
 *
 * If the file path given is absolute, then just checks for existence of a file at that path.
 * If the file path is relative, then searches for that file relative to each of the directories
 * in the searchPaths list.
 *
 * @return The path of the file if found or an empty string if not found.
 */
//--------------------------------------------------------------------------------------------------
std::string FindFile
(
    const std::string& path,                    ///< The file path.
    const std::list<std::string>& searchPaths   ///< List of directory paths to search in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Searches for a directory.
 *
 * If the path given is absolute, then just checks for existence of a directory at that path.
 * If the path is relative, then searches for that directory relative to each of the
 * directories in the searchPaths list.
 *
 * @return The path of the directory if found or an empty string if not found.
 */
//--------------------------------------------------------------------------------------------------
std::string FindDirectory
(
    const std::string& path,                    ///< The path to search for.
    const std::list<std::string>& searchPaths   ///< List of directory paths to search in.
);


//----------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a C source code file path.
 *
 * @return true if this is a C source code file path.
 */
//----------------------------------------------------------------------------------------------
bool IsCSource
(
    const std::string& path
);



//--------------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a C++ source code file path.
 *
 * @return true if this is a C++ source code file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsCxxSource
(
    const std::string& path
);



//--------------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a library file path.
 *
 * @return true if this is a library file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsLibrary
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a shared library file path.
 *
 * @return true if this is a shared library file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsSharedLibrary
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the short name for a library by stripping off the directory path, the "lib" file name prefix
 * and the ".so" or ".a" suffix.  E.g., "/usr/local/lib/libfoo.so", the short name is "foo".
 *
 * @return The short name.
 *
 * @throw legato::Exception on error.
 **/
//--------------------------------------------------------------------------------------------------
std::string LibraryShortName
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a component name (which is a directory path,
 * either absolute or relative to one of the search directories provided in searchPathList).
 *
 * @return true if it is a component name.
 */
//--------------------------------------------------------------------------------------------------
bool IsComponent
(
    const std::string& path,    ///< The path to check.
    const std::list<std::string> searchPathList ///< List of component search directory paths.
);


//--------------------------------------------------------------------------------------------------
/**
 * Searches for a component with a given name (which is a directory path, either absolute or
 * relative to one of the search directories provided in searchPathList).
 *
 * @return The path to the component directory, or "" if failed.
 */
//--------------------------------------------------------------------------------------------------
std::string FindComponent
(
    const std::string& name,    ///< Component name.
    const std::list<std::string> searchPathList ///< List of component search directory paths.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a directory in the file system if it doesn't already exist.  Will create any missing
 * parent directories too.  (Equivalent to 'mkdir -P'.)
 */
//--------------------------------------------------------------------------------------------------
void MakeDir
(
    const std::string& path,                                ///< File system path
    mode_t mode = (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)   ///< Permissions.
);


//--------------------------------------------------------------------------------------------------
/**
 * Recursively delete a directory.  That is, delete everything in the directory,
 * then delete the directory itself.
 *
 * If nothing exists at the path, quietly returns without error.
 *
 * If something other than a directory exists at the given path, it's an error.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void CleanDir
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete a file.
 *
 * If nothing exists at the path, quietly returns without error.
 *
 * If something other than a file exists at the given path, it's an error.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void CleanFile
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute file system path of the current working directory.
*/
//--------------------------------------------------------------------------------------------------
std::string GetWorkingDir
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Look for environment variables (specified as "$VAR_NAME" or "${VAR_NAME}") in the path
 * and replace with environment variable contents.
 *
 * @return The converted string.
 **/
//--------------------------------------------------------------------------------------------------
std::string DoEnvVarSubstitution
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Clean all the '/./', '//', and '/../' nodes out of a path, follow symlinks, and makes the
 * path absolute.
 *
 * @return The canonical path.
 *
 * @throw legato::Exception on error.
 **/
//--------------------------------------------------------------------------------------------------
std::string CanonicalPath
(
    const std::string& path
);


}

#endif // FILE_PATH_H_INCLUDE_GUARD
