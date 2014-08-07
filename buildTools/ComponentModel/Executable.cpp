//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Executable object.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"



//--------------------------------------------------------------------------------------------------
/**
 * Default constructor
 */
//--------------------------------------------------------------------------------------------------
legato::Executable::Executable
(
)
//--------------------------------------------------------------------------------------------------
:   m_IsDebuggingEnabled(false)
//--------------------------------------------------------------------------------------------------
{
    m_DefaultComponent.Name("default");
}



//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 */
//--------------------------------------------------------------------------------------------------
legato::Executable::Executable
(
    legato::Executable&& original
)
//--------------------------------------------------------------------------------------------------
:   m_OutputPath(std::move(original.m_OutputPath)),
    m_CName(std::move(original.m_CName)),
    m_IsDebuggingEnabled(std::move(original.m_IsDebuggingEnabled)),
    m_ComponentInstances(std::move(original.m_ComponentInstances)),
    m_DefaultComponent(std::move(original.m_DefaultComponent))
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator (=)
 **/
//--------------------------------------------------------------------------------------------------
legato::Executable& legato::Executable::operator =
(
    Executable&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_OutputPath = std::move(original.m_OutputPath);
        m_CName = std::move(original.m_CName);
        m_IsDebuggingEnabled = std::move(original.m_IsDebuggingEnabled);
        m_ComponentInstances = std::move(original.m_ComponentInstances);
        m_DefaultComponent = std::move(original.m_DefaultComponent);
    }

    return *this;
}




//--------------------------------------------------------------------------------------------------
/**
 * Set the path to which the built executable will be output.
 **/
//--------------------------------------------------------------------------------------------------
void legato::Executable::OutputPath
(
    const std::string& path ///< [IN] The file system path of the executable to be generated.
)
//--------------------------------------------------------------------------------------------------
{
    m_OutputPath = path;

    if (m_CName == "")
    {
        CName(GetCSafeName(GetLastPathNode(m_OutputPath)));
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the file path to which the executable file will be output.
 *
 * @return the path.
 */
//--------------------------------------------------------------------------------------------------
const std::string& legato::Executable::OutputPath() const
{
    return m_OutputPath;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the name that will be used inside identifiers in generated C code.
 */
//--------------------------------------------------------------------------------------------------
void legato::Executable::CName(const std::string& name)
{
    m_CName = name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name that will be used inside identifiers in generated C code.
 *
 * @return the C-friendly name.
 */
//--------------------------------------------------------------------------------------------------
const std::string& legato::Executable::CName() const
{
    return m_CName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a Component Instance to the list of Component Instances to be included in this executable.
 */
//--------------------------------------------------------------------------------------------------
legato::ComponentInstance& legato::Executable::AddComponentInstance
(
    ComponentInstance&& instance
)
//--------------------------------------------------------------------------------------------------
{
    m_ComponentInstances.push_back(instance);

    return m_ComponentInstances.back();
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a list of all the Component Instances that are to be included in this executable.
 *
 * @return A reference to the list.
 */
//--------------------------------------------------------------------------------------------------
const std::list<legato::ComponentInstance>& legato::Executable::ComponentInstances
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return m_ComponentInstances;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for an instance of a component in the executable's list of component instances.
 *
 * @return A reference to the component instance.
 *
 * @throw legato::Exception if not found.
 **/
//--------------------------------------------------------------------------------------------------
legato::ComponentInstance& legato::Executable::FindComponentInstance
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    for (auto& c : m_ComponentInstances)
    {
        if (c.Name() == name)
        {
            return c;
        }
    }

    throw legato::Exception("Executable '" + m_OutputPath
                            + "' doesn't contain component '" + name + "'.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a library directly to the executable's "default" component instance.
 */
//--------------------------------------------------------------------------------------------------
void legato::Executable::AddLibrary
(
    std::string path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefaultComponent.AddLibrary(path);
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a C/C++ source code file directly to the executable's "default" component instance.
 */
//--------------------------------------------------------------------------------------------------
void legato::Executable::AddCSourceFile
(
    std::string path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefaultComponent.AddSourceFile(path);
}



//--------------------------------------------------------------------------------------------------
/**
 * Does the current executable have C language sources?
 *
 * @return True if the executable or any of it's sub-components have C code in them.
 */
//--------------------------------------------------------------------------------------------------
bool legato::Executable::HasCSources
(
)
const
//--------------------------------------------------------------------------------------------------
{
    if (m_DefaultComponent.HasCSources())
    {
        return true;
    }

    for (const auto& componentInstance : m_ComponentInstances)
    {
        if (componentInstance.GetComponent().HasCSources())
        {
            return true;
        }
    }

    return false;
}




//--------------------------------------------------------------------------------------------------
/**
 * Does the current executable have C++ language sources?
 *
 * @return True if the executable or any of it's sub-components have C++ code in them.
 */
//--------------------------------------------------------------------------------------------------
bool legato::Executable::HasCppSources
(
)
const
//--------------------------------------------------------------------------------------------------
{
    if (m_DefaultComponent.HasCppSources())
    {
        return true;
    }

    for (const auto& componentInstance : m_ComponentInstances)
    {
        if (componentInstance.GetComponent().HasCppSources())
        {
            return true;
        }
    }

    return false;
}
