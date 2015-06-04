//--------------------------------------------------------------------------------------------------
/**
 * @file modellerCommon.cpp
 *
 * Functions shared by multiple modeller modules.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include <stdlib.h>
#include <climits>

#include "modellerCommon.h"

namespace modeller
{


//--------------------------------------------------------------------------------------------------
/**
 * Verifies that all client-side interfaces of an application have either been bound to something
 * or marked as an external interface to be bound at the system level.  Will auto-bind any unbound
 * le_cfg or le_wdog interfaces it finds.  Also checks that client and server side of bindings
 * implement the same API.
 *
 * @throw mk::Exception_t if any client-side interface is found to be unsatisfied.
 */
//--------------------------------------------------------------------------------------------------
void EnsureClientInterfacesSatisfied
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto exePtr : appPtr->executables)
    {
        for (auto componentInstancePtr : exePtr->componentInstances)
        {
            for (auto ifInstancePtr : componentInstancePtr->clientApis)
            {
                if ((ifInstancePtr->bindingPtr == NULL) && (ifInstancePtr->isExternal == false))
                {
                    // If this is an le_cfg API, then bind it to the one offered by the root user.
                    if (ifInstancePtr->ifPtr->internalName == "le_cfg")
                    {
                        auto bindingPtr = new model::Binding_t();
                        bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
                        bindingPtr->clientIfName = ifInstancePtr->name;
                        bindingPtr->serverAgentName = "root";
                        bindingPtr->serverIfName = "le_cfg";
                        ifInstancePtr->bindingPtr = bindingPtr;
                        appPtr->bindings.push_back(bindingPtr);
                    }
                    else if (ifInstancePtr->ifPtr->internalName == "le_wdog")
                    {
                        auto bindingPtr = new model::Binding_t();
                        bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
                        bindingPtr->clientIfName = ifInstancePtr->name;
                        bindingPtr->serverAgentName = "root";
                        bindingPtr->serverIfName = "le_wdog";
                        ifInstancePtr->bindingPtr = bindingPtr;
                        appPtr->bindings.push_back(bindingPtr);
                    }
                    else
                    {
                        throw mk::Exception_t("Client interface '"
                                                   + ifInstancePtr->ifPtr->internalName
                                                   + "' of component '"
                                                   + componentInstancePtr->componentPtr->name
                                                   + "' in executable '" + exePtr->name
                                                   + "' is unsatisfied. It must either be declared"
                                                   " an external (inter-app) required interface"
                                                   " (in a \"requires: api:\" section in the .adef)"
                                                   " or be bound to a server side interface"
                                                   " (in the \"bindings:\" section of the .adef).");
                    }
                }
                else if (ifInstancePtr->bindingPtr != NULL)
                {
                    /// @todo When possible, double check that the client and server are using the
                    ///       same .api file.
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set permissions inside a Permissions_t object based on the contents of a FILE_PERMISSIONS token.
 */
//--------------------------------------------------------------------------------------------------
void GetPermissions
(
    model::Permissions_t& permissions,  ///< [OUT]
    const parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Permissions string always starts with '[' and ends with ']'.
    // Could contain 'r', 'w', or 'x'.
    const std::string& permissionsText = tokenPtr->text;
    for (int i = 1; permissionsText[i] != ']'; i++)
    {
        switch (permissionsText[i])
        {
            case 'r':
                permissions.SetReadable();
                break;
            case 'w':
                permissions.SetWriteable();
                break;
            case 'x':
                permissions.SetExecutable();
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given bundled file or directory in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetBundledItem
(
    const parseTree::TokenList_t* itemPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto fileSystemObjectPtr = new model::FileSystemObject_t(itemPtr);

    const std::string* srcPathPtr;
    const std::string* destPathPtr;

    // Optionally, there may be permissions.
    auto firstTokenPtr = itemPtr->Contents()[0];
    if (firstTokenPtr->type == parseTree::Token_t::FILE_PERMISSIONS)
    {
        srcPathPtr = &(itemPtr->Contents()[1]->text);
        destPathPtr = &(itemPtr->Contents()[2]->text);

        GetPermissions(fileSystemObjectPtr->permissions, firstTokenPtr);
    }
    // If no permissions, default to read-only.
    else
    {
        srcPathPtr = &(firstTokenPtr->text);
        destPathPtr = &(itemPtr->Contents()[1]->text);

        fileSystemObjectPtr->permissions.SetReadable();
    }

    fileSystemObjectPtr->srcPath = path::Unquote(envVars::DoSubstitution(*srcPathPtr));
    fileSystemObjectPtr->destPath = path::Unquote(envVars::DoSubstitution(*destPathPtr));

    // If the destination path ends in a slash, append the last path node from the source to it.
    if (fileSystemObjectPtr->destPath.back() == '/')
    {
        fileSystemObjectPtr->destPath += path::GetLastNode(fileSystemObjectPtr->srcPath);
    }

    return fileSystemObjectPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given required file or directory in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetRequiredFileOrDir
(
    const parseTree::TokenList_t* itemPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto srcPathTokenPtr = itemPtr->Contents()[0];
    auto destPathTokenPtr = itemPtr->Contents()[1];

    std::string srcPath = path::Unquote(envVars::DoSubstitution(srcPathTokenPtr->text));
    std::string destPath = path::Unquote(envVars::DoSubstitution(destPathTokenPtr->text));

    // The source path must not end in a slash.
    if (srcPath.back() == '/')
    {
        srcPathTokenPtr->ThrowException("Required item's path must not end in a '/'.");
    }

    // If the destination path ends in a slash, append the last path node from the source to it.
    if (destPath.back() == '/')
    {
        destPath += path::GetLastNode(srcPath);
    }

    auto fileSystemObjPtr = new model::FileSystemObject_t(itemPtr);
    fileSystemObjPtr->srcPath = srcPath;
    fileSystemObjPtr->destPath = destPath;

    // Note: Items bind-mounted into the sandbox from outside have the permissions they
    //       have inside the target filesystem.  This cannot be changed by the app.

    return fileSystemObjPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the integer value from a simple (name: value) section and verifies that it is
 * non-negative.
 *
 * @return the value.
 *
 * @throw mkToolsException if out of range.
 */
//--------------------------------------------------------------------------------------------------
size_t GetNonNegativeInt
(
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto valueTokenPtr = sectionPtr->Contents()[0];
    char* endPtr;
    errno = 0;
    size_t result = strtoul(valueTokenPtr->text.c_str(), &endPtr, 0);
    if (errno != 0)
    {
        std::stringstream msg;
        msg << "Value must be an integer between 0 and " << ULONG_MAX << ", with an optional 'K'"
               " suffix.";
        valueTokenPtr->ThrowException(msg.str());
    }
    if (*endPtr == 'K')
    {
        result *= 1024;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the signed integer value from a simple (name: value) section.
 *
 * @return the value.
 *
 * @throw mkToolsException if out of range.
 */
//--------------------------------------------------------------------------------------------------
ssize_t GetInt
(
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto valueTokenPtr = sectionPtr->Contents()[0];
    char* endPtr;
    errno = 0;
    size_t result = strtol(valueTokenPtr->text.c_str(), &endPtr, 0);
    if (errno != 0)
    {
        std::stringstream msg;
        msg << "Value must be an integer between " << LONG_MIN << " and " << LONG_MAX
            << ", with an optional 'K' suffix.";
        valueTokenPtr->ThrowException(msg.str());
    }
    if (*endPtr == 'K')
    {
        result *= 1024;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the integer value from a simple (name: value) section and verifies that it is
 * positive.
 *
 * @return the value.
 *
 * @throw mkToolsException if out of range.
 */
//--------------------------------------------------------------------------------------------------
size_t GetPositiveInt
(
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    size_t result = GetNonNegativeInt(sectionPtr);

    if (result == 0)
    {
        std::stringstream msg;
        msg << "Value must be an integer between 1 and " << ULONG_MAX << ", with an optional 'K'"
               " suffix.";
        sectionPtr->Contents()[0]->ThrowException(msg.str());
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print permissions to stdout.
 **/
//--------------------------------------------------------------------------------------------------
void PrintPermissions
(
    const model::Permissions_t& permissions
)
//--------------------------------------------------------------------------------------------------
{
    if (permissions.IsReadable())
    {
        std::cout << " read";
    }
    if (permissions.IsWriteable())
    {
        std::cout << " write";
    }
    if (permissions.IsExecutable())
    {
        std::cout << " execute";
    }
}



} // namespace modeller
