//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Component Instance class.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
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
    Component& component
)
//--------------------------------------------------------------------------------------------------
:   m_Component(component),
    m_RequiredApis(component.RequiredApis()),
    m_ProvidedApis(component.ProvidedApis())
//--------------------------------------------------------------------------------------------------
{
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
    m_ProvidedApis(source.m_ProvidedApis)
//--------------------------------------------------------------------------------------------------
{
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
    m_ProvidedApis(std::move(rvalue.m_ProvidedApis))
//--------------------------------------------------------------------------------------------------
{
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
        m_ProvidedApis = std::move(rvalue.m_ProvidedApis);
    }

    return *this;
}




//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the component instance.
 */
//--------------------------------------------------------------------------------------------------
const std::string& ComponentInstance::Name() const
{
    return m_Component.Name();
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

    throw legato::Exception("Component '" + Name() + "' does not have an interface named '"
                            + name + "'.");
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
ImportedInterface& ComponentInstance::FindClientInterface
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

    throw legato::Exception("Component '" + Name() + "' does not have a client-side interface"
                            " named '" + name + "'.");
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
ExportedInterface& ComponentInstance::FindServerInterface
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

    throw legato::Exception("Component '" + Name() + "' does not have a server-side interface"
                            " named '" + name + "'.");
}



}
