//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Library class.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
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
    m_IsUpToDate(original.m_IsUpToDate)
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
    m_IsUpToDate(std::move(rvalue.m_IsUpToDate))
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
        m_IsUpToDate = library.m_IsUpToDate;
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
        m_IsUpToDate = std::move(library.m_IsUpToDate);
    }

    return *this;
}


}
