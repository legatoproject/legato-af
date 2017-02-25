//--------------------------------------------------------------------------------------------------
/**
 * @file moduleModeller.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "modellerCommon.h"


namespace modeller
{

//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a module whose .mdef file can be found at a given path.
 *
 * @return Pointer to the module object.
 */
//--------------------------------------------------------------------------------------------------
model::Module_t* GetModule
(
    const std::string& mdefPath,    ///< Path to the module's .mdef file.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Save the old CURDIR environment variable value and set it to the dir containing this file.
    auto oldDir = envVars::Get("CURDIR");
    envVars::Set("CURDIR", path::GetContainingDir(mdefPath));

    auto mdefFilePtr = parser::mdef::Parse(mdefPath, buildParams.beVerbose);
    auto modulePtr = new model::Module_t(mdefFilePtr);

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Modelling module defined in: '%s'"), mdefPath)
                  << std::endl;
    }

    for (auto sectionPtr : mdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;
        if ("params" == sectionName)
        {
            for (auto& paramsPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
            {
                auto paramName = paramsPtr->firstTokenPtr->text;
                // We should call this RemoveQuotes(), but it still works.
                auto paramVal = RemoveAngleBrackets(paramsPtr->lastTokenPtr->text);
                modulePtr->AddParam(paramName, paramVal);

            }
        }
        else if ("preBuilt" == sectionName)
        {
            auto modulePath =
                path::Unquote(envVars::DoSubstitution(ToSimpleSectionPtr(sectionPtr)->Text()));
            if (!path::HasSuffix(modulePath, ".ko"))
            {
                // Throw exception: not a kernel module
                sectionPtr->ThrowException(
                    mk::format(LE_I18N("File '%s' is not a kernel module (*.ko)."), modulePath)
                );
            }
            if (!file::FileExists(modulePath))
            {
                // Throw exception: file doesn't exist
                sectionPtr->ThrowException(
                    mk::format(LE_I18N("Module file '%s' does not exist."),  modulePath)
                );
            }
            modulePtr->SetPath(modulePath);
        }
    }

    // preBuilt: section is mandatory so check if we parsed it
    if (modulePtr->path.empty())
    {
        // Throw generic exception at file level
        throw mk::Exception_t(
            mk::format(LE_I18N("%s: error: Missing section 'preBuilt'."), mdefPath)
        );
    }

    // Restore the previous contents of the CURDIR environment variable.
    envVars::Set("CURDIR", oldDir);

    return modulePtr;
}

} // namespace modeller
