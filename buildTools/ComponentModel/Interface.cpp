//--------------------------------------------------------------------------------------------------
/**
 * Implementation of select methods of the Interface class.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"



namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
Interface::Interface
(
    const std::string& name,
    Api_t* apiPtr
)
//--------------------------------------------------------------------------------------------------
:   m_InternalName(name),
    m_ApiPtr(apiPtr),
    m_IsExternalToApp(false),
    m_ManualStart(false),
    m_ComponentInstancePtr(NULL)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
Interface::Interface
(
    std::string&& name,
    Api_t* apiPtr
)
//--------------------------------------------------------------------------------------------------
:   m_InternalName(std::move(name)),
    m_ApiPtr(apiPtr),
    m_IsExternalToApp(false),
    m_ManualStart(false),
    m_ComponentInstancePtr(NULL)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 */
//--------------------------------------------------------------------------------------------------
Interface::Interface
(
    const Interface& original
)
//--------------------------------------------------------------------------------------------------
:   m_InternalName(original.m_InternalName),
    m_ExternalName(original.m_ExternalName),
    m_ApiPtr(original.m_ApiPtr),
    m_Library(original.m_Library),
    m_IsExternalToApp(original.m_IsExternalToApp),
    m_ManualStart(original.m_ManualStart),
    m_ComponentInstancePtr(original.m_ComponentInstancePtr)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 */
//--------------------------------------------------------------------------------------------------
Interface::Interface
(
    Interface&& rvalue
)
//--------------------------------------------------------------------------------------------------
:   m_InternalName(std::move(rvalue.m_InternalName)),
    m_ExternalName(std::move(rvalue.m_ExternalName)),
    m_ApiPtr(std::move(rvalue.m_ApiPtr)),
    m_Library(std::move(rvalue.m_Library)),
    m_IsExternalToApp(std::move(rvalue.m_IsExternalToApp)),
    m_ManualStart(std::move(rvalue.m_ManualStart)),
    m_ComponentInstancePtr(std::move(rvalue.m_ComponentInstancePtr))
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Splits a given application-wide unique interface specifier into its three parts:
 *  - exe name,
 *  - component instance name,
 *  - interface instance name.
 *
 * @throw legato::Exception if something is wrong.
 **/
//--------------------------------------------------------------------------------------------------
void Interface::SplitAppUniqueName
(
    const std::string& interfaceSpec,   ///< [in] Specifier of the form "exe.component.interface"
    std::string& exeName,               ///< [out] Exe name.
    std::string& componentName,         ///< [out] Component instance name.
    std::string& interfaceName          ///< [out] Interface instance name.
)
//--------------------------------------------------------------------------------------------------
{
    size_t firstPeriodPos = interfaceSpec.find('.');
    if (firstPeriodPos == std::string::npos)
    {
        throw legato::Exception("Interface specifier '" + interfaceSpec + "'"
                                " is missing its '.' separators.");
    }
    if (firstPeriodPos == 0)
    {
        throw legato::Exception("Interface executable name missing before '.' separator in"
                                " interface specifier '" + interfaceSpec + "'.");
    }

    size_t secondPeriodPos = interfaceSpec.find('.', firstPeriodPos + 1);
    if (secondPeriodPos == std::string::npos)
    {
        throw legato::Exception("Interface specifier '" + interfaceSpec + "' is missing its"
                                " second '.' separator.");
    }
    if (secondPeriodPos == firstPeriodPos + 1)
    {
        throw legato::Exception("Interface component name missing between '.' separators in"
                                " interface specifier '" + interfaceSpec + "'.");
    }

    if (interfaceSpec.find('.', secondPeriodPos + 1) != std::string::npos)
    {
        throw legato::Exception("Interface specifier '" + interfaceSpec + "' contains too"
                                " many '.' separators.");
    }

    exeName = interfaceSpec.substr(0, firstPeriodPos);
    componentName = interfaceSpec.substr(firstPeriodPos + 1, secondPeriodPos - firstPeriodPos - 1);
    interfaceName = interfaceSpec.substr(secondPeriodPos + 1);

    if (interfaceName.empty())
    {
        throw legato::Exception("Interface instance name missing after second '.' separator in"
                                " interface specifier '" + interfaceSpec + "'.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the interface's external name (i.e., the name to use when talking to the Service Directory).
 **/
//--------------------------------------------------------------------------------------------------
std::string Interface::ExternalName
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    // If no external name has been set yet,
    if (m_ExternalName.empty())
    {
        // If the Component Instance pointer has been set, use the App-Wide Unique Name as
        // the external name.
        if (m_ComponentInstancePtr != NULL)
        {
            return AppUniqueName();
        }
        else
        {
            // Fall back to the Internal Name.
            return m_InternalName;
        }
    }
    else
    {
        return m_ExternalName;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the component instance that the interface is associated with.
 *
 * A side effect of this is that the interface library name will change.
 **/
//--------------------------------------------------------------------------------------------------
void Interface::ComponentInstancePtr
(
    const ComponentInstance* componentInstancePtr
)
//--------------------------------------------------------------------------------------------------
{
    m_ComponentInstancePtr = componentInstancePtr;

    m_Library.ShortName("IF_" + componentInstancePtr->AppUniqueName() + "." + m_InternalName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the application-wide unique name for this interface.
 *
 * @note    Won't work if the ExecutablePtr and ComponentInstancePtr have not been set.
 **/
//--------------------------------------------------------------------------------------------------
std::string Interface::AppUniqueName
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (m_ComponentInstancePtr == NULL)
    {
        throw legato::Exception("Component instance pointer not set on interface "
                                + m_InternalName);
    }

    return m_ComponentInstancePtr->AppUniqueName() + "." + m_InternalName;
}




//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
ClientInterface::ClientInterface
(
    const std::string& name,
    Api_t* apiPtr
)
//--------------------------------------------------------------------------------------------------
:   Interface(name, apiPtr),
    m_IsBound(false),
    m_TypesOnly(false)
//--------------------------------------------------------------------------------------------------
{
    m_Library.ShortName("IF_" + m_InternalName + "_client");
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 */
//--------------------------------------------------------------------------------------------------
ClientInterface::ClientInterface
(
    const ClientInterface& original
)
//--------------------------------------------------------------------------------------------------
:   Interface(original),
    m_IsBound(original.m_IsBound),
    m_TypesOnly(original.m_TypesOnly)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 */
//--------------------------------------------------------------------------------------------------
ClientInterface::ClientInterface
(
    ClientInterface&& rvalue
)
//--------------------------------------------------------------------------------------------------
:   Interface(rvalue),
    m_IsBound(std::move(rvalue.m_IsBound)),
    m_TypesOnly(std::move(rvalue.m_TypesOnly))
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 *
 * @return A reference to the object assigned to.
 **/
//--------------------------------------------------------------------------------------------------
ClientInterface& ClientInterface::operator =
(
    ClientInterface&& rvalue
)
//--------------------------------------------------------------------------------------------------
{
    if (&rvalue != this)
    {
        m_InternalName = std::move(rvalue.m_InternalName);
        m_ExternalName = std::move(rvalue.m_ExternalName);
        m_ApiPtr = std::move(rvalue.m_ApiPtr);
        m_Library = std::move(rvalue.m_Library);
        m_IsExternalToApp = std::move(rvalue.m_IsExternalToApp);
        m_ManualStart = std::move(rvalue.m_ManualStart);
        m_ComponentInstancePtr = std::move(rvalue.m_ComponentInstancePtr);
        m_IsBound = std::move(rvalue.m_IsBound);
        m_TypesOnly = std::move(rvalue.m_TypesOnly);
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator.
 *
 * @return A reference to the object assigned to.
 **/
//--------------------------------------------------------------------------------------------------
ClientInterface& ClientInterface::operator =
(
    const ClientInterface& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_InternalName = original.m_InternalName;
        m_ExternalName = original.m_ExternalName;
        m_ApiPtr = original.m_ApiPtr;
        m_Library = original.m_Library;
        m_IsExternalToApp = original.m_IsExternalToApp;
        m_ManualStart = original.m_ManualStart;
        m_ComponentInstancePtr = original.m_ComponentInstancePtr;
        m_IsBound = original.m_IsBound;
        m_TypesOnly = original.m_TypesOnly;
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * @return true if the interface has either been bound to something or declared an application
 *         external interface (i.e., binding has been deferred).
 **/
//--------------------------------------------------------------------------------------------------
bool ClientInterface::IsSatisfied
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return (IsBound() || IsExternalToApp());
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
ServerInterface::ServerInterface
(
    const std::string& name,
    Api_t* apiPtr
)
//--------------------------------------------------------------------------------------------------
:   Interface(name, apiPtr),
    m_IsAsync(false)
//--------------------------------------------------------------------------------------------------
{
    m_Library.ShortName("IF_" + m_InternalName + "_server");
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 */
//--------------------------------------------------------------------------------------------------
ServerInterface::ServerInterface
(
    const ServerInterface& original
)
//--------------------------------------------------------------------------------------------------
:   Interface(original),
    m_IsAsync(original.m_IsAsync)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 */
//--------------------------------------------------------------------------------------------------
ServerInterface::ServerInterface
(
    ServerInterface&& rvalue
)
//--------------------------------------------------------------------------------------------------
:   Interface(rvalue),
    m_IsAsync(std::move(rvalue.m_IsAsync))
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 *
 * @return A reference to the object assigned to.
 **/
//--------------------------------------------------------------------------------------------------
ServerInterface& ServerInterface::operator =
(
    ServerInterface&& rvalue
)
//--------------------------------------------------------------------------------------------------
{
    if (&rvalue != this)
    {
        m_InternalName = std::move(rvalue.m_InternalName);
        m_ExternalName = std::move(rvalue.m_ExternalName);
        m_ApiPtr = std::move(rvalue.m_ApiPtr);
        m_Library = std::move(rvalue.m_Library);
        m_IsExternalToApp = std::move(rvalue.m_IsExternalToApp);
        m_ManualStart = std::move(rvalue.m_ManualStart);
        m_ComponentInstancePtr = std::move(rvalue.m_ComponentInstancePtr);
        m_IsAsync = std::move(rvalue.m_IsAsync);
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator.
 *
 * @return A reference to the object assigned to.
 **/
//--------------------------------------------------------------------------------------------------
ServerInterface& ServerInterface::operator =
(
    const ServerInterface& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_InternalName = original.m_InternalName;
        m_ExternalName = original.m_ExternalName;
        m_ApiPtr = original.m_ApiPtr;
        m_Library = original.m_Library;
        m_IsExternalToApp = original.m_IsExternalToApp;
        m_ManualStart = original.m_ManualStart;
        m_ComponentInstancePtr = original.m_ComponentInstancePtr;
        m_IsAsync = original.m_IsAsync;
    }

    return *this;
}


}
