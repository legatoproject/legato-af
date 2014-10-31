//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the FilePath class, which is used to hold the information regarding a single
 * file system object.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

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
)
//--------------------------------------------------------------------------------------------------
{
    if (path.empty())
    {
        return false;
    }

    bool isAbsolute = (path[0] == '/');

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
                    if ((depthCount < 0) && isAbsolute)
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

            case '\0':  // std::string can contain null chars in the middle.
                break;

            // TODO: check for characters not allowed in a file system path?

            default:
                state = OTHER;
                break;
        }
    }

    if (state == TWO_DOTS)
    {
        depthCount -= 1;
        if ((depthCount < 0) && isAbsolute)
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

    if (add.empty())
    {
        return newPath;
    }

    if (   (newPath.back() == '/')
        && (add[0] == '/'))
    {
        return newPath + add.substr(1);
    }

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
 * Make a file system path into a relative path by stripping off leading separators.
 *
 * @return  The relative path.
 */
//--------------------------------------------------------------------------------------------------
std::string MakeRelativePath
(
    const std::string& path     ///< The original path, which could be absolute or relative.
)
//--------------------------------------------------------------------------------------------------
{
    std::string relPath = path;

    while (IsAbsolutePath(relPath))
    {
        // Remove the first character.
        relPath.erase(0, 1);
    }

    return relPath;
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

        if (pos != std::string::npos)
        {
            if (pos == path.size() - suffix.size())
            {
                return true;
            }
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
    const std::string& path,                    ///< The file path.
    const std::list<std::string>& searchPaths   ///< List of directory paths to search in.
)
//--------------------------------------------------------------------------------------------------
{
    std::string actualPath = DoEnvVarSubstitution(path);

    if (legato::IsAbsolutePath(actualPath))
    {
        if (legato::FileExists(actualPath) == false)
        {
            return "";
        }

        return actualPath;
    }

    for (const auto& searchPath : searchPaths)
    {
        if (legato::DirectoryExists(searchPath))
        {
            std::string newPath = CombinePath(searchPath, actualPath);

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
    std::string actualPath = DoEnvVarSubstitution(path);

    if (legato::IsAbsolutePath(actualPath))
    {
        if (legato::DirectoryExists(actualPath) == false)
        {
            return "";
        }

        return actualPath;
    }

    for (const auto& searchPath : searchPaths)
    {
        if (legato::DirectoryExists(searchPath))
        {
            std::string newPath = CombinePath(searchPath, actualPath);

            if (legato::DirectoryExists(newPath))
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
    static const std::list<std::string> suffixes = { ".c" };

    return HasSuffix(path, suffixes);
}



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
)
//--------------------------------------------------------------------------------------------------
{
    // If it ends in one of these extensions, then it's a C++ source code file.
    static const std::list<std::string> suffixes =
        {
            ".cc", ".cp", ".cxx", ".cpp", ".c++", ".C", ".CC", ".CPP"
        };

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
 * Figures out whether or not a given string is a shared library file path.
 *
 * @return true if this is a shared library file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsSharedLibrary
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    // If it ends in ".so" then it's a shared library.
    static const std::list<std::string> suffixes = { ".so" };

    return HasSuffix(path, suffixes);
}


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
)
//--------------------------------------------------------------------------------------------------
{
    // Get just the file name.
    std::string name = GetLastPathNode(path);

    // Strip off the "lib" prefix.
    if (name.compare(0, 3, "lib") != 0)
    {
        throw legato::Exception("Library file name '" + name + "' doesn't start with 'lib'.");
    }
    name = name.substr(3);

    // If it ends in ".so",
    if ( (name.length() > 3) && (name.substr(name.length() - 3).compare(".so") == 0) )
    {
        return name.substr(0, name.length() - 3);
    }

    // If it ends in ".a",
    if ( (name.length() > 2) && (name.substr(name.length() - 2).compare(".a") == 0) )
    {
        return name.substr(0, name.length() - 2);
    }

    throw legato::Exception("Library file path '" + path
                            + "' does not end in either '.a' or '.so'.");
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
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    if (path == "")
    {
        throw legato::Exception("Attempt to delete using an empty path.");
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
                std::stringstream buffer;
                buffer << "Failed to execute command '" << commandLine << "'"
                       << " result: " << result;
                throw legato::Exception(buffer.str());
            }
        }
        else
        {
            throw legato::Exception("Object at path '" + path + "' is not a directory."
                                    " Aborting deletion.");
        }
    }
    else if (errno != ENOENT)
    {
        throw legato::Exception("Failed to delete directory at '" + path + "'"
                                " (" + strerror(errno) + ").");
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
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
void CleanFile
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    struct stat statBuffer;

    if (path == "")
    {
        throw legato::Exception("Attempt to delete using an empty path.");
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
                std::stringstream buffer;
                buffer << "Failed to execute command '" << commandLine << "'"
                       << " result: " << result;
                throw legato::Exception(buffer.str());
            }
        }
        else
        {
            throw legato::Exception("Object at path '" + path + "' is not a file."
                                    " Aborting deletion.");
        }
    }
    else if (errno != ENOENT)
    {
        throw legato::Exception("Failed to delete file at '" + path + "'"
                                " (" + strerror(errno) + ").");
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
)
//--------------------------------------------------------------------------------------------------
{
    std::string result;
    std::string envVarName;

    enum
    {
        NORMAL,
        AFTER_DOLLAR,
        UNBRACKETED_VAR_NAME,
        BRACKETED_VAR_NAME,
    }
    state = NORMAL;

    for (auto i = path.begin(); i != path.end() ; i++)
    {
        switch (state)
        {
            case NORMAL:

                if (*i == '$')
                {
                    envVarName = "";
                    state = AFTER_DOLLAR;
                }
                else
                {
                    result += *i;
                }
                break;

            case AFTER_DOLLAR:

                // Opening curly can be used to start a bracketed environment variable name,
                // which must be terminated by a closing curly.
                if (*i == '{')
                {
                    state = BRACKETED_VAR_NAME;
                    break;
                }
                else
                {
                    state = UNBRACKETED_VAR_NAME;
                }
                // ** FALL THROUGH **

            case UNBRACKETED_VAR_NAME:

                // The first character in the env var name can be an alpha character or underscore.
                // The remaining can be alphanumeric or underscore.
                if (   (isalpha(*i))
                    || (!envVarName.empty() && isdigit(*i))
                    || (*i == '_') )
                {
                    envVarName += *i;
                }
                else
                {
                    // Look up the environment variable, and if found, add its value to the result.
                    const char* envVarPtr = getenv(envVarName.c_str());
                    if (envVarPtr != NULL)
                    {
                        result += envVarPtr;
                    }

                    // Copy into the result string the current character from the source string
                    // (i.e., the one right after the environment variable).
                    result += *i;

                    state = NORMAL;
                }
                break;

            case BRACKETED_VAR_NAME:

                // The first character in the env var name can be an alpha character or underscore.
                // The remaining can be alphanumeric or underscore.
                if (   (isalpha(*i))
                    || (!envVarName.empty() && isdigit(*i))
                    || (*i == '_') )
                {
                    envVarName += *i;
                }
                else if (*i == '}') // Make sure properly terminated with closing curly.
                {
                    // Look up the environment variable, and if found, add its value to the result.
                    const char* envVarPtr = getenv(envVarName.c_str());
                    if (envVarPtr != NULL)
                    {
                        result += envVarPtr;
                    }
                    state = NORMAL;
                }
                else
                {
                    throw legato::Exception("Invalid character inside bracketed environment"
                                            "variable name.");
                }
                break;
        }
    }

    switch (state)
    {
        case NORMAL:
            break;

        case AFTER_DOLLAR:
            throw legato::Exception("Environment variable name missing after '$'.");

        case UNBRACKETED_VAR_NAME:
            // The end of the string terminates the environment variable name.
            // Look up the environment variable, and if found, add its value to the result.
            {
                const char* envVarPtr = getenv(envVarName.c_str());
                if (envVarPtr != NULL)
                {
                    result += envVarPtr;
                }
            }
            break;

        case BRACKETED_VAR_NAME:
            throw legato::Exception("Closing brace missing from environment variable.");
    }

    return result;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    char pathBuff[PATH_MAX];
    char* cannonicalPath = realpath(path.c_str(), pathBuff);

    if (cannonicalPath == NULL)
    {
        throw legato::Exception("Path '" + path + "' is malformed.");
    }

    return std::string(cannonicalPath);
}


}
