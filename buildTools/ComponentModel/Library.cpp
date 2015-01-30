//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Library class.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"


namespace legato
{

//--------------------------------------------------------------------------------------------------
/**
 * Copy Constructor
 **/
//--------------------------------------------------------------------------------------------------
Library::Library
(
    const Library& original
)
//--------------------------------------------------------------------------------------------------
:   m_ShortName(original.m_ShortName),
    m_BuildOutputDir(original.m_BuildOutputDir),
    m_BuildOutputPath(original.m_BuildOutputPath),
    m_IsUpToDate(original.m_IsUpToDate),
    m_IsStatic(original.m_IsStatic),
    m_Exists(original.m_Exists)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Move Constructor
 **/
//--------------------------------------------------------------------------------------------------
Library::Library
(
    Library&& rvalue
)
//--------------------------------------------------------------------------------------------------
:   m_ShortName(std::move(rvalue.m_ShortName)),
    m_BuildOutputDir(std::move(rvalue.m_BuildOutputDir)),
    m_BuildOutputPath(std::move(rvalue.m_BuildOutputPath)),
    m_IsUpToDate(std::move(rvalue.m_IsUpToDate)),
    m_IsStatic(std::move(rvalue.m_IsStatic)),
    m_Exists(std::move(rvalue.m_Exists))
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Asignment Operator
 **/
//--------------------------------------------------------------------------------------------------
Library& Library::operator =
(
    const Library& library
)
//--------------------------------------------------------------------------------------------------
{
    if (&library != this)
    {
        m_ShortName = library.m_ShortName;
        m_BuildOutputDir = library.m_BuildOutputDir;
        m_BuildOutputPath = library.m_BuildOutputPath;
        m_IsUpToDate = library.m_IsUpToDate;
        m_IsStatic = library.m_IsStatic;
        m_Exists = library.m_Exists;
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Asignment Operator
 **/
//--------------------------------------------------------------------------------------------------
Library& Library::operator =
(
    Library&& library
)
//--------------------------------------------------------------------------------------------------
{
    if (&library != this)
    {
        m_ShortName = std::move(library.m_ShortName);
        m_BuildOutputDir = std::move(library.m_BuildOutputDir);
        m_BuildOutputPath = std::move(library.m_BuildOutputPath);
        m_IsUpToDate = std::move(library.m_IsUpToDate);
        m_IsStatic = std::move(library.m_IsStatic);
        m_Exists = std::move(library.m_Exists);
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the file system path at which the library file will appear when it is built.
 *
 * @return The path.
 **/
//--------------------------------------------------------------------------------------------------
std::string Library::BuildOutputPath
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (m_BuildOutputPath.empty())
    {
        std::string path = m_BuildOutputDir + "/lib" + m_ShortName;

        if (m_IsStatic)
        {
            path += ".a";
        }
        else
        {
            path += ".so";
        }

        // TODO: Add a version number to the library.

        return path;
    }
    else
    {
        return m_BuildOutputPath;
    }
}


}   // namespace legato
