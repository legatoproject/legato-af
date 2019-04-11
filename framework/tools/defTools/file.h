//--------------------------------------------------------------------------------------------------
/**
 * @file file.h  File system access functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_FILE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_FILE_H_INCLUDE_GUARD


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace file
{


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
 * Determines whether or not anything exists at a given path.
 *
 * @return true if something exists at that path.
 *         false if nothing exists.
 *
 * @throw legato::Exception on error.
 */
//--------------------------------------------------------------------------------------------------
bool AnythingExists
(
    const std::string& path
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
 * Get a list of files in the specified directory.  Symlinks are returned, but not sub-directories.
 */
//--------------------------------------------------------------------------------------------------
std::list<std::string> ListFiles
(
    const std::string& path   ///< Path to the directory to iterate.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a directory in the file system if it doesn't already exist.  Will create any missing
 * parent directories too.  (Equivalent to 'mkdir -P'.)
 *
 * @throw legato::Exception if the directory doesn't already exist and can't be created.
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
void DeleteDir
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
void DeleteFile
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Rename a file.
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void RenameFile
(
    std::string oldFileName,
    std::string newFileName
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a file.
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveFile
(
    std::string fileName
);


} // namespace file

#endif // LEGATO_DEFTOOLS_FILE_H_INCLUDE_GUARD
