//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Process class methods.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include "limit.h"

namespace model
{

//--------------------------------------------------------------------------------------------------
/**
 * Check the validity of the process name.
 *
 * @throw mk::Exception_t if name is not valid.
 **/
//--------------------------------------------------------------------------------------------------
static void CheckName
(
    const std::string& name,
    const parseTree::RunProcess_t* parseTreePtr ///< Parse tree object to use to throw exceptions.
)
//--------------------------------------------------------------------------------------------------
{
    if (name.length() > LIMIT_MAX_PROCESS_NAME_LEN)
    {
        parseTreePtr->ThrowException(
            mk::format(LE_I18N("Process name '%s' is too long.  Must be a maximum of %d bytes."),
                       name, LIMIT_MAX_PROCESS_NAME_LEN)
        );
    }

    if (name.empty())
    {
        parseTreePtr->ThrowException(LE_I18N("Empty process name."));
    }

    if ((name == ".") || (name == ".."))
    {
        parseTreePtr->ThrowException(LE_I18N("Process name cannot be '.' or '..'."));
    }

    if (name.find(':') != std::string::npos)
    {
        parseTreePtr->ThrowException(LE_I18N("Process name cannot contain a colon (':')."));
    }

    if (name.find('/') != std::string::npos)
    {
        parseTreePtr->ThrowException(LE_I18N("Process name cannot contain a slash ('/')."));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the process name.
 **/
//--------------------------------------------------------------------------------------------------
void Process_t::SetName
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    // Note: The process name will become a config tree node name, so it can't contain
    // slashes or quotes or it will mess with the config tree.
    auto procName = path::GetLastNode(path::Unquote(name));

    CheckName(procName, parseTreePtr);

    this->name = procName;
}


}
