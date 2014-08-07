//--------------------------------------------------------------------------------------------------
/**
 * Implementation of functions that are common to both the Component Parser and the
 * Application Parser.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "Parser.h"
#include <string.h>

extern "C"
{
    #include "ParserCommonInternals.h"
}

#include <limits.h>


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
)
//--------------------------------------------------------------------------------------------------
{
    int permissions = 0;
    int i;

    // Check each character and set the appropriate flag.
    // NOTE: We can assume that the first character is '[', the last character is ']', and
    //       the only characters in between are 'r', 'w', 'x', and/or 'p' because that's
    //       enforced by the lexer.
    for (i = 1; stringPtr[i] != ']'; i++)
    {
        switch (stringPtr[i])
        {
            case 'r':
                permissions |= legato::PERMISSION_READABLE;
                break;

            case 'w':
                permissions |= legato::PERMISSION_WRITEABLE;
                break;

            case 'x':
                permissions |= legato::PERMISSION_EXECUTABLE;
                break;

            default:
                fprintf(stderr,
                        "Unexpected character '%c' in permissions string '%s' (lexer bug).\n",
                        stringPtr[i],
                        stringPtr);
                exit(EXIT_FAILURE);
        }
    }

    std::cerr << "** WARNING: File permissions not fully supported yet." << std::endl;

    return permissions;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    char* endPtr = NULL;

    errno = 0;
    long int number = strtol(stringPtr, &endPtr, 0);
    if ((errno == ERANGE) || (number < INT_MIN) || (number > INT_MAX))
    {
        std::string msg = "Number '";
        msg += stringPtr;
        msg += "' is out of range (magnitude too large).";
        throw legato::Exception(msg);
    }

    if (*endPtr != '\0')
    {
        if ((endPtr[0] == 'K') && (endPtr[1] == '\0'))
        {
            number *= 1024;
            if ((number < INT_MIN) || (number > INT_MAX))
            {
                std::string msg = "Number '";
                msg += stringPtr;
                msg += "' is out of range (magnitude too large).";
                throw legato::Exception(msg);
            }
        }
        else
        {
            fprintf(stderr,
                    "Unexpected character '%c' in number '%s' (lexer bug).\n",
                    *endPtr,
                    stringPtr);
            exit(EXIT_FAILURE);
        }
    }

    return (int)number;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    size_t startPos = 0;

    size_t quotePos = string.find('"');
    if (quotePos == string.npos)
    {
        return string;
    }
    else
    {
        std::string result;

        while (quotePos != string.npos)
        {
            result += string.substr(startPos, quotePos - startPos);
            startPos = quotePos + 1;
            quotePos = string.find('"', startPos);
        }
        result += string.substr(startPos);

        return result;
    }
}



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
)
//--------------------------------------------------------------------------------------------------
{
    if (!legato::IsValidPath(path))
    {
        throw legato::Exception("'" + path + "' is not a valid path.");
    }
    if (path.back() == '/')
    {
        throw legato::Exception("'" + path + "' has a trailing slash ('/').");
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (!legato::IsValidPath(path))
    {
        throw legato::Exception("'" + path + "' is not a valid path.");
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (!legato::IsValidPath(path))
    {
        throw legato::Exception("'" + path + "' is not a valid path.");
    }
    if (!legato::IsAbsolutePath(path))
    {
        throw legato::Exception("Destination path inside app must be an absolute path ('"
                                + path + "' is not absolute).");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Strip the trailing slashes from a path, leaving none, unless the one slash is the only character
 * in the string (i.e., won't reduce "/" to "").
 **/
//--------------------------------------------------------------------------------------------------
static void StripTrailingSlashes
(
    std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    size_t pos = path.find_last_not_of('/');

    if (pos != std::string::npos)
    {
        path.erase(pos + 1);
    }
    else
    {
        path.erase(1);
    }
}

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
)
//--------------------------------------------------------------------------------------------------
{
    legato::FileMapping mapping;

    mapping.m_PermissionFlags = yy_GetPermissionFlags(permissions);
    mapping.m_SourcePath = legato::DoEnvVarSubstitution(yy_StripQuotes(sourcePath));
    mapping.m_DestPath = legato::DoEnvVarSubstitution(yy_StripQuotes(destPath));

    yy_CheckRequiredFilePathValidity(mapping.m_SourcePath);
    yy_CheckBindMountDestPathValidity(mapping.m_DestPath);

    if (buildParams.IsVerbose())
    {
        std::cout << "  Importing file '" << mapping.m_SourcePath
                  << "' from the target file system to '" << mapping.m_DestPath
                  << "' " << permissions << " inside the sandbox." << std::endl;
    }

    return mapping;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    legato::FileMapping mapping;

    mapping.m_PermissionFlags = yy_GetPermissionFlags(permissions);
    mapping.m_SourcePath = legato::DoEnvVarSubstitution(yy_StripQuotes(sourcePath));
    mapping.m_DestPath = legato::DoEnvVarSubstitution(yy_StripQuotes(destPath));

    yy_CheckRequiredDirPathValidity(mapping.m_SourcePath);
    yy_CheckBindMountDestPathValidity(mapping.m_DestPath);

    StripTrailingSlashes(mapping.m_SourcePath);

    if (buildParams.IsVerbose())
    {
        std::cout << "  Importing directory '" << mapping.m_SourcePath
                  << "' from the target file system to '" << mapping.m_DestPath
                  << "' " << permissions << " inside the sandbox." << std::endl;
    }

    return mapping;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    legato::FileMapping mapping;

    mapping.m_PermissionFlags = yy_GetPermissionFlags(permissions);
    mapping.m_SourcePath = legato::DoEnvVarSubstitution(yy_StripQuotes(sourcePath));
    mapping.m_DestPath = legato::DoEnvVarSubstitution(yy_StripQuotes(destPath));

    yy_CheckRequiredFilePathValidity(mapping.m_SourcePath);
    yy_CheckBindMountDestPathValidity(mapping.m_DestPath);

    if (buildParams.IsVerbose())
    {
        std::cout << "Adding file '" << mapping.m_SourcePath
                  << "' to the application bundle (to appear at '" << mapping.m_DestPath
                  << "' " << permissions << " in the sandbox)." << std::endl;
    }

    return mapping;
}



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
)
//--------------------------------------------------------------------------------------------------
{
    legato::FileMapping mapping;

    mapping.m_PermissionFlags = legato::PERMISSION_READABLE | legato::PERMISSION_EXECUTABLE;
    mapping.m_SourcePath = legato::DoEnvVarSubstitution(yy_StripQuotes(sourcePath));
    mapping.m_DestPath = legato::DoEnvVarSubstitution(yy_StripQuotes(destPath));

    yy_CheckRequiredDirPathValidity(mapping.m_SourcePath);

    StripTrailingSlashes(mapping.m_SourcePath);

    if (mapping.m_SourcePath == "/")
    {
        throw legato::Exception("Not permitted to bundle the root directory into the app.");
    }

    yy_CheckBindMountDestPathValidity(mapping.m_DestPath);

    if (buildParams.IsVerbose())
    {
        std::cout << "Adding directory '" << mapping.m_SourcePath
                  << "' to the application bundle (to appear at '" << mapping.m_DestPath
                  << "' in the sandbox)." << std::endl;
    }

    return mapping;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an instance of a given component to an executable.
 */
//--------------------------------------------------------------------------------------------------
void legato::parser::AddComponentToExe
(
    legato::App* appPtr,
    legato::Executable* exePtr,
    legato::Component* componentPtr ///< The component object.
)
//--------------------------------------------------------------------------------------------------
{
    // If the component is not already in the application's list of components, add it.
    auto& map = appPtr->ComponentMap();
    auto i = map.find(componentPtr->Path());
    if (i == map.end())
    {
        map[componentPtr->Path()] = componentPtr;
    }

    // Create a new component instance and add it to the executable.
    exePtr->AddComponentInstance(*componentPtr);

    // Recursively pull in all the sub-components too.
    for (auto& mapEntry : componentPtr->SubComponents())
    {
        AddComponentToExe(appPtr, exePtr, mapEntry.second);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an instance of a given component to an executable.
 *
 * @return Pointer to the Component Instance object.
 */
//--------------------------------------------------------------------------------------------------
void legato::parser::AddComponentToExe
(
    legato::App* appPtr,
    legato::Executable* exePtr,
    const std::string& path,    ///< The path to the component.
    const legato::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string resolvedPath = legato::FindComponent(path, buildParams.ComponentDirs());

    legato::Component* componentPtr = Component::FindComponent(resolvedPath);

    // If the component has not yet been parsed,
    if (componentPtr == NULL)
    {
        // Create a new Component object for this component name.
        componentPtr = legato::Component::CreateComponent(resolvedPath);

        // Tell the parser to parse it
        legato::parser::ParseComponent(componentPtr, buildParams);
    }

    // Recursively add it and all its sub-components to the executable and the app.
    AddComponentToExe(appPtr, exePtr, componentPtr);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the API object for a given .api file.
 *
 * @return A pointer to the API object.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
legato::Api_t* legato::parser::GetApiObject
(
    const std::string& filePath,                ///< [in] Absolute file system path to API file.
    const legato::BuildParams_t& buildParams    ///< [in] Build parameters.
)
//--------------------------------------------------------------------------------------------------
{
    // If there's already an API object for this file path, return that.
    legato::Api_t* apiPtr = legato::Api_t::GetApiPtr(filePath);
    if (apiPtr != NULL)
    {
        return apiPtr;
    }

    // Create a new object for this path.
    apiPtr = new legato::Api_t(filePath);

    // Use ifgen to determine the dependencies and compute the hash.
    std::stringstream commandLine;
    commandLine << "ifgen --hash " << filePath;

    // Specify all the interface search directories as places to look for interface files.
    for (auto dir : buildParams.InterfaceDirs())
    {
        commandLine << " --import-dir " << dir;
    }

    if (buildParams.IsVerbose())
    {
        std::cout << commandLine.str() << std::endl;
    }

    FILE* output = popen(commandLine.str().c_str(), "r");

    if (output == NULL)
    {
        throw Exception("Could not exec ifgen to generate an interface hash.");
    }

    char buffer[1024] = { 0 };
    static const size_t bufferSize = sizeof(buffer);

    for (;;)
    {
        if (fgets(buffer, bufferSize, output) != buffer)
        {
            pclose(output); // Don't care about return code.

            std::stringstream msg;
            msg << "Failed to receive the interface hash from ifgen. Errno = ";
            msg << strerror(errno);
            throw Exception(msg.str());
        }

        // Remove any trailing newline character.
        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }

        // If we received "importing foo.api", then add "foo.api" to the list of dependencies.
        const char importingString[] = "importing ";
        if (strncmp(buffer, importingString, sizeof(importingString) - 1) == 0)
        {
            if (buffer[sizeof(importingString)] == '\0')
            {
                std::cerr << "WARNING: ifgen reported an empty dependency." << std::endl;
            }
            else
            {
                std::string otherApiFilePath;
                otherApiFilePath = legato::FindFile((buffer + sizeof(importingString) - 1),
                                                    buildParams.InterfaceDirs());

                if (!IsAbsolutePath(otherApiFilePath))
                {
                    otherApiFilePath = CombinePath(GetContainingDir(filePath), otherApiFilePath);
                }

                if (buildParams.IsVerbose())
                {
                    std::cout << "    API '" << filePath << "' depends on API '"
                              << otherApiFilePath << "'" << std::endl;
                }

                try
                {
                    apiPtr->AddDependency(GetApiObject(otherApiFilePath, buildParams));
                }
                catch (legato::Exception& e)
                {
                    // Close the connection and collect the exit code from ifgen.
                    int result = pclose(output);
                    if (result == -1)
                    {
                        std::stringstream msg;
                        msg << "ifgen failed. errno = ";
                        msg << strerror(errno);
                        throw Exception(msg.str());
                    }
                    else if (!WIFEXITED(result) || (WEXITSTATUS(result) != EXIT_SUCCESS))
                    {
                        // Rely on ifgen's error message to help the user.  Don't confuse them
                        // with whatever exception message we got from trying to add some
                        // garbage dependency string onto the API's dependency list.
                        throw Exception("ifgen failed.");
                    }
                    else
                    {
                        throw e;
                    }
                }
            }
        }
        else
        {
            // Store the hash in the new API object.
            apiPtr->Hash(buffer);

            if (buildParams.IsVerbose())
            {
                std::cout << "    API '" << filePath << "' has hash '"
                          << buffer << "'" << std::endl;
            }

            // Close the connection and collect the exit code from ifgen.
            int result = pclose(output);
            if (result == -1)
            {
                std::stringstream msg;
                msg << "ifgen failed. errno = ";
                msg << strerror(errno);
                throw Exception(msg.str());
            }

            // DONE.
            return apiPtr;
        }
    }
}

