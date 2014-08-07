//--------------------------------------------------------------------------------------------------
/**
 * Implementation of select methods of the Api class.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include <string.h>


#include <string.h>


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Map of API file absolute paths to Api objects.
 **/
//--------------------------------------------------------------------------------------------------
static std::map<std::string, Api_t*> Apis;


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the API object for a given API file path, or NULL if it doesn't exist yet.
 **/
//--------------------------------------------------------------------------------------------------
Api_t* Api_t::GetApiPtr
(
    const std::string& filePath
)
//--------------------------------------------------------------------------------------------------
{
    if (!IsAbsolutePath(filePath))
    {
        throw Exception("API file path '" + filePath + "' is not an absolute path.");
    }

    auto iter = Apis.find(filePath);
    if (iter != Apis.end())
    {
        return iter->second;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
Api_t::Api_t
(
    const std::string& filePath
)
//--------------------------------------------------------------------------------------------------
:   m_FilePath(filePath)
//--------------------------------------------------------------------------------------------------
{
    std::string fileName = GetLastPathNode(filePath);
    size_t pos = fileName.find_last_of('.');
    if ((pos == std::string::npos) || (fileName.substr(pos) != ".api"))
    {
        throw Exception("Interface file '" + filePath + "' missing .api file name extension.");
    }

    m_Name = fileName.substr(0, pos);

    if (Apis.find(m_FilePath) != Apis.end())
    {
        throw Exception("Parser internal error: Duplicate .api file '" + m_FilePath + "'.");
    }

    Apis[m_FilePath] = this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move constructor.
 **/
//--------------------------------------------------------------------------------------------------
Api_t::Api_t
(
    Api_t&& original
)
//--------------------------------------------------------------------------------------------------
:   m_Name(std::move(m_Name)),
    m_FilePath(std::move(m_FilePath)),
    m_Hash(std::move(m_Hash)),
    m_Dependencies(std::move(m_Dependencies))

//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 **/
//--------------------------------------------------------------------------------------------------
Api_t& Api_t::operator=
(
    Api_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        m_Name = std::move(m_Name);
        m_FilePath = std::move(m_FilePath);
        m_Hash = std::move(m_Hash);
        m_Dependencies = std::move(m_Dependencies);
    }

    return *this;
}


}
