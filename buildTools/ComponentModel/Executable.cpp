//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Executable object.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
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
    m_ComponentInstanceList(std::move(original.m_ComponentInstanceList)),
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
        m_ComponentInstanceList = std::move(original.m_ComponentInstanceList);
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
void legato::Executable::AddComponentInstance
(
    ComponentInstance&& instance
)
//--------------------------------------------------------------------------------------------------
{
    m_ComponentInstanceList.push_back(instance);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a list of all the Component Instances that are to be included in this executable.
 *
 * @return A reference to the list.
 */
//--------------------------------------------------------------------------------------------------
const std::list<legato::ComponentInstance>& legato::Executable::ComponentInstanceList
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return m_ComponentInstanceList;
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
 * Add a C source code file directly to the executable's "default" component instance.
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
