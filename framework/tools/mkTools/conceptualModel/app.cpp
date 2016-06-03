//--------------------------------------------------------------------------------------------------
/**
 * @file app.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

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
    isPreloaded(false),
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

            componentTokenPtr->ThrowException("Component '" + componentName + "'"
                                              " not found in executable "
                                              "'" + exeName + "'.");
        }
    }

    exeTokenPtr->ThrowException("Executable '" + exeName + "' not defined in application.");

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

    // Find the component instance specified.
    auto componentInstancePtr = FindComponentInstance(exeTokenPtr, componentTokenPtr);

    // Find the interface in the component instance's list of server interfaces,
    auto ifInstancePtr = componentInstancePtr->FindServerInterface(interfaceName);

    if (ifInstancePtr == NULL)
    {
        interfaceTokenPtr->ThrowException("Server interface '" + interfaceName + "'"
                                          " not found in component '" + componentName + "'"
                                          " in executable '" + exeName + "'.");
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

    // Find the component instance specified.
    auto componentInstancePtr = FindComponentInstance(exeTokenPtr, componentTokenPtr);

    // Find the interface in the component instance's list of client interfaces,
    auto ifInstancePtr = componentInstancePtr->FindClientInterface(interfaceName);

    if (ifInstancePtr == NULL)
    {
        interfaceTokenPtr->ThrowException("Client interface '" + interfaceName + "'"
                                          " not found in component '" + componentName + "'"
                                          " in executable '" + exeName + "'.");
    }

    return ifInstancePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the client interface instance object associated with a given external interface name.
 *
 * @return Pointer to the object.
 *
 * @throw mk::Exception_t if not found.
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
        interfaceTokenPtr->ThrowException("App '" + name + "' has no external client-side interface"
                                          " named '" + interfaceName + "'");
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

                    interfaceTokenPtr->ThrowException("Interface '" + interfaceTokenPtr->text
                                                      + "' not found in component '"
                                                      + componentInstancePtr->componentPtr->name
                                                      + "' in executable '" + exePtr->name + "'.");
                }
            }
            componentTokenPtr->ThrowException("Component '" + componentTokenPtr->text + "'"
                                              " not found in executable '" + exePtr->name + "'.");
        }
    }
    exeTokenPtr->ThrowException("Executable '" + exeTokenPtr->text
                                + "' not defined in application.");
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
