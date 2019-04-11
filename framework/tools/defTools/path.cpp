//--------------------------------------------------------------------------------------------------
/**
 * @file path.cpp  Implementation of generic file system path utility functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include "defTools.h"


namespace path
{


//--------------------------------------------------------------------------------------------------
/**
 * @return true if the path is absolute (starts with a '/').
 */
//--------------------------------------------------------------------------------------------------
bool IsAbsolute
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    return ((!path.empty()) && (path[0] == '/'));
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes matching outer quotation marks (' or ") from a path, if it has any.
 *
 * @return The path without the quotes.
 */
//--------------------------------------------------------------------------------------------------
std::string Unquote
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    auto pathLen = path.length();

    if ((pathLen >= 2) && (   ((path.front() == '\'') && (path.back() == '\''))
                           || ((path.front() == '"') && (path.back() == '"'))  )   )
    {
        return path.substr(1, pathLen - 2);
    }
    else
    {
        return path;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Insert a backslash ('\') character in front of any quotation marks ('"') characters.
 *
 * @return The new string.
 **/
//--------------------------------------------------------------------------------------------------
std::string EscapeQuotes
(
    const std::string& str
)
//--------------------------------------------------------------------------------------------------
{
    size_t pos = str.find('"');

    // If there are no quotes, just return the string as-is.
    if (pos == str.npos)
    {
        return str;
    }

    // Create a new string and copy in everything before the first ".
    std::string result = str.substr(0, pos);

    // Append a backslash.
    result += '\\';

    // Find the position of the next " in str or the end of the string.
    size_t nextPos = str.find('"', pos + 1);

    // Append everything from the first " to the next one (or end of string).
    result += str.substr(pos, nextPos);

    // While we keep finding more quotes,
    while (nextPos != str.npos)
    {
        // Append another backslash to the end of the result string.
        result += '\\';

        // Find the next " in str or the end of str.
        pos = nextPos;
        nextPos = str.find('"', pos + 1);

        // Append that to the result.
        result += str.substr(pos, nextPos);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reduces a path to its minimum length by cleaning out any instances of "./" and "//" and any
 * non-leading instances of "../".
 *
 * @return The cleaned path.
 *
 * @throw mk::Exception_t if the path is invalid.
 */
//--------------------------------------------------------------------------------------------------
std::string Minimize
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    std::string result;

    // true if the path is absolute.  Assume relative to start.
    bool isAbsolute = false;

    // Counter used to count how deep in the path the current node is.
    // 0 = the shallowest point known.  If the path is absolute, this is the root directory and
    // no ".." nodes are allowed from here.
    int depthCount = 0;

    // States.  The algorithm is based on a finite state machine.
    enum
    {
        START,      ///< We haven't processed any characters yet.
        SLASH,      ///< The last character we saw was a slash.
        ONE_DOT,    ///< Since start of path node we have seen one '.'.
        TWO_DOTS,   ///< Since start of path node we have seen "..".
        NAME        ///< Since start of path node we have seen a node name other than "." or ".."
    }
    state = START;

    // State machine function that processes a slash (/) character and updates the result string.
    auto addSlash = [&state, &result, &depthCount, &isAbsolute, &path]()
        {
            switch (state)
            {
                case START:
                    // This is an absolute path.
                    isAbsolute = true;
                    state = SLASH;
                    result += '/';
                    break;

                case SLASH:
                case ONE_DOT:
                    // Don't do anything in the SLASH or ONE_DOT states to skip extra slashes
                    // and "./" nodes.
                    break;

                case TWO_DOTS:
                    // The path node is ".."
                    if (depthCount == 0)
                    {
                        // If we are at the root of an absolute path, and they've tried to go up past
                        // the root, then throw an exception.
                        if (isAbsolute)
                        {
                            throw mk::Exception_t(
                                mk::format(LE_I18N("File system path '%s' attempts to leave "
                                                   "the root directory."), path)
                            );
                        }

                        // If we are at the shallowest depth we've seen so far in a relative path,
                        // then we just append the ".." node to make it shallower, and this now
                        // becomes the new zero depth.
                        // E.g., "" -> "../" and "../" -> "../../"
                        result += "../";
                    }
                    else // (depthCount > 0)
                    {
                        // Since we are not at the shallowest point we've seen so far, we strip
                        // off everything back to the previous '/' or the beginning of the result
                        // if there is only the trailing '/' character in the result.
                        // E.g., "/foo/" -> "/" and "foo/" -> "" and "foo/bar/" -> "foo/"
                        size_t secondLastSlashPos = result.rfind('/', result.length() - 2);
                        if (secondLastSlashPos == std::string::npos)
                        {
                            result = "";
                        }
                        else
                        {
                            result = result.substr(0, secondLastSlashPos + 1);
                        }
                        depthCount--;
                    }
                    break;

                case NAME:
                    // The slash has terminated a path node name.
                    // Add the slash to the result.
                    result += '/';
                    break;
            }

            // After seeing a '/', the state is always SLASH.
            state = SLASH;
        };

    // State machine function that processes a dot (.) character and updates the result string.
    auto addDot = [&state, &result, &depthCount]()
        {
            switch (state)
            {
                case START:
                case SLASH:
                    // We've seen one '.', wait to see what comes next.
                    state = ONE_DOT;
                    break;

                case ONE_DOT:
                    // We've seen "..", wait to see what comes next.
                    state = TWO_DOTS;
                    break;

                case TWO_DOTS:
                    // Three dots or more can be used as a regular file or directory name.
                    state = NAME;

                    // Add the three dots to the result.
                    result += "...";

                    // We are now one node deeper than we were before.
                    depthCount++;

                    break;

                case NAME:
                    // Append the dot to the path node name.
                    result += '.';
                    break;
            }
        };

    // State machine function that processes a character that is not a dot or a slash and updates the result string.
    auto addOtherChar = [&state, &result, &depthCount](char c)
        {
            // Since we have seen something other than a '.' or a '/' it is definitely a
            // part of a path node's name.
            switch (state)
            {
                case START:
                case SLASH:
                    // We are now one node deeper than we were.
                    depthCount++;

                    // Add the character to the result.
                    result += c;
                    break;

                case ONE_DOT:
                    // The path node name started with a '.', which we have not yet added to the
                    // result, so add that now.
                    result += '.';
                    result += c;

                    // We are now one node deeper than we were.
                    depthCount++;
                    break;

                case TWO_DOTS:
                    // The path node name started with a '..', which we have not yet added to the
                    // result, so add that now.
                    result += "..";
                    result += c;

                    // We are now one node deeper than we were.
                    depthCount++;
                    break;

                case NAME:
                    // Add the character to the result.
                    result += c;
            }

            // The state is always NAME after something other than '.' or '/' has been seen.
            state = NAME;
        };

    // Process all the characters in the original path, one-by-one.
    for (auto c : path)
    {
        switch (c)
        {
            case '/':
                addSlash();
                break;

            case '.':
                addDot();
                break;

            case '\0':  // std::string can contain null chars in the middle, but that's not valid
                        // in a file system path
                throw mk::Exception_t(LE_I18N("Invalid (null) character in the middle of a path."));
                break;

            default:
                addOtherChar(c);
                break;
        }
    }

    // End of string reached.
    switch (state)
    {
        case START:
        case SLASH:
        case ONE_DOT:
        case NAME:
            // Nothing more to do.
            break;

        case TWO_DOTS:
            // Must execute the ".." by going back up one level.
            if (depthCount == 0)
            {
                if (isAbsolute)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("File system path '%s' attempts to leave "
                                           "the root directory."), path)
                    );
                }
                result += "..";
            }
            else // (depthCount > 0)
            {
                result = GetContainingDir(result);
            }
            break;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Concatenate two file system paths together.
 *
 * @return  The combined path.
 */
//--------------------------------------------------------------------------------------------------
std::string Combine
(
    const std::string& base,  ///< The left-hand part of the path.
    const std::string& add    ///< The right-hand part of the path.
)
{
    if (add.empty())
    {
        return base;
    }

    if (base.empty())
    {
        // Remove any leading '/'s from the beginning of the right-hand part of the path.
        return add.substr(add.find_first_not_of('/'));
    }

    return Minimize(base + '/' + add);
}


//--------------------------------------------------------------------------------------------------
/**
 * Make a file system path into an absolute path.
 *
 * @return  The absolute path.
 */
//--------------------------------------------------------------------------------------------------
std::string MakeAbsolute
(
    const std::string& path     ///< The original path, which could be absolute or relative.
)
//--------------------------------------------------------------------------------------------------
{
    if (IsAbsolute(path))
    {
        return path;
    }
    else
    {
        return Combine(GetCurrentDir(), path);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Make a file system path into a relative path by stripping off leading separators.
 *
 * @return  The relative path.
 */
//--------------------------------------------------------------------------------------------------
std::string MakeRelative
(
    const std::string& path     ///< The original path, which could be absolute or relative.
)
//--------------------------------------------------------------------------------------------------
{
    std::string relPath = path;

    while (IsAbsolute(relPath))
    {
        // Remove the first character.
        relPath.erase(0, 1);
    }

    return relPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Clean all the '/./', '//', and '/../' nodes out of a path, follow symlinks, and makes the
 * path absolute.
 *
 * @return The canonical path.
 *
 * @throw mk::Exception_t on error.
 **/
//--------------------------------------------------------------------------------------------------
std::string MakeCanonical
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    char pathBuff[PATH_MAX];
    char* cannonicalPath = realpath(path.c_str(), pathBuff);

    if (cannonicalPath == NULL)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Path '%s' does not exist or is malformed."), path)
        );
    }

    return std::string(cannonicalPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * @return The path of the directory containing this path ("." if the path contains no slashes).
 *
 * @throw mk::Exception_t on error.
 */
//--------------------------------------------------------------------------------------------------
std::string GetContainingDir
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    if (path.empty())
    {
        return std::string("..");
    }

    // Ignore trailing slashes.
    size_t pos = path.find_last_not_of('/');
    if (pos == std::string::npos)
    {
        throw mk::Exception_t(LE_I18N("Directory paths outside the root directory are not"
                                      " permitted."));
    }

    // Find the last '/' in the path (excluding the trailing slashes).
    pos = path.rfind('/', pos);

    if (pos == std::string::npos)
    {
        if (path == ".")
        {
            return std::string("..");
        }

        if (path == "..")
        {
            return "../..";
        }

        return ".";
    }

    // If ends in "/.." or "/.",
    if (   ((pos == 3) && (path.substr(pos) == "/.."))
        || ((pos == 2) && (path.substr(pos) == "/."))  )
    {
        return Minimize(path + "/..");
    }

    return path.substr(0, pos);
}



//--------------------------------------------------------------------------------------------------
/**
 * @return The last part of a file path (e.g., just the name of a file, with no directories
 * or slashes in front of it).
 */
//--------------------------------------------------------------------------------------------------
std::string GetLastNode
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
 * @return The file name extension at the end of the file name (e.g., ".c", ".so", etc.), or an
 *         empty string ("") if the path doesn't have a file name extension.
 */
//--------------------------------------------------------------------------------------------------
std::string GetFileNameExtension
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    auto pos = path.rfind('.');

    if (pos != std::string::npos)
    {
        std::string suffix = path.substr(pos);

        // Make sure the last '.' isn't before the last '/'.
        if (suffix.find('/') == std::string::npos)
        {
            return suffix;
        }
    }

    return "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given path has one of the suffixes in a given list of suffixes.
 *
 * @return  The suffix found, or an emptry string ("") if not found.
 */
//--------------------------------------------------------------------------------------------------
std::string HasSuffix
(
    const std::string& path,
    const std::list<std::string>& suffixList
)
//--------------------------------------------------------------------------------------------------
{
    for (auto const &suffix: suffixList)
    {
        if (HasSuffix(path, suffix))
        {
            return suffix;
        }
    }

    return "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given path has a given suffix.
 *
 * @return  true if it has that suffix.
 */
//--------------------------------------------------------------------------------------------------
bool HasSuffix
(
    const std::string& path,
    const std::string& suffix
)
//--------------------------------------------------------------------------------------------------
{
    auto pos = path.rfind(suffix);

    if (pos != std::string::npos)
    {
        if (pos == path.size() - suffix.size())
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a given suffix from the end of a file path.
 *
 * @return  The path, with the suffix removed.
 *
 * @throw mk::Exception_t if the path doesn't have the given suffix.
 */
//--------------------------------------------------------------------------------------------------
std::string RemoveSuffix
(
    const std::string& path,
    const std::string& suffix
)
//--------------------------------------------------------------------------------------------------
{
    if (!path::HasSuffix(path, suffix))
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Path '%s' does not end in '%s'."), path, suffix)
        );
    }

    return path.substr(0, (path.length() - suffix.length()));
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

    return (HasSuffix(path, suffixes) != "");
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

    return (HasSuffix(path, suffixes) != "");
}


//--------------------------------------------------------------------------------------------------
/**
 * Figures out whether or not a given string is a Python source code file path.
 *
 * @return true if this is a Python source code file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsPythonSource
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    // If it ends in ".py", then it's a Python source code file.
    static const std::list<std::string> suffixes = { ".py" };

    return (HasSuffix(path, suffixes) != "");
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
    // If it ends in ".a" or a ".so" or ".so.*" then it's a library.
    static const std::list<std::string> suffixes = { ".a", ".so" };

    return (   (HasSuffix(path, suffixes) != "")
            || (path.find(".so.") != std::string::npos) );
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

    return (HasSuffix(path, suffixes) != "");
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the short name for a library by stripping off the directory path, the "lib" file name prefix
 * and the ".so" or ".a" suffix.  E.g., "/usr/local/lib/libfoo.so", the short name is "foo".
 *
 * @return The short name.
 *
 * @throw mk::Exception_t on error.
 **/
//--------------------------------------------------------------------------------------------------
std::string GetLibShortName
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    // Get just the file name.
    std::string name = GetLastNode(path);

    // Strip off the "lib" prefix.
    if (name.compare(0, 3, "lib") != 0)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Library file name '%s' doesn't start with 'lib'."), name)
        );
    }
    name = name.substr(3);

    // If it ends in ".so",
    if ( (name.length() > 3) && (name.substr(name.length() - 3).compare(".so") == 0) )
    {
        return name.substr(0, name.length() - 3);
    }

    // If it ends in ".so.*" (e.g., "libfoo.so.2"),
    auto pos = name.find(".so.");
    if (pos != std::string::npos)
    {
        return name.substr(0, pos);
    }

    // If it ends in ".a",
    if ( (name.length() > 2) && (name.substr(name.length() - 2).compare(".a") == 0) )
    {
        return name.substr(0, name.length() - 2);
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Library file path '%s' does not appear to be either '.a' or '.so'."),
                   path)
    );
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute file system path of the current directory.
*/
//--------------------------------------------------------------------------------------------------
std::string GetCurrentDir
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char* dirPtr = get_current_dir_name();

    std::string dirPath = dirPtr;

    free(dirPtr);

    return dirPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert a name into one that is safe for use as identifiers in programming language code
 * by replacing all unsafe characters with underscores.
 *
 * @return  The identifier-safe name.
 *
 * @throw   mk::Exception if all the characters in the original name had to be replaced.
 */
//--------------------------------------------------------------------------------------------------
std::string GetIdentifierSafeName
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    std::string result;
    size_t meaningfulCharCount = 0;
    size_t length = name.length();

    if (length == 0)
    {
        throw mk::Exception_t(LE_I18N("Empty name cannot be made identifier-safe."));
    }

    // The first character of the identifier cannot be a digit.
    // NOTE: Don't use isalpha() because its output is locale-dependent.
    char c = name[0];
    if (isupper(c) || islower(c))
    {
        result.push_back(c);
        meaningfulCharCount++;
    }

    // The rest of the identifier can include digits.
    for (size_t i = 1; i < length; i++)
    {
        // NOTE: Don't use isalnum() because its output is locale-dependent.
        c = name[i];
        if (isdigit(c) || isupper(c) || islower(c))
        {
            meaningfulCharCount++;
        }
        else
        {
            c = '_';
        }
        result.push_back(c);
    }

    // Make sure the identifier-safe name contains some meaningful characters
    // (i.e., is not just a bunch of underscores).
    if (meaningfulCharCount == 0)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Name '%s' contained no characters safe for use in an identifier."),
                       name)
        );
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Concatenation operator.
 *
 * @return A new path object.
 **/
//--------------------------------------------------------------------------------------------------
Path_t Path_t::operator+
(
    const std::string& add
)
const
//--------------------------------------------------------------------------------------------------
{
    return Path_t(Combine(str, add));
}


//--------------------------------------------------------------------------------------------------
/**
 * Append operator.
 *
 * @return Reference to the updated path object.
 **/
//--------------------------------------------------------------------------------------------------
Path_t& Path_t::operator+=
(
    const std::string& add
)
//--------------------------------------------------------------------------------------------------
{
    str = Combine(str, add);
    return *this;
}


} // namespace path
