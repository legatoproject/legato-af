//--------------------------------------------------------------------------------------------------
/**
 * @file file.cpp  Implementation of generic file system access functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <fts.h>
#include <stdlib.h>

#include "defTools.h"


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
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    return (   (stat(path.c_str(), &statBuffer) == 0)
               && S_ISDIR(statBuffer.st_mode)  );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    return (   (stat(path.c_str(), &statBuffer) == 0)
               && S_ISREG(statBuffer.st_mode)  );
}


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether or not anything exists at a given path.
 *
 * @return true if something exists at that path.
 *         false if nothing exists.
 *
 * @throw mk::Exception_t on error.
 */
//--------------------------------------------------------------------------------------------------
bool AnythingExists
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    int result = stat(path.c_str(), &statBuffer);

    if (result == 0)
    {
        return true;
    }

    if ((errno == ENOENT) || (errno == ENOTDIR) || (errno == EOVERFLOW))
    {
        return false;
    }

    int err = errno;

    throw mk::Exception_t(
        mk::format(LE_I18N("stat() failed (%s) for path '%s'."), std::string(strerror(err)), path)
    );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (path::IsAbsolute(path))
    {
        if (FileExists(path) == false)
        {
            return "";
        }

        return path;
    }

    for (const auto& searchPath : searchPaths)
    {
        if (DirectoryExists(searchPath))
        {
            std::string newPath = path::Combine(searchPath, path);

            if (FileExists(newPath))
            {
                return newPath;
            }
        }
    }

    return "";
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (path::IsAbsolute(path))
    {
        if (DirectoryExists(path) == false)
        {
            return "";
        }

        return path;
    }

    for (const auto& searchPath : searchPaths)
    {
        if (DirectoryExists(searchPath))
        {
            std::string newPath = path::Combine(searchPath, path);

            if (DirectoryExists(newPath))
            {
                return newPath;
            }
        }
    }

    return "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a list of files in the specified directory.  Symlinks are returned, but not sub-directories.
 */
//--------------------------------------------------------------------------------------------------
std::list<std::string> ListFiles
(
    const std::string& path   ///< Path to the directory to iterate.
)
//--------------------------------------------------------------------------------------------------
{
    std::list<std::string> files;

    char* pathArrayPtr[] = { const_cast<char*>(path.c_str()), NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    if (ftsPtr == NULL)
    {
        return files;
    }

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
                if (entPtr->fts_level == 1)
                {
                    fts_set(ftsPtr, entPtr, FTS_SKIP);
                }
                break;

            case FTS_F:
            case FTS_SL:
                files.push_back(path::GetLastNode(entPtr->fts_path));
                break;
        }
    }

    fts_close(ftsPtr);

    return files;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    // If it's an absolute path, see if it's a directory containing a file called Component.cdef.
    if (path::IsAbsolute(name))
    {
        if (FileExists(name + "/Component.cdef"))
        {
            return name;
        }
    }
    // Otherwise, it may be a relative path, so, for each directory in the list of component
    // search directories, append the content name and see if there's a directory with that name
    // that contains a file called "Component.cdef".
    else
    {
        for (auto const &searchPath: searchPathList)
        {
            std::string path = searchPath + "/" + name;

            if (FileExists(path + "/Component.cdef"))
            {
                return path;
            }
        }
    }

    return "";
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a directory in the file system if it doesn't already exist.  Will create any missing
 * parent directories too.  (Equivalent to 'mkdir -P'.)
 *
 * @throw mk::Exception_t if the directory doesn't already exist and can't be created.
 */
//--------------------------------------------------------------------------------------------------
void MakeDir
(
    const std::string& path,    ///< File system path
    mode_t mode                 ///< Permissions
)
//--------------------------------------------------------------------------------------------------
{
    if (DirectoryExists(path) == false)
    {
        auto pos = path.rfind('/');

        // If the last character of the path is a '/', skip it.
        if (pos == 0)
        {
            pos = path.rfind('/', 1);
        }

        if (pos != std::string::npos)
        {
            MakeDir(path.substr(0, pos), mode);
        }

        int status = mkdir(path.c_str(), mode);

        if (status != 0)
        {
            int err = errno;

            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to create directory '%s' (%s)"), path, strerror(err))
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively delete a directory.  That is, delete everything in the directory,
 * then delete the directory itself.
 *
 * If nothing exists at the path, quietly returns without error.
 *
 * If something other than a directory exists at the given path, it's an error.
 *
 * @throw mk::Exception_t if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void DeleteDir
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    if (path == "")
    {
        throw mk::Exception_t(LE_I18N("Attempt to delete using an empty path."));
    }

    // Get the status of whatever exists at that path.
    if (stat(path.c_str(), &statBuffer) == 0)
    {
        // If it's a directory, delete it.
        if (S_ISDIR(statBuffer.st_mode))
        {
            std::string commandLine = "rm -rf " + path;
            int result = system(commandLine.c_str());
            if (result != EXIT_SUCCESS)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Failed to execute command '%s' result: %d"),
                               commandLine, result)
                );
            }
        }
        else
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Object at path '%s' is not a directory. Aborting deletion."),
                           path)
            );
        }
    }
    else if (errno != ENOENT)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to delete directory at '%s' (%s)."), path, strerror(errno))
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a file.
 *
 * If nothing exists at the path, quietly returns without error.
 *
 * If something other than a file exists at the given path, it's an error.
 *
 * @throw mk::Exception_t if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void DeleteFile
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    if (path == "")
    {
        throw mk::Exception_t(LE_I18N("Attempt to delete using an empty path."));
    }

    // Get the status of whatever exists at that path.
    if (stat(path.c_str(), &statBuffer) == 0)
    {
        // If it's a regular file, delete it.
        if (S_ISREG(statBuffer.st_mode))
        {
            std::string commandLine = "rm -f " + path;
            int result = system(commandLine.c_str());
            if (result != EXIT_SUCCESS)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Failed to execute command '%s' result: %d"),
                               commandLine, result)
                );
            }
        }
        else
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Object at path '%s' is not a file. Aborting deletion."), path)
            );
        }
    }
    else if (errno != ENOENT)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to delete file at '%s' (%s)."), path, strerror(errno))
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Rename a given old file to a new file name.
 **/
//--------------------------------------------------------------------------------------------------
void RenameFile
(
    std::string oldFileName,
    std::string newFileName
)
{
    int result;
    result = rename(oldFileName.c_str(), newFileName.c_str());

    if (result != 0)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Error in renaming file '%s' to '%s' ('%s')"),
                                       oldFileName, newFileName, strerror(errno))
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove/delete a given file.
 **/
//--------------------------------------------------------------------------------------------------
void RemoveFile
(
    std::string fileName
)
{
    int result;
    result = remove(fileName.c_str());

    if (result != 0)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Error in removing file '%s' ('%s')."),
                                       fileName, strerror(errno))
        );
    }
}

} // namespace file
