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
    dir(path::MakeAbsolute(path::GetContainingDir(filePtr->path))),
    name(path::GetIdentifierSafeName(path::RemoveSuffix(path::GetLastNode(filePtr->path), ".adef"))),
    workingDir("app/" + name),
    isSandboxed(true),
    startTrigger(AUTO),
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
 * Find the client interface instance object associated with a given internal interface
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
    for (auto exePtr : executables)
    {
        if (exePtr->name == exeTokenPtr->text)
        {
            for (auto componentInstancePtr : exePtr->componentInstances)
            {
                if (componentInstancePtr->componentPtr->name == componentTokenPtr->text)
                {
                    for (auto ifInstancePtr : componentInstancePtr->serverApis)
                    {
                        if (ifInstancePtr->ifPtr->internalName == interfaceTokenPtr->text)
                        {
                            return ifInstancePtr;
                        }
                    }
                    interfaceTokenPtr->ThrowException("Server interface '" + interfaceTokenPtr->text
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
    for (auto exePtr : executables)
    {
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
                    interfaceTokenPtr->ThrowException("Client interface '" + interfaceTokenPtr->text
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
    for (auto exePtr : executables)
    {
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


} // namespace modeller
