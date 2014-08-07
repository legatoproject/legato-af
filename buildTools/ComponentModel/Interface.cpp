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
    m_ServiceInstanceName(name),
    m_ApiPtr(apiPtr),
    m_IsInterApp(false),
    m_ManualStart(false)
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
    m_ServiceInstanceName(m_InternalName),
    m_ApiPtr(apiPtr),
    m_IsInterApp(false),
    m_ManualStart(false)
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
    m_ServiceInstanceName(original.m_ServiceInstanceName),
    m_ApiPtr(original.m_ApiPtr),
    m_AppUniqueName(original.m_AppUniqueName),
    m_Library(original.m_Library),
    m_IsInterApp(original.m_IsInterApp),
    m_ManualStart(original.m_ManualStart)
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
    m_ServiceInstanceName(std::move(rvalue.m_ServiceInstanceName)),
    m_ApiPtr(std::move(rvalue.m_ApiPtr)),
    m_AppUniqueName(std::move(rvalue.m_AppUniqueName)),
    m_Library(std::move(rvalue.m_Library)),
    m_IsInterApp(std::move(rvalue.m_IsInterApp)),
    m_ManualStart(std::move(rvalue.m_ManualStart))
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
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
ImportedInterface::ImportedInterface
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
    // NOTE: It's tempting to do framework API auto-binding here, but we don't have the
    //       isVerbose flag available here, so instead we do it in the Application Parser.

    m_Library.ShortName(m_InternalName + '_' + m_ApiPtr->Hash() + "_client");
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 */
//--------------------------------------------------------------------------------------------------
ImportedInterface::ImportedInterface
(
    const ImportedInterface& original
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
ImportedInterface::ImportedInterface
(
    ImportedInterface&& rvalue
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
ImportedInterface& ImportedInterface::operator =
(
    ImportedInterface&& rvalue
)
//--------------------------------------------------------------------------------------------------
{
    if (&rvalue != this)
    {
        m_InternalName = std::move(rvalue.m_InternalName);
        m_ServiceInstanceName = std::move(rvalue.m_ServiceInstanceName);
        m_ApiPtr = std::move(rvalue.m_ApiPtr);
        m_AppUniqueName = std::move(rvalue.m_AppUniqueName);
        m_Library = std::move(rvalue.m_Library);
        m_IsInterApp = std::move(rvalue.m_IsInterApp);
        m_ManualStart = std::move(rvalue.m_ManualStart);
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
ImportedInterface& ImportedInterface::operator =
(
    const ImportedInterface& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_InternalName = original.m_InternalName;
        m_ServiceInstanceName = original.m_ServiceInstanceName;
        m_ApiPtr = original.m_ApiPtr;
        m_AppUniqueName = original.m_AppUniqueName;
        m_Library = original.m_Library;
        m_IsInterApp = original.m_IsInterApp;
        m_ManualStart = original.m_ManualStart;
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
bool ImportedInterface::IsSatisfied
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return (IsBound() || IsExternal());
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
ExportedInterface::ExportedInterface
(
    const std::string& name,
    Api_t* apiPtr
)
//--------------------------------------------------------------------------------------------------
:   Interface(name, apiPtr),
    m_IsAsync(false)
//--------------------------------------------------------------------------------------------------
{
    m_Library.ShortName(m_InternalName + '_' + m_ApiPtr->Hash() + "_server");
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 */
//--------------------------------------------------------------------------------------------------
ExportedInterface::ExportedInterface
(
    const ExportedInterface& original
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
ExportedInterface::ExportedInterface
(
    ExportedInterface&& rvalue
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
ExportedInterface& ExportedInterface::operator =
(
    ExportedInterface&& rvalue
)
//--------------------------------------------------------------------------------------------------
{
    if (&rvalue != this)
    {
        m_InternalName = std::move(rvalue.m_InternalName);
        m_ServiceInstanceName = std::move(rvalue.m_ServiceInstanceName);
        m_ApiPtr = std::move(rvalue.m_ApiPtr);
        m_AppUniqueName = std::move(rvalue.m_AppUniqueName);
        m_Library = std::move(rvalue.m_Library);
        m_IsInterApp = std::move(rvalue.m_IsInterApp);
        m_ManualStart = std::move(rvalue.m_ManualStart);
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
ExportedInterface& ExportedInterface::operator =
(
    const ExportedInterface& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_InternalName = original.m_InternalName;
        m_ServiceInstanceName = original.m_ServiceInstanceName;
        m_ApiPtr = original.m_ApiPtr;
        m_AppUniqueName = original.m_AppUniqueName;
        m_Library = original.m_Library;
        m_IsInterApp = original.m_IsInterApp;
        m_ManualStart = original.m_ManualStart;
        m_IsAsync = original.m_IsAsync;
    }

    return *this;
}


}
