//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the FilePath class, which is used to hold the information regarding a single
 * file system object.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * @return true if the path is valid (not empty and doesn't contain ".." elements that
 *              take it above its starting point).
 */
//--------------------------------------------------------------------------------------------------
bool IsValidPath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    if (path.empty())
    {
        return false;
    }

    int depthCount = 0;

    enum
    {
        SLASH,
        ONE_DOT,
        TWO_DOTS,
        OTHER
    }
    state = SLASH;

    for (auto c : path)
    {
        switch (c)
        {
            case '/':
                if (state == TWO_DOTS)
                {
                    depthCount -= 1;
                    if (depthCount < 0)
                    {
                        return false;
                    }
                }
                state = SLASH;
                break;

            case '.':
                if (state == SLASH)
                {
                    state = ONE_DOT;
                }
                else if (state == ONE_DOT)
                {
                    state = TWO_DOTS;
                }
                else if (state == TWO_DOTS)
                {
                    state = OTHER;  // Three dots or more doesn't have special meaning.
                }
                break;

            default:
                state = OTHER;
                break;
        }
    }

    if (state == TWO_DOTS)
    {
        depthCount -= 1;
        if (depthCount < 0)
        {
            return false;
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * @return true if the path is absolute (starts with a '/').
 */
//--------------------------------------------------------------------------------------------------
bool IsAbsolutePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    return ((!path.empty()) && (path[0] == '/'));
}


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
)
{
    std::string newPath = base;

    if (   (newPath.back() != '/')
        && (add[0] != '/'))
    {
        newPath += "/";
    }

    return newPath + add;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (IsAbsolutePath(path))
    {
        return path;
    }
    else
    {
        return CombinePath(GetWorkingDir(), path);
    }
}


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
 * @return The path of the directory containing this path, or "" if that can't be determined.
 */
//--------------------------------------------------------------------------------------------------
std::string GetContainingDir
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    auto pos = path.rfind('/');

    if (pos == std::string::npos)
    {
        return ".";
    }

    if (pos == 0)
    {
        return "/";
    }

    return path.substr(0, pos);
}



//--------------------------------------------------------------------------------------------------
/**
 * @return The last part of a file path (e.g., just the name of a file, with no directories
 * or slashes in front of it).
 */
//--------------------------------------------------------------------------------------------------
std::string GetLastPathNode
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    // Work back to the last '/' or the beginning of the string, whichever comes first.
    size_t slashPos = path.rfind('/');
    if (slashPos == std::string::npos)
    {
        // No slash.  Return the whole string.
        return path;
    }

    else
    {
        return path.c_str() + slashPos + 1;
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    for (auto suffix: suffixList)
    {
        auto pos = path.rfind(suffix);

        if (pos == path.size() - suffix.size())
        {
            return true;
        }
    }

    return false;
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
    const std::string& name,                    ///< The file path.
    const std::list<std::string>& searchPaths   ///< List of directory paths to search in.
)
//--------------------------------------------------------------------------------------------------
{
    if (legato::IsAbsolutePath(name))
    {
        if (legato::FileExists(name) == false)
        {
            return "";
        }

        return name;
    }

    for (const auto& path : searchPaths)
    {
        if (legato::DirectoryExists(path))
        {
            std::string newPath = CombinePath(path, name);

            if (legato::FileExists(newPath))
            {
                return newPath;
            }
        }
    }

    return "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a C source code file path.
 *
 * @return true if this is a C source code file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsCSource
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    // If it ends in ".c" or ".C", then it's a C source code file.
    static const std::list<std::string> suffixes = { ".c", ".C" };

    return HasSuffix(path, suffixes);
}



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
)
//--------------------------------------------------------------------------------------------------
{
    // If it ends in ".a" or a ".so" then it's a library.
    static const std::list<std::string> suffixes = { ".a", ".so" };

    return HasSuffix(path, suffixes);
}



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
    const std::string& name,    ///< The path to check.
    const std::list<std::string> searchPathList ///< List of component search directory paths.
)
//--------------------------------------------------------------------------------------------------
{
    return (FindComponent(name, searchPathList) != "");
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
    if (IsAbsolutePath(name))
    {
        if (legato::FileExists(name + "/Component.cdef"))
        {
            return name;
        }
    }
    // Otherwise, it may be a relative path, so, for each directory in the list of component
    // search directories, append the content name and see if there's a directory with that name
    // that contains a file called "Component.cdef".
    else
    {
        for (auto searchPath: searchPathList)
        {
            std::string path = searchPath + "/" + name;

            if (legato::FileExists(path + "/Component.cdef"))
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

        if (pos != std::string::npos)
        {
            MakeDir(path.substr(0, pos), mode);
        }

        int status = mkdir(path.c_str(), mode);

        if (status != 0)
        {
            int err = errno;

            std::string msg = "Failed to create directory '";
            msg += path;
            msg += "' (";
            msg += strerror(err);
            msg += ")";

            throw Exception(msg);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute file system path of the current working directory.
*/
//--------------------------------------------------------------------------------------------------
std::string GetWorkingDir
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char* workingDir = get_current_dir_name();

    std::string newPath = workingDir;

    free(workingDir);

    return newPath;
}



}
