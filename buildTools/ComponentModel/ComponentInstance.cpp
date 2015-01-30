//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Component Instance class.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
ComponentInstance::ComponentInstance
(
    Component& component,
    Executable* exePtr
)
//--------------------------------------------------------------------------------------------------
:   m_Component(component),
    m_RequiredApis(component.RequiredApis()),
    m_ProvidedApis(component.ProvidedApis()),
    m_ExePtr(exePtr)
//--------------------------------------------------------------------------------------------------
{
    for (auto& mapEntry : m_RequiredApis)
    {
        mapEntry.second.ComponentInstancePtr(this);
    }

    for (auto& mapEntry : m_ProvidedApis)
    {
        mapEntry.second.ComponentInstancePtr(this);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 **/
//--------------------------------------------------------------------------------------------------
ComponentInstance::ComponentInstance
(
    const ComponentInstance& source
)
//--------------------------------------------------------------------------------------------------
:   m_Component(source.m_Component),
    m_RequiredApis(source.m_RequiredApis),
    m_ProvidedApis(source.m_ProvidedApis),
    m_SubInstances(source.m_SubInstances),
    m_ExePtr(source.m_ExePtr)
//--------------------------------------------------------------------------------------------------
{
    for (auto& mapEntry : m_RequiredApis)
    {
        mapEntry.second.ComponentInstancePtr(this);
    }

    for (auto& mapEntry : m_ProvidedApis)
    {
        mapEntry.second.ComponentInstancePtr(this);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 **/
//--------------------------------------------------------------------------------------------------
ComponentInstance::ComponentInstance
(
    ComponentInstance&& rvalue
)
//--------------------------------------------------------------------------------------------------
:   m_Component(rvalue.m_Component),
    m_RequiredApis(std::move(rvalue.m_RequiredApis)),
    m_ProvidedApis(std::move(rvalue.m_ProvidedApis)),
    m_SubInstances(std::move(rvalue.m_SubInstances)),
    m_ExePtr(std::move(rvalue.m_ExePtr))
//--------------------------------------------------------------------------------------------------
{
    for (auto& mapEntry : m_RequiredApis)
    {
        mapEntry.second.ComponentInstancePtr(this);
    }

    for (auto& mapEntry : m_ProvidedApis)
    {
        mapEntry.second.ComponentInstancePtr(this);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Destructor
 **/
//--------------------------------------------------------------------------------------------------
ComponentInstance::~ComponentInstance()
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator (=)
 **/
//--------------------------------------------------------------------------------------------------
ComponentInstance& ComponentInstance::operator =
(
    ComponentInstance&& rvalue
)
//--------------------------------------------------------------------------------------------------
{
    if (&rvalue != this)
    {
        m_Component = std::move(rvalue.m_Component);

        m_RequiredApis = std::move(rvalue.m_RequiredApis);
        for (auto& mapEntry : m_RequiredApis)
        {
            mapEntry.second.ComponentInstancePtr(this);
        }

        m_ProvidedApis = std::move(rvalue.m_ProvidedApis);
        for (auto& mapEntry : m_ProvidedApis)
        {
            mapEntry.second.ComponentInstancePtr(this);
        }

        m_SubInstances = std::move(rvalue.m_SubInstances);
        m_ExePtr = rvalue.m_ExePtr;
    }

    return *this;
}




//--------------------------------------------------------------------------------------------------
/**
 * Get the application-wide unique name of the component instance ("exe.component").
 */
//--------------------------------------------------------------------------------------------------
std::string ComponentInstance::AppUniqueName
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (m_ExePtr == NULL)
    {
        throw Exception("Executable not set for instance of component '" + m_Component.Name()
                        + "'");
    }

    return m_ExePtr->CName() + "." + m_Component.CName();
}



//--------------------------------------------------------------------------------------------------
/**
 * Find an IPC interface either provided-by or required-by the component instance.
 *
 * @return A reference to the Interface object.
 *
 * @throw legato::Exception if not found.
 **/
//--------------------------------------------------------------------------------------------------
Interface& ComponentInstance::FindInterface
(
    const std::string& name ///< [in] Name of the interface.
)
//--------------------------------------------------------------------------------------------------
{
    auto i = m_RequiredApis.find(name);
    if (i != m_RequiredApis.end())
    {
        return i->second;
    }

    auto e = m_ProvidedApis.find(name);
    if (e != m_ProvidedApis.end())
    {
        return e->second;
    }

    throw legato::Exception("Component instance'" + AppUniqueName() + "' does not have an"
                            " interface named '" + name + "'.");
}



//--------------------------------------------------------------------------------------------------
/**
 * Find a client-side IPC API interface required by the component instance.
 *
 * @return A reference to the Interface object.
 *
 * @throw legato::Exception if not found.
 **/
//--------------------------------------------------------------------------------------------------
ClientInterface& ComponentInstance::FindClientInterface
(
    const std::string& name ///< [in] Name of the interface.
)
//--------------------------------------------------------------------------------------------------
{
    auto i = m_RequiredApis.find(name);
    if (i != m_RequiredApis.end())
    {
        return i->second;
    }

    throw legato::Exception("Component instance'" + AppUniqueName() + "' does not have a "
                            "client-side interface named '" + name + "'.");
}



//--------------------------------------------------------------------------------------------------
/**
 * Find a server-side IPC API interface provided by the component instance.
 *
 * @return A reference to the Interface object.
 *
 * @throw legato::Exception if not found.
 **/
//--------------------------------------------------------------------------------------------------
ServerInterface& ComponentInstance::FindServerInterface
(
    const std::string& name ///< [in] Name of the interface.
)
//--------------------------------------------------------------------------------------------------
{
    auto e = m_ProvidedApis.find(name);
    if (e != m_ProvidedApis.end())
    {
        return e->second;
    }

    throw legato::Exception("Component '" + AppUniqueName() + "' does not have a server-side"
                            " interface named '" + name + "'.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the executable that this component instance belongs to.
 *
 * @note Some things won't work right until the executable is set.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentInstance::SetExe
(
    Executable* exePtr  ///< pointer to the executable, or NULL if component has no executable.
)
//--------------------------------------------------------------------------------------------------
{
    m_ExePtr = exePtr;
}



} // namespace legato
