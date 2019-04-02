//--------------------------------------------------------------------------------------------------
/**
 * @file app.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 **/
//--------------------------------------------------------------------------------------------------
App_t::App_t
(
    parseTree::AdefFile_t* filePtr  ///< Pointer to the root of the parse tree for the .adef file.
)
//--------------------------------------------------------------------------------------------------
:   defFilePtr(filePtr),
    parseTreePtr(NULL),
    dir(path::MakeAbsolute(path::GetContainingDir(filePtr->path))),
    name(path::GetIdentifierSafeName(path::RemoveSuffix(path::GetLastNode(filePtr->path), ".adef"))),
    workingDir("app/" + name),
    isSandboxed(true),
    startTrigger(AUTO),
    preloadedMode(NONE),
    isPreBuilt(false),
    cpuShare(1024),
    maxFileSystemBytes(128 * 1024),   // 128 KB
    maxMemoryBytes(40000 * 1024), // 40 MB
    maxMQueueBytes(512),
    maxQueuedSignals(100),
    maxThreads(20),
    maxSecureStorageBytes(8192)
//--------------------------------------------------------------------------------------------------
{

}


//--------------------------------------------------------------------------------------------------
/**
 * Find the component instance object associated with a given exe name and component name.
 *
 * @return Pointer to the object.
 *
 * @throw mk::Exception_t if not found.
 */
//--------------------------------------------------------------------------------------------------
ComponentInstance_t* App_t::FindComponentInstance
(
    const parseTree::Token_t* exeTokenPtr,
    const parseTree::Token_t* componentTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& exeName = exeTokenPtr->text;
    const std::string& componentName = componentTokenPtr->text;

    // Find the executable in the app,
    for (auto mapItem : executables)
    {
        auto exePtr = mapItem.second;

        if (exePtr->name == exeName)
        {
            // Find the component instance in the executable,
            for (auto componentInstancePtr : exePtr->componentInstances)
            {
                if (componentInstancePtr->componentPtr->name == componentName)
                {
                    return componentInstancePtr;
                }
            }

            componentTokenPtr->ThrowException(
                mk::format(LE_I18N("Component '%s' not found in executable '%s'."),
                           componentName, exeName)
            );
        }
    }

    exeTokenPtr->ThrowException(
        mk::format(LE_I18N("Executable '%s' not defined in application."), exeName)
    );

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the server interface instance object associated with a given internal interface
 * specification.
 *
 * @return Pointer to the object.
 *
 * @throw mk::Exception_t if not found.
 */
//--------------------------------------------------------------------------------------------------
ApiServerInterfaceInstance_t* App_t::FindServerInterface
(
    const parseTree::Token_t* exeTokenPtr,
    const parseTree::Token_t* componentTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& exeName = exeTokenPtr->text;
    const std::string& componentName = componentTokenPtr->text;
    const std::string& interfaceName = interfaceTokenPtr->text;

    const std::string fullName = exeName + "." + componentName + "." + interfaceName;
    auto iter = externServerInterfaces.find(fullName);

    ApiServerInterfaceInstance_t* ifInstancePtr;

    if (iter != externServerInterfaces.end())
    {
        ifInstancePtr = iter->second;
    }
    else
    {
        iter = preBuiltServerInterfaces.find(interfaceName);

        if (iter != preBuiltServerInterfaces.end())
        {
            ifInstancePtr = iter->second;
        }
        else
        {
            // Find the component instance specified.
            auto componentInstancePtr = FindComponentInstance(exeTokenPtr, componentTokenPtr);

            // Find the interface in the component instance's list of server interfaces,
            ifInstancePtr = componentInstancePtr->FindServerInterface(interfaceName);

            if (ifInstancePtr == NULL)
            {
                interfaceTokenPtr->ThrowException(
                                  mk::format(LE_I18N("Server interface '%s' not found in component "
                                                     "'%s' in executable '%s'."),
                                             interfaceName,
                                             componentName,
                                             exeName));
            }
        }
    }

    return ifInstancePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the client interface instance object associated with a given internal interface
 * specification.
 *
 * @return Pointer to the object.
 *
 * @throw mk::Exception_t if not found.
 */
//--------------------------------------------------------------------------------------------------
ApiClientInterfaceInstance_t* App_t::FindClientInterface
(
    const parseTree::Token_t* exeTokenPtr,
    const parseTree::Token_t* componentTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& exeName = exeTokenPtr->text;
    const std::string& componentName = componentTokenPtr->text;
    const std::string& interfaceName = interfaceTokenPtr->text;

    const std::string fullName = exeName + "." + componentName + "." + interfaceName;
    auto iter = preBuiltClientInterfaces.find(fullName);

    ApiClientInterfaceInstance_t* ifInstancePtr;

    if (iter != preBuiltClientInterfaces.end())
    {
        ifInstancePtr = iter->second;
    }
    else
    {
        // Find the component instance specified.
        auto componentInstancePtr = FindComponentInstance(exeTokenPtr, componentTokenPtr);

        // Find the interface in the component instance's list of client interfaces,
        ifInstancePtr = componentInstancePtr->FindClientInterface(interfaceName);

        if (ifInstancePtr == NULL)
        {
            interfaceTokenPtr->ThrowException(
                        mk::format(LE_I18N("Client interface '%s' not found in component '%s'"
                                            " in executable '%s'."),
                                    interfaceName,
                                    componentName,
                                    exeName));
        }
    }

    return ifInstancePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the client interface instance object associated with a given external interface name.
 *
 * @return Pointer to the object, or NULL if no matching interface is found.
 */
//--------------------------------------------------------------------------------------------------
ApiClientInterfaceInstance_t* App_t::FindClientInterface
(
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& interfaceName = interfaceTokenPtr->text;

    auto i = externClientInterfaces.find(interfaceName);

    if (i == externClientInterfaces.end())
    {
        i = preBuiltClientInterfaces.find(interfaceName);

        if (i == preBuiltClientInterfaces.end())
        {
            return NULL;
        }
    }

    return i->second;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the server interface instance object associated with a given external interface name.
 *
 * @return Pointer to the object, or NULL if no matching interface is found.
 */
//--------------------------------------------------------------------------------------------------
ApiServerInterfaceInstance_t* App_t::FindServerInterface
(
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& interfaceName = interfaceTokenPtr->text;

    auto i = externServerInterfaces.find(interfaceName);

    if (i == externServerInterfaces.end())
    {
        i = preBuiltServerInterfaces.find(interfaceName);

        if (i == preBuiltServerInterfaces.end())
        {
            return NULL;
        }
    }

    return i->second;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the interface instance object associated with a given internal interface specification.
 *
 * @return Pointer to the object.
 *
 * @throw mk::Exception_t if not found.
 */
//--------------------------------------------------------------------------------------------------
ApiInterfaceInstance_t* App_t::FindInterface
(
    const parseTree::Token_t* exeTokenPtr,
    const parseTree::Token_t* componentTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto mapItem : executables)
    {
        auto exePtr = mapItem.second;

        if (exePtr->name == exeTokenPtr->text)
        {
            for (auto componentInstancePtr : exePtr->componentInstances)
            {
                if (componentInstancePtr->componentPtr->name == componentTokenPtr->text)
                {
                    for (auto ifInstancePtr : componentInstancePtr->clientApis)
                    {
                        if (ifInstancePtr->ifPtr->internalName == interfaceTokenPtr->text)
                        {
                            return ifInstancePtr;
                        }
                    }

                    for (auto ifInstancePtr : componentInstancePtr->serverApis)
                    {
                        if (ifInstancePtr->ifPtr->internalName == interfaceTokenPtr->text)
                        {
                            return ifInstancePtr;
                        }
                    }

                    interfaceTokenPtr->ThrowException(
                        mk::format(LE_I18N("Interface '%s' not found in component '%s'"
                                           " in executable '%s'."),
                                   interfaceTokenPtr->text,
                                   componentInstancePtr->componentPtr->name,
                                   exePtr->name)
                    );
                }
            }
            componentTokenPtr->ThrowException(
                mk::format(LE_I18N("Component '%s' not found in executable '%s'."),
                           componentTokenPtr->text, exePtr->name)
            );
        }
    }
    exeTokenPtr->ThrowException(
        mk::format(LE_I18N("Executable '%s' not defined in application."), exeTokenPtr->text)
    );
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the app's root.cfg file relative to the build's working directory.
 *
 * @return the file path.
 */
//--------------------------------------------------------------------------------------------------
std::string App_t::ConfigFilePath
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return workingDir + "/staging/root.cfg";
}


} // namespace modeller
