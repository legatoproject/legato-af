//--------------------------------------------------------------------------------------------------
/**
 * @file path.h   File system path handling functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PATH_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PATH_H_INCLUDE_GUARD


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes matching outer quotation marks (' or ") from a path.
 *
 * @return The path without the quotes.
 */
//--------------------------------------------------------------------------------------------------
std::string Unquote
(
    const std::string& path
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Concatenate an arbitrary number of file system paths together.
 *
 * @return  The combined path.
 */
//--------------------------------------------------------------------------------------------------
template <typename... args_t>
std::string Combine
(
    const std::string& base,
    const std::string& first,
    const args_t&... rest
)
//--------------------------------------------------------------------------------------------------
{
    return Combine(Combine(base, first), rest...);
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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Clean all the '/./', '//', and '/../' nodes out of a path, follow symlinks, and make the
 * path absolute.
 *
 * @return The canonical path.
 *
 * @throw legato::Exception on error.
 **/
//--------------------------------------------------------------------------------------------------
std::string MakeCanonical
(
    const std::string& path
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * @return The last part of a file path (e.g., just the name of a file, with no directories
 * or slashes in front of it).
 */
//--------------------------------------------------------------------------------------------------
std::string GetLastNode
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * @return The file name extension at the end of the file name (e.g., ".c", ".so", etc.), or an
 *         empty string ("") if the path doesn't have a file name extension.
 */
//--------------------------------------------------------------------------------------------------
std::string GetFileNameExtension
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given path has one of the suffixes in a given list of suffixes.
 *
 * @return  The suffix found, or an empty string ("") if not found.
 */
//--------------------------------------------------------------------------------------------------
std::string HasSuffix
(
    const std::string& path,
    const std::list<std::string>& suffixList
);


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
);


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
 * Figures out whether or not a given string is a Python source code file path.
 *
 * @return true if this is a Python source code file path.
 */
//--------------------------------------------------------------------------------------------------
bool IsPythonSource
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
std::string GetLibShortName
(
    const std::string& path
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute file system path of the current directory.
*/
//--------------------------------------------------------------------------------------------------
std::string GetCurrentDir
(
    void
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Path object.  Provides better syntax for long path concatenation operations.
 **/
//--------------------------------------------------------------------------------------------------
struct Path_t
{
    std::string str;    ///< Contains the path string.

    Path_t operator+(const std::string& add) const;
    Path_t& operator+=(const std::string& add);

    Path_t(const char* s): str(s) {}
    Path_t(const std::string& s): str(s) {}
    Path_t(std::string&& s): str(std::move(s)) {}
};


} // namespace path

#endif // LEGATO_DEFTOOLS_PATH_H_INCLUDE_GUARD
