//--------------------------------------------------------------------------------------------------
/**
 * @file updateDefinitionFile.cpp
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <stdexcept>
#include <iostream>
#include <set>
#include <list>
#include <string>
#include <cstring>

#include "mkTools.h"
#include "commandLineInterpreter.h"

namespace cli
{

namespace updateDefs
{

//-------------------------------------------------------------------------------------------------
/**
 * Compare the position of two vectors
 */
//-------------------------------------------------------------------------------------------------
bool ComparePosition
(
    const ArgHandler_t::LinePosition_t& p1,
    const ArgHandler_t::LinePosition_t& p2
)
{
    return (p1.beforePos < p2.beforePos);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update a given definition file with a string of line to write to the definition file at a
 * given position.
 * 1. Create a temporary backup file where the appropriate section is updated.
 * 2. Copy the contents of the original definition source file to the temporary file until first
 *    position of line to write.
 * 3. Update the temporary file with the line that need to be written.
 * 4. Copy the remaining contents of the original definition file from second position till the end
 *    of the file.
 * 5. After the temporary file is successfully updated, rename the temporary file to the original
 *    file name such that the orignal file is updated.
 */
//--------------------------------------------------------------------------------------------------
void UpdateDefinitionFile
(
    ArgHandler_t& handler,      ///< ArgHandler_t object
    std::string sourceFile      ///< Original definition file is the source file
)
{
    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N(
                                  "\nCreating temporary working file '%s' from original file '%s'."
                                  "\nEditing the specified section(s) in the temporary file."),
                                handler.tempWorkDefFilePath, sourceFile);
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    std::ifstream source(sourceFile);
    std::ofstream dest(handler.tempWorkDefFilePath);

    if (!source)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for input."), sourceFile)
        );
    }

    if (!dest)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for output."), handler.tempWorkDefFilePath)
        );
    }

    source.seekg(0, source.end);
    int totalLength = source.tellg();

    // Copy first part of sdef
    source.seekg(0, source.beg);

    // Sort vector into ascending order
    std::sort(handler.linePositionToWrite.begin(), handler.linePositionToWrite.end(),
              ComparePosition);

    int startPos = 0;
    for (auto& it : handler.linePositionToWrite)
    {
        int length = it.beforePos - startPos;
        std::unique_ptr<char> firstBuffer(new char[length]);

        source.read(firstBuffer.get(), length-1);

        if (!dest.is_open())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to open file '%s' for writing."),
                           handler.tempWorkDefFilePath)
            );
        }

        dest.write(firstBuffer.get(), length-1);

        if (!it.lineToWrite.empty())
        {
            // Strip off '.adef' or '.mdef' optional suffix before writing to the definition file.
            if (path::HasSuffix(it.lineToWrite, ADEF_EXT))
            {
                it.lineToWrite = path::RemoveSuffix(it.lineToWrite, ADEF_EXT);
            }
            else if (path::HasSuffix(it.lineToWrite, MDEF_EXT))
            {
                it.lineToWrite = path::RemoveSuffix(it.lineToWrite, MDEF_EXT);
            }

            if (handler.buildParams.beVerbose)
            {
                std::cout << mk::format(LE_I18N("\nWriting '%s'."), it.lineToWrite);
            }

            // Write the line to the destination file.
            dest << it.lineToWrite;
        }
        else
        {
           dest << "" ;
        }
        startPos = it.afterPos-1;

        source.seekg(startPos);
    }

    // Copy the remaining part of sdef
    int remainLength = totalLength-startPos;
    std::unique_ptr<char> secondBuffer(new char[remainLength]);

    source.read(secondBuffer.get(), remainLength);
    dest.write(secondBuffer.get(), remainLength);

    source.close();
    dest.close();
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the search path directory list pointed to by sectionPtr and add to the searchPathList
 */
//--------------------------------------------------------------------------------------------------
static void ReadSearchDirs
(
    std::list<std::string>& searchPathList,   ///< And add the new search paths to this list.
    const parseTree::TokenList_t* sectionPtr    ///< From the search paths from this section.
)
//--------------------------------------------------------------------------------------------------
{
    // An interfaceSearch section is a list of FILE_PATH tokens.
    for (const auto contentItemPtr : sectionPtr->Contents())
    {
        auto tokenPtr = dynamic_cast<const parseTree::Token_t*>(contentItemPtr);

        auto dirPath = path::Unquote(DoSubstitution(tokenPtr));

        // If the environment variable substitution resulted in an empty string, just ignore this.
        if (!dirPath.empty())
        {
            // Files in .leaf must not be modified
            if (dirPath.find(".leaf") != std::string::npos)
            {
                continue;
            }
            searchPathList.push_back(dirPath);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the sdef to read appSearch and componentSearch sections
 */
//--------------------------------------------------------------------------------------------------
void ParseSdefReadSearchPath
(
    ArgHandler_t& handler
)
{
    std::string sdefPath = path::MakeAbsolute(handler.sdefFilePath);
    // Parse the sdef file and read appSearch, componentSearch, moduleSearch sections
    const auto sdefFilePtr = parser::sdef::Parse(sdefPath, false);

    for (auto sectionPtr : sdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "appSearch")
        {
             ReadSearchDirs(handler.appSearchPath, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "componentSearch")
        {
            ReadSearchDirs(handler.compSearchPath, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "moduleSearch")
        {
            ReadSearchDirs(handler.moduleSearchPath, ToTokenListPtr(sectionPtr));
        }
    }

    // Use the search paths to get absolute paths for apps, components, modules.
    if (!handler.adefFilePath.empty() || !handler.cdefFilePath.empty())
    {
        std::string appFoundPath = file::FindFile(handler.adefFilePath, handler.appSearchPath);
        if (!appFoundPath.empty())
        {
            handler.absAdefFilePath = appFoundPath;
        }

        // When app is being created
        if (appFoundPath.empty() && !handler.appSearchPath.empty())
        {
            if(path::IsAbsolute(handler.adefFilePath))
            {
                handler.absAdefFilePath = handler.adefFilePath;
            }
            else
            {
                std::string absAppSearchPath = path::Minimize(handler.appSearchPath.front());

                if (!path::IsAbsolute(absAppSearchPath))
                {
                    absAppSearchPath = path::MakeAbsolute(absAppSearchPath);
                }

                handler.absAdefFilePath = absAppSearchPath + "/" + handler.adefFilePath;
            }
        }


        if (!handler.oldAdefFilePath.empty())
        {
            std::string oldAppFoundPath = file::FindFile(handler.oldAdefFilePath,
                                                         handler.appSearchPath);
            if (!oldAppFoundPath.empty())
            {
                handler.oldAdefFilePath = oldAppFoundPath;
            }
        }

        if (!handler.cdefFilePath.empty())
        {
            std::string compFoundPath = file::FindComponent(handler.cdefFilePath,
                                                            handler.compSearchPath);
            if (!compFoundPath.empty())
            {
                handler.absCdefFilePath = compFoundPath;
            }

            // When component is being created
            if (compFoundPath.empty() && !handler.compSearchPath.empty())
            {
                if(path::IsAbsolute(handler.cdefFilePath))
                {
                    handler.absCdefFilePath = handler.cdefFilePath;
                }
                else
                {
                    std::string absCompSearchPath = path::Minimize(handler.compSearchPath.front());

                    if (!path::IsAbsolute(absCompSearchPath))
                    {
                        absCompSearchPath = path::MakeAbsolute(absCompSearchPath);
                    }

                    handler.absCdefFilePath = absCompSearchPath + "/" + handler.cdefFilePath;
                }
            }
        }

        // Component is created when app is created
        if (handler.cdefFilePath.empty() && !handler.compSearchPath.empty())
        {
            std::string absCompSearchPath = path::Minimize(handler.compSearchPath.front());

            if (!path::IsAbsolute(absCompSearchPath))
            {
                absCompSearchPath = path::MakeAbsolute(absCompSearchPath);
            }

            handler.absCdefFilePath = absCompSearchPath + "/"
                                      + path::GetLastNode(path::RemoveSuffix(
                                                          handler.adefFilePath, ADEF_EXT))
                                      + "Component";
        }

        if (!handler.oldCdefFilePath.empty())
        {
            std::string oldCompFoundPath = file::FindComponent(handler.oldCdefFilePath,
                                                               handler.compSearchPath);
            if (!oldCompFoundPath.empty())
            {
                handler.oldCdefFilePath = oldCompFoundPath;
            }
        }
    }

    if (!handler.mdefFilePath.empty())
    {
        std::string modFoundPath = file::FindFile(handler.mdefFilePath, handler.moduleSearchPath);
        if (!modFoundPath.empty())
        {
            handler.absMdefFilePath = modFoundPath;
        }

        // When module is being created
        if (modFoundPath.empty() && !handler.moduleSearchPath.empty())
        {
            if(path::IsAbsolute(handler.mdefFilePath))
            {
                handler.absMdefFilePath = handler.mdefFilePath;
            }
            else
            {
                std::string absModSearchPath = path::Minimize(handler.moduleSearchPath.front());

                if (!path::IsAbsolute(absModSearchPath))
                {
                    absModSearchPath = path::MakeAbsolute(absModSearchPath);
                }

                handler.absMdefFilePath = absModSearchPath + "/" + handler.mdefFilePath;
            }
        }
    }

    if (!handler.oldMdefFilePath.empty())
    {
        std::string oldModFoundPath = file::FindFile(handler.oldMdefFilePath,
                                                     handler.moduleSearchPath);
        if (!oldModFoundPath.empty())
        {
            handler.oldMdefFilePath = oldModFoundPath;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Evaluate the line that needs to be written to the definition file.
 */
//--------------------------------------------------------------------------------------------------
static std::string GetLineToWrite
(
    ArgHandler_t& handler
)
{
    std::string defFile;
    std::string writePath;
    std::string lineToWrite;

    if ((handler.editActionType == ArgHandler_t::EditActionType_t::REMOVE)
         || (handler.editActionType == ArgHandler_t::EditActionType_t::DELETE))
    {
        lineToWrite = "";
        return lineToWrite;
    }

    switch (handler.editItemType)
    {
        case ArgHandler_t::EditItemType_t::APP:
        {
            defFile = handler.absAdefFilePath;
            // If appSearch section is present, list just relative app name, no need to specify
            // the absolute path.
            for (const auto& it : handler.appSearchPath)
            {
                writePath = path::EraseCommonBasePath(handler.absAdefFilePath, it, false);
                if (!writePath.empty() && !path::IsAbsolute(writePath))
                {
                    break;
                }
            }
            break;
        }
        case ArgHandler_t::EditItemType_t::MODULE:
        {
            defFile = handler.absMdefFilePath;
            // If moduleSearch section is present, list just relative module name, no need to
            // specify full absolute path.
            for (const auto& it : handler.moduleSearchPath)
            {
                writePath = path::EraseCommonBasePath(handler.absMdefFilePath, it, false);
                if (!writePath.empty() && !path::IsAbsolute(writePath))
                {
                    break;
                }
            }
            break;
        }
        case ArgHandler_t::EditItemType_t::APPSEARCH:
        case ArgHandler_t::EditItemType_t::COMPONENTSEARCH:
        case ArgHandler_t::EditItemType_t::MODULESEARCH:
        case ArgHandler_t::EditItemType_t::INTERFACESEARCH:
        {
            std::string dirPath = handler.searchPath;
            std::map<std::string, std::string> matchedEnvVarMap;

            // Iterate through the existing environment variables to match base path of the search
            // path and to replace sub-string of the search path with an environment variable.
            envVars::Iterate([&matchedEnvVarMap, &dirPath](const std::string& name,
                                                           const std::string& value)
            {
                if (value.empty())
                {
                    return;
                }

                std::size_t posFound = dirPath.find(value);
                if (posFound == 0)
                {
                    // Skip irrelevant environment variables.
                    if (name == "PWD" || name == "OLDPWD" || name == "HOME" || value == "/")
                    {
                        return;
                    }

                    // Insert matching environment variables to the map.
                    matchedEnvVarMap.insert({value, name});
                }
            });

            if (matchedEnvVarMap.size() == 0)
            {
                // If path does not match any of the environment variables. Check if base path
                // of the sdef matches the search path.
                std::string sdefDir = path::GetContainingDir(handler.absSdefFilePath);
                std::string erasedCommonPath = path::EraseCommonBasePath(dirPath, sdefDir, false);

                if (erasedCommonPath.empty())
                {
                    // Do not write empty string to sdef, write the absolute dirPath instead.
                    writePath = dirPath;
                }
                else
                {
                    // Write the relative path to sdef.
                    writePath = erasedCommonPath;
                }
            }
            else
            {
                // If there are multiple strings matching the search path's sub-string, select the
                // longest string. Since, elements in map are always sorted by its key, select the
                // last element for the environment variable value of longest string length.
                std::string matchedEnvVar = matchedEnvVarMap.rbegin()->second;
                std::string matchedEnvValue = matchedEnvVarMap.rbegin()->first;

                dirPath = path::EraseCommonBasePath(dirPath, matchedEnvValue, false);

                if (dirPath.empty())
                {
                    writePath = "${" + matchedEnvVar + "}";
                }
                else
                {
                    writePath = "${" + matchedEnvVar + "}/" + dirPath;
                }
            }

            break;
        }
        default:
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Internal error: '%s' edit item type is invalid"),
                           handler.editItemType)
            );
            break;
        }
    }

    if (writePath.empty())
    {
        writePath = path::EraseCommonBasePath(defFile, handler.absSdefFilePath, true);
    }

    if (handler.editActionType == ArgHandler_t::EditActionType_t::RENAME)
    {
        lineToWrite = writePath;
    }
    else
    {
        // mkedit adds 4 spaces infront any text added to the active definition file.
        lineToWrite = "    " + writePath;
    }

    return lineToWrite;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if section is a search section (appSearch, componentSearch, moduleSearch, interfaceSearch)
 * and the edit item type matches the corresponding search section.
 */
//--------------------------------------------------------------------------------------------------
static bool isSearchSection (const std::string& sectionName, ArgHandler_t& handler)
{
     if (((sectionName == "appSearch") &&
           (handler.editItemType == ArgHandler_t::EditItemType_t::APPSEARCH))
          || ((sectionName == "componentSearch") &&
              (handler.editItemType == ArgHandler_t::EditItemType_t::COMPONENTSEARCH))
          || ((sectionName == "moduleSearch") &&
              (handler.editItemType == ArgHandler_t::EditItemType_t::MODULESEARCH))
          || ((sectionName == "interfaceSearch") &&
              (handler.editItemType == ArgHandler_t::EditItemType_t::INTERFACESEARCH)))
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse system definition file to evaluate the line to write and its position. If the section to
 * update is not present in the defintion file, append the new section and new contents to the end
 * of the file.
 */
//--------------------------------------------------------------------------------------------------
void ParseSdefUpdateItem
(
    ArgHandler_t& handler
)
{
    std::string itemMustExist;
    std::string itemMustNotExist;
    std::string itemMustExistStrip;
    std::string itemMustNotExistStrip;

    switch (handler.editItemType)
    {
        case ArgHandler_t::EditItemType_t::APP:
                if (handler.editActionType == ArgHandler_t::EditActionType_t::ADD)
                {
                    itemMustNotExist = handler.absAdefFilePath;
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::CREATE)
                {
                    itemMustNotExist = handler.absAdefFilePath;
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::REMOVE)
                {
                    itemMustExist = handler.absAdefFilePath;
                    itemMustNotExist = "";
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::DELETE)
                {
                    itemMustExist = handler.absAdefFilePath;
                    itemMustNotExist = "";
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::RENAME)
                {
                    itemMustExist = handler.oldAdefFilePath;
                    itemMustNotExist = handler.absAdefFilePath;
                }

                itemMustExistStrip = path::GetLastNode(itemMustExist);

                if (path::HasSuffix(itemMustExistStrip, ADEF_EXT))
                {
                    itemMustExistStrip = path::RemoveSuffix(itemMustExistStrip, ADEF_EXT);
                }

                itemMustNotExistStrip =  path::GetLastNode(itemMustNotExist);

                if (path::HasSuffix(itemMustNotExistStrip, ADEF_EXT))
                {
                    itemMustNotExistStrip = path::RemoveSuffix(itemMustNotExistStrip, ADEF_EXT);
                }
            break;

        case ArgHandler_t::EditItemType_t::MODULE:
                if (handler.editActionType == ArgHandler_t::EditActionType_t::ADD)
                {
                    itemMustNotExist = handler.absMdefFilePath;
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::CREATE)
                {
                    itemMustNotExist = handler.absMdefFilePath;
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::REMOVE)
                {
                    itemMustExist = handler.absMdefFilePath;
                    itemMustNotExist = "";
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::DELETE)
                {
                    itemMustExist = handler.absMdefFilePath;
                    itemMustNotExist = "";
                }
                else if (handler.editActionType == ArgHandler_t::EditActionType_t::RENAME)
                {
                    itemMustExist = handler.oldMdefFilePath;
                    itemMustNotExist = handler.absMdefFilePath;
                }

                itemMustExistStrip = path::GetLastNode(itemMustExist);

                if (path::HasSuffix(itemMustExistStrip, MDEF_EXT))
                {
                    itemMustExistStrip = path::RemoveSuffix(itemMustExistStrip, MDEF_EXT);
                }

                itemMustNotExistStrip =  path::GetLastNode(itemMustNotExist);

                if (path::HasSuffix(itemMustNotExistStrip, MDEF_EXT))
                {
                    itemMustNotExistStrip = path::RemoveSuffix(itemMustNotExistStrip, MDEF_EXT);
                }
            break;

        case ArgHandler_t::EditItemType_t::APPSEARCH:
        case ArgHandler_t::EditItemType_t::COMPONENTSEARCH:
        case ArgHandler_t::EditItemType_t::MODULESEARCH:
        case ArgHandler_t::EditItemType_t::INTERFACESEARCH:
            if (handler.editActionType == ArgHandler_t::EditActionType_t::ADD)
            {
                itemMustNotExist = handler.searchPath;
            }
            break;

        default:
            break;
    };

    // Parse the sdef file and look for the section to update
    const auto sdefFilePtr = parser::sdef::Parse(handler.absSdefFilePath, false);

    bool foundSection = false;
    bool foundItem = false;
    int foundPos, nextPos, length, endPos = 0;
    std::string lineToWrite = GetLineToWrite(handler);

    for (auto sectionPtr : sdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if ((sectionName == "apps") && (handler.editItemType == ArgHandler_t::EditItemType_t::APP))
        {
            // There can be multiple files with apps: section included in the active sdef.
            // Need to make sure only the active sdef is looked into for updating apps.
            std::size_t found = sectionPtr->lastTokenPtr->GetLocation().find(sdefFilePtr->path);

            if (found != std::string::npos)
            {
                foundSection = true;
                length = sectionPtr->lastTokenPtr->curPos;

                auto appsSectionPtr =
                            dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);
                if (appsSectionPtr == NULL)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                                   sectionName)
                    );
                }

                for (auto itemPtr : appsSectionPtr->Contents())
                {
                    const auto appPtr = dynamic_cast<const parseTree::App_t*>(itemPtr);
                    if (appPtr == NULL)
                    {
                        throw mk::Exception_t(
                            mk::format(LE_I18N("Internal error: '%s' section item pointer is NULL"),
                                       sectionName)
                        );
                    }

                    const auto appSpec = path::Unquote(DoSubstitution(appPtr->firstTokenPtr));
                    std::string appSpecStrip;

                    appSpecStrip = path::GetLastNode(appSpec);

                    if (path::HasSuffix(appSpecStrip, ADEF_EXT))
                    {
                        appSpecStrip = path::RemoveSuffix(appSpecStrip, ADEF_EXT);
                    }

                    if (appSpecStrip.compare(itemMustExistStrip) == 0)
                    {
                        foundItem = true;
                        foundPos = itemPtr->lastTokenPtr->curPos;
                        nextPos = itemPtr->firstTokenPtr->nextPtr->curPos;

                        if (handler.isPrintLogging())
                        {
                            std::cout << mk::format(
                                            LE_I18N("\nApp '%s' found in apps: section in '%s'."),
                                            itemMustExistStrip,
                                            itemPtr->lastTokenPtr->GetLocation()
                                         );
                        }
                    }

                    if (!itemMustNotExist.empty())
                    {
                        if (appSpecStrip.compare(itemMustNotExistStrip) == 0)
                        {
                            throw mk::Exception_t(
                                   mk::format(LE_I18N("App already listed: '%s'"),
                                              itemPtr->lastTokenPtr->GetLocation())
                            );
                        }
                    }
                }
           } //end of if
        } // end of if

        else if (parser::IsNameSingularPlural(sectionName, "kernelModule") &&
            (handler.editItemType == ArgHandler_t::EditItemType_t::MODULE))
        {
            // There can be multiple files with kernelModules: section included with active sdef.
            // Need to make sure only the active sdef is looked into for updating apps.
            std::size_t found = sectionPtr->lastTokenPtr->GetLocation().find(sdefFilePtr->path);

            length = sectionPtr->lastTokenPtr->curPos;

            if (found != std::string::npos)
            {
                foundSection = true;
                auto moduleSectionPtr =
                            dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);
                if (moduleSectionPtr == NULL)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                                   sectionName)
                    );
                }

                for (auto itemPtr : moduleSectionPtr->Contents())
                {
                    const parseTree::RequiredModule_t* modulePtr =
                                        dynamic_cast<const parseTree::RequiredModule_t*>(itemPtr);
                   if (modulePtr == NULL)
                    {
                        throw mk::Exception_t(
                            mk::format(LE_I18N("Internal error: '%s' section item pointer is NULL"),
                                       sectionName)
                        );
                    }

                    const auto moduleSpec = path::Unquote(DoSubstitution(modulePtr->firstTokenPtr));

                    std::string moduleSpecStrip;
                    moduleSpecStrip = path::GetLastNode(moduleSpec);

                    if (path::HasSuffix(moduleSpecStrip, MDEF_EXT))
                    {
                        moduleSpecStrip = path::RemoveSuffix(moduleSpecStrip, MDEF_EXT);
                    }

                    if (moduleSpecStrip.compare(itemMustExistStrip) == 0)
                    {
                        foundItem = true;
                        foundPos = itemPtr->lastTokenPtr->curPos;
                        nextPos = itemPtr->firstTokenPtr->nextPtr->curPos;

                        if (handler.isPrintLogging())
                        {
                            std::cout << mk::format(
                                            LE_I18N("\nModule '%s' found in kernelModules: section "
                                                    "at '%s'."),
                                            itemMustExistStrip, itemPtr->lastTokenPtr->GetLocation()
                                         );
                        }
                    }

                    if (!itemMustNotExist.empty())
                    {
                        if (moduleSpecStrip.compare(itemMustNotExistStrip) == 0)
                        {
                            throw mk::Exception_t(
                                   mk::format(LE_I18N("Module already listed: '%s'"),
                                              itemPtr->lastTokenPtr->GetLocation())
                            );
                        }
                    }
               } //end of for
            } //end of if
        } //end of if

        else if (isSearchSection(sectionName, handler))
        {
            // There can be multiple files with search section included in the active sdef.
            // Need to make sure only the active sdef is looked into for updating apps.
            std::size_t found = sectionPtr->lastTokenPtr->GetLocation().find(sdefFilePtr->path);

            if (found != std::string::npos)
            {
                foundSection = true;
                length = sectionPtr->lastTokenPtr->curPos;
            }

            auto intSearchSectionPtr =
                        dynamic_cast<const parseTree::TokenList_t*>(sectionPtr);
            if (intSearchSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                               sectionName)
                );
            }

            for (const auto contentItemPtr : intSearchSectionPtr->Contents())
            {
                auto tokenPtr = dynamic_cast<const parseTree::Token_t*>(contentItemPtr);

                auto dirPath = path::Unquote(DoSubstitution(tokenPtr));

                if (!itemMustNotExist.empty())
                {
                    if (dirPath.compare(itemMustNotExist) == 0)
                    {
                        throw mk::Exception_t(
                               mk::format(LE_I18N("interfaceSearch already listed: '%s'"),
                                          contentItemPtr->GetLocation())
                        );
                    }
                }
            }
        }
        else
        {
            // Evaluate end positon which is closer to the end of the file.
            std::size_t found = sectionPtr->lastTokenPtr->GetLocation().find(sdefFilePtr->path);
            if (found != std::string::npos)
            {
                endPos = sectionPtr->lastTokenPtr->nextPtr->curPos;
            }
        }
    } // end of for

    if (foundSection)
    {
        if (!itemMustExist.empty() && foundItem)
        {
            // Rename or remove app from sdef
            handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite,
                                                                               foundPos, nextPos});
        }
        else if (!itemMustExist.empty() && !foundItem)
        {
            // If the app to be renamed or removed is not found on the list, print error.
            throw mk::Exception_t(
                   mk::format( LE_I18N("'%s' not listed in Sdef"), itemMustExist)
            );
        }
        else if (!itemMustNotExist.empty())
        {
            // Add app to sdef
            handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite,
                                                                               length, length-1});
        }
    }

    // If the section is not found then add new section and append to the end of the sdef file
    if (!foundSection && !itemMustNotExist.empty() && itemMustExist.empty())
    {
        std::string strWrite;

        switch (handler.editItemType)
        {
            // Evaluate line to be written to the end of the file.
            case ArgHandler_t::EditItemType_t::APP:
                  strWrite = "\n\napps:\n{\n" + lineToWrite + "\n}\n";
                  break;

            case ArgHandler_t::EditItemType_t::MODULE:
                  strWrite = "\n\nkernelModules:\n{\n" + lineToWrite + "\n}\n";
                  break;

            case ArgHandler_t::EditItemType_t::APPSEARCH:
                  strWrite = "\n\nappSearch:\n{\n" + lineToWrite + "\n}\n";
                  break;

            case ArgHandler_t::EditItemType_t::COMPONENTSEARCH:
                  strWrite = "\n\ncomponentSearch:\n{\n" + lineToWrite + "\n}\n";
                  break;

            case ArgHandler_t::EditItemType_t::MODULESEARCH:
                  strWrite = "\n\nmoduleSearch:\n{\n" + lineToWrite + "\n}\n";
                  break;

            case ArgHandler_t::EditItemType_t::INTERFACESEARCH:
                  strWrite = "\n\ninterfaceSearch:\n{\n" + lineToWrite + "\n}\n";
                  break;

            default:
                  throw mk::Exception_t(
                            mk::format(LE_I18N("Internal: '%s' edit item type is invalid."),
                                       handler.editItemType)
                  );
                  break;
        }

        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{strWrite, endPos,
                                                                           endPos});

        if (handler.isPrintLogging())
        {
            std::cout << mk::format(
                            LE_I18N("Section not found. Append '%s' to end of the file '%s'."),
                            strWrite, handler.absSdefFilePath
                         );
        }
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Get the position of component element in the tracking file.
 *
 * handler: Handler argument passed on the command line.
 * memberPtr: Pointer defines the subsection of the "sectionName" section.
 * sectionName: The section name.
 * compList: Component that must be already listed in the definition file.
 * reqCompPosition: The position of component.
 * reqCompPositionList: The list of the component's positions.
 * foundItem: Boolean flag to check if the component is found.
 **/
//-------------------------------------------------------------------------------------------------
static void GetPositionOfComponentElement
(
    ArgHandler_t& handler,
    const parseTree::CompoundItem_t* memberPtr,
    const std::string& sectionName,
    const std::string& compList,
    CompPosition_t& reqCompPosition,
    std::vector<CompPosition_t>& reqCompPositionList,
    bool& foundItem
)
{
    auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
    for (auto itSecton = subsectionPtr->Contents().rbegin();
        itSecton != subsectionPtr->Contents().rend();
        ++itSecton)
    {
        auto compSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(
                                                                    subsectionPtr);

        if (compSectionPtr == NULL)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                            sectionName)
            );
        }

        for (auto itemPtr : compSectionPtr->Contents())
        {
            const parseTree::RequiredComponent_t* compPtr =
                dynamic_cast<const parseTree::RequiredComponent_t*>(itemPtr);

            if (compPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N(
                        "Internal error: '%s' section content pointer is NULL"),
                        sectionName)
                );
            }

            for (auto tokenPtr : compPtr->Contents())
            {
                // If component to rename/remove is found in the list of components
                if (path::GetLastNode(compList).compare(
                    path::GetLastNode(tokenPtr->text)) == 0)
                {
                    reqCompPosition.foundPos = tokenPtr->curPos;
                    reqCompPosition.nextPos = tokenPtr->nextPtr->curPos;
                    reqCompPosition.sectionPos = itemPtr->firstTokenPtr->curPos;
                    reqCompPosition.sectionNextPos =
                        itemPtr->lastTokenPtr->nextPtr->curPos;
                    reqCompPositionList.push_back(reqCompPosition);
                    foundItem =true;

                    if (handler.isPrintLogging())
                    {
                        std::cout << mk::format(
                            LE_I18N("\nComponent '%s' found in '%s' section '%s'"),
                            path::GetLastNode(compList),
                            sectionName,
                            tokenPtr->GetLocation()
                        );
                    }
                }
            }
        }
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Parse the component definition file to update the component: section.
 * 1. Parse the cdef file to check if cdef contains components listed in coponent: section.
 * 2. Update the cdef file by adding the component in coponent: section.
 *
 * compList: Component that must be already listed in the definition file.
 * compNotList: Component that must not be already listed in the definition file.
 **/
//-------------------------------------------------------------------------------------------------
static void ParseCdefGetEditLinePosition
(
    ArgHandler_t& handler,
    const std::string& cdefPath,
    const std::string& compList,
    const std::string& compNotList
)
{
    CompPosition_t reqCompPosition {false, 0, 0, 0, 0};
    // Boolean flag to check if the component is found in the component: section.
    bool foundItem = false;
    // List of components found in the requires: component: section.
    std::vector<CompPosition_t> reqCompPositionList;
    handler.linePositionToWrite.clear();
    auto cdefFilePtr = parser::cdef::Parse(cdefPath, false);
    // Iterate over the .cdef file's list of sections.

    for (auto sectionPtr : cdefFilePtr->sections)
    {
        const std::string& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "requires")
        {
            // "requires:" section is comprised of subsections
            for (auto memberPtr :
                static_cast<const parseTree::ComplexSection_t*>(sectionPtr)->Contents())
            {
                const std::string& subsectionName = memberPtr->firstTokenPtr->text;

                if (subsectionName == "component")
                {
                    GetPositionOfComponentElement(handler, memberPtr, sectionName, compList,
                                                reqCompPosition, reqCompPositionList, foundItem);
                }
            }
        }
    }

    std::string compName;
    std::string compPath;
    compName = path::GetLastNode(compNotList);

    // If componentSearch section is present, list the relative component name.
    // No need to specify full absolute path.
    for (const auto& it : handler.compSearchPath)
    {
        compPath = path::EraseCommonBasePath(compNotList, it, false);
        if (!compPath.empty())
        {
            break;
        }
    }

    if (compPath.empty())
    {
        compPath = path::EraseCommonBasePath(compNotList, handler.absCdefFilePath, true);
    }

    std::string lineToWrite1;
    std::string lineToWrite2;
    std::string lineToWrite3;

    if (!compList.empty()  && !compNotList.empty())
    {
        // Rename component in adef
        if (foundItem)
        {
            // Append the component path to the cdef in the component: section
            lineToWrite1 =  compPath;

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(LE_I18N("\nRename component to '%s' in components: or "
                                                "executables: section"), compPath
                );
            }
        }
        else
        {
            // Component to rename not found on the list, throw error.
            throw mk::Exception_t(
                mk::format(LE_I18N("Component '%s' not listed in either components: or "
                                   "executables: section of '%s'."), compList, cdefPath)
            );
        }

        for (auto it : reqCompPositionList)
        {
            handler.linePositionToWrite.push_back(
                ArgHandler_t::LinePosition_t{lineToWrite1,it.foundPos, it.nextPos});
        }
    }
    else if (!compList.empty() && compNotList.empty())
    {
        // Remove component from cdef.
        if (foundItem)
        {
            for (const auto& it : reqCompPositionList)
            {
                handler.linePositionToWrite.push_back(
                    ArgHandler_t::LinePosition_t{"", it.foundPos, it.nextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(LE_I18N(
                                        "\nRemove component '%s' from components: or executables: "
                                        "section."), path::GetLastNode(compList)
                );
            }
        }
        else
        {
            // Component to be removed not found on the list, throw error.
            throw mk::Exception_t(
                mk::format(LE_I18N("Component '%s' not listed in either components: or "
                                   "executables: section of '%s'."), compList, cdefPath)
            );
        }
    }
    else
    {
        // Unhandled case.
        throw mk::Exception_t(
            mk::format(LE_I18N("Internal error: Unhandled case when getting line to edit in '%s'"),
                       cdefPath)
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the application definition file to update the components: or executables: section.
 * 1. Parse the cdef file to check if cdef contains .c files listed in sources: section.
 * 2. Parse the adef file:
 *    a. If cdef contains sources: section, update the executables: section of the adef.
 *    b. If cdef does not contain sources: section, add component to components: section of adef.
 * 3. Update the adef file by adding the component in either sources or executables section.
 *
 * compList: Component that must be already listed in the definition file.
 * compNotList: Component that must not be already listed in the definition file.
 **/
//--------------------------------------------------------------------------------------------------
static void ParseAdefGetEditLinePosition
(
    ArgHandler_t& handler,
    std::string adefPath,
    std::string cdefPath,
    std::string compList,
    std::string compNotList
)
{
    bool sourcesSectionExist = false;
    auto cdefFilePtr = parser::cdef::Parse(cdefPath, false);

    // Iterate over the .cdef file's list of sections.
    for (auto sectionPtr : cdefFilePtr->sections)
    {
        const std::string& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "sources")
        {
            sourcesSectionExist = true;
        }
    }

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nSearching component '%s' in ADEF file '%s'."),
                                compList, adefPath
                     );
    }

    auto adefFilePtr = parser::adef::Parse(adefPath, false);

    // Position of the end of the components: or executables: section.
    int length1 = 0;
    // Boolean flag to check if the component is found in the components: or executables: section.
    bool foundItem1 = false;

    // Position of the end of the component in processes: run: section.
    int length2 = 0;
    // Boolean flag to check if the executable name is found in the processes: run: section.
    bool foundItem2 = false;

    // Boolean flag to check if the component name is found in the bindings: section.
    bool foundItem3 = false;

    // Boolean flag to check if the component name is found in the extern: section.
    bool foundItem4 = false;

    CompPosition_t compPosition {false, 0, 0, 0, 0};
    CompPosition_t exeCompPosition {false, 0, 0, 0, 0};
    CompPosition_t procRunPosition {false, 0, 0, 0, 0};
    CompPosition_t bindingPosition {false, 0, 0, 0, 0};
    CompPosition_t externPosition {false, 0, 0, 0, 0};

    // List of components found in the components: section.
    std::vector<CompPosition_t> compPositionList;
    // List of components found in the executables: section.
    std::vector<CompPosition_t> exeCompPositionList;
    // List of executables related to the component found in the processes: run: section.
    std::vector<CompPosition_t> procRunPositionList;
    // List of components found in the bindings: section.
    std::vector<CompPosition_t> bindingPositionList;
    // List of components found in the extern: section.
    std::vector<CompPosition_t> externPositionList;

    // List of executables containing only single component.
    std::vector<std::string> singleCompExe;

    handler.linePositionToWrite.clear();

    // Iterate over the .adef file's list of sections, processing content items.
    for (auto sectionPtr : adefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sourcesSectionExist && (sectionName == "executables"))
        {
            auto exeSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);

            if (exeSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"), sectionName)
                );
            }

            for (auto itemPtr : exeSectionPtr->Contents())
            {
                const parseTree::Executable_t* exePtr =
                                    dynamic_cast<const parseTree::Executable_t*>(itemPtr);
                if (exePtr == NULL)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("Internal error: '%s' section content pointer is NULL"),
                                   sectionName)
                    );
                }

                if (exePtr->Contents().size() > 1)
                {
                    // If the executables: section contain list of multiple components, set bool
                    // flag exeMultiComp to true.
                    exeCompPosition.isExeMultiComp = true;
                }

                for (auto tokenPtr : exePtr->Contents())
                {
                    // Resolve the path to the component.
                    std::string componentPath = path::Unquote(DoSubstitution(tokenPtr));
                    if (path::GetLastNode(compList).compare(path::GetLastNode(componentPath)) == 0)
                    {
                        foundItem1 = true;

                        // If executables doesn't contain multiple components.
                        if (!exeCompPosition.isExeMultiComp)
                        {
                            // Push the executable name into the vector.
                            singleCompExe.push_back(itemPtr->firstTokenPtr->text);
                        }

                        exeCompPosition.foundPos = tokenPtr->curPos;
                        exeCompPosition.nextPos = tokenPtr->nextPtr->curPos;
                        exeCompPosition.sectionPos = itemPtr->firstTokenPtr->curPos;
                        exeCompPosition.sectionNextPos = itemPtr->lastTokenPtr->nextPtr->curPos;

                        exeCompPositionList.push_back(exeCompPosition);

                        if (handler.isPrintLogging())
                        {
                            std::cout << mk::format(
                                            LE_I18N("\nComponent '%s' found in '%s' section '%s'"),
                                            path::GetLastNode(compList), sectionName,
                                            tokenPtr->GetLocation()
                                         );
                        }
                    }

                    if (path::GetLastNode(compNotList).compare(path::GetLastNode(componentPath))
                        == 0)
                    {
                        throw mk::Exception_t(
                               mk::format(LE_I18N("Component already listed: '%s'"),
                                          itemPtr->lastTokenPtr->GetLocation())
                        );
                    }
                }
            }
            length1 = sectionPtr->lastTokenPtr->curPos;
        }

        if (!sourcesSectionExist && (sectionName == "components"))
        {
            auto componentSectionPtr =
                        dynamic_cast<const parseTree::TokenListSection_t*>(sectionPtr);
            if (componentSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"), sectionName)
                );
            }

            for (auto tokenPtr : componentSectionPtr->Contents())
            {
                // Resolve the path to the component.
                std::string componentPath = path::Unquote(DoSubstitution(tokenPtr));

                if (path::GetLastNode(compList).compare(path::GetLastNode(componentPath)) == 0)
                {
                    foundItem1 = true;

                    compPosition.foundPos = tokenPtr->curPos;
                    compPosition.nextPos = tokenPtr->nextPtr->curPos;
                    compPositionList.push_back(compPosition);

                    if (handler.isPrintLogging())
                    {
                        std::cout << mk::format(
                                        LE_I18N("\nComponent '%s' found in '%s' section at '%s'"),
                                        path::GetLastNode(compList), sectionName,
                                        tokenPtr->GetLocation()
                                     );
                    }
                }

                if (path::GetLastNode(compNotList).compare(path::GetLastNode(componentPath)) == 0)
                {
                    throw mk::Exception_t(
                           mk::format(LE_I18N("Component already listed: '%s'"),
                                      componentSectionPtr->lastTokenPtr->GetLocation())
                    );
                }
            }
            length1 = sectionPtr->lastTokenPtr->curPos;
        }

        if (sourcesSectionExist && (sectionName == "processes"))
        {
            auto processesSectionPtr =
                        dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);
            if (processesSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"), sectionName)
                );
            }

            for (auto subsectionPtr : processesSectionPtr->Contents())
            {
                auto& subsectionName = subsectionPtr->firstTokenPtr->text;

                if (subsectionName == "run")
                {
                    auto runSectionPtr =
                            dynamic_cast<const parseTree::CompoundItemList_t*>(subsectionPtr);

                    if (runSectionPtr == NULL)
                    {
                        throw mk::Exception_t(
                            mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                                       sectionName)
                        );
                    }

                    // Each item in this section is a process specification in the form of a
                    // TokenList_t.
                    for (auto itemPtr : runSectionPtr->Contents())
                    {
                        auto processSpecPtr = dynamic_cast<const parseTree::RunProcess_t*>(itemPtr);
                        if (processSpecPtr == NULL)
                        {
                            itemPtr->ThrowException(
                                mk::format(LE_I18N("Internal error: '%s'' is not a RunProcess_t."),
                                           itemPtr->TypeName())
                            );
                        }
                        auto tokens = processSpecPtr->Contents();
                        auto i = tokens.begin();

                        // In case the tokens are empty, go on to the next process specification.
                        if (i != tokens.end())
                        {
                            auto procName = (*i)->text;
                            for (const auto& it : singleCompExe)
                            {
                                // If executable with single component  match the process name then
                                // mark for removal. This is used only for remove cases.
                                if (it.compare(procName) == 0)
                                {
                                    foundItem2 = true;

                                    procRunPosition.foundPos = itemPtr->firstTokenPtr->curPos;
                                    procRunPosition.nextPos = itemPtr->lastTokenPtr->nextPtr->curPos;

                                    if (handler.isPrintLogging())
                                    {
                                        std::cout << mk::format(
                                                        LE_I18N("\nProcess '%s' found in '%s' "
                                                                "section '%s'"),
                                                        procName, sectionName,
                                                        itemPtr->firstTokenPtr->GetLocation()
                                                     );
                                    }
                                }
                            }
                        }
                    }
                    procRunPositionList.push_back(procRunPosition);
                    length2 = subsectionPtr->lastTokenPtr->curPos;
                }
            }
        } //end of if

        if (sectionName == "bindings")
        {
            auto bindSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);

            if (bindSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"), sectionName)
                );
            }

            for (auto itemPtr : bindSectionPtr->Contents())
            {
                const parseTree::Binding_t* bindPtr =
                                    dynamic_cast<const parseTree::Binding_t*>(itemPtr);
                if (bindPtr == NULL)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("Internal error: '%s' section content pointer is NULL"),
                                   sectionName)
                    );
                }

                for (auto tokenPtr : bindPtr->Contents())
                {
                    // If component to rename/remove is found in the list of bindings.
                    if (path::GetLastNode(compList).compare(tokenPtr->text) == 0)
                    {
                        foundItem3  = true;

                        bindingPosition.foundPos = tokenPtr->curPos;
                        bindingPosition.nextPos = tokenPtr->nextPtr->curPos;
                        bindingPosition.sectionPos = itemPtr->firstTokenPtr->curPos;
                        bindingPosition.sectionNextPos = itemPtr->lastTokenPtr->nextPtr->curPos;

                        bindingPositionList.push_back(bindingPosition);

                        if (handler.isPrintLogging())
                        {
                            std::cout << mk::format(
                                            LE_I18N("\nComponent '%s' found in '%s' section '%s'"),
                                            path::GetLastNode(compList), sectionName,
                                            tokenPtr->GetLocation()
                                         );
                        }
                    }
                }
            }
        } //end of if

        if (sectionName == "extern")
        {
            auto externSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);

            if (externSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                               sectionName)
                );
            }

            for (auto itemPtr : externSectionPtr->Contents())
            {
                const parseTree::ExternApiInterface_t* externPtr =
                                    dynamic_cast<const parseTree::ExternApiInterface_t*>(itemPtr);
                if (externPtr == NULL)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("Internal error: '%s' section content pointer is NULL"),
                                   sectionName)
                    );
                }

                for (auto tokenPtr : externPtr->Contents())
                {
                    // If component to rename/remove is found in the list of externs.
                    if (path::GetLastNode(compList).compare(tokenPtr->text) == 0)
                    {
                        foundItem4  = true;

                        externPosition.foundPos = tokenPtr->curPos;
                        externPosition.nextPos = tokenPtr->nextPtr->curPos;
                        externPosition.sectionPos = itemPtr->firstTokenPtr->curPos;
                        externPosition.sectionNextPos = itemPtr->lastTokenPtr->nextPtr->curPos;

                        externPositionList.push_back(externPosition);

                        if (handler.isPrintLogging())
                        {
                            std::cout << mk::format(
                                            LE_I18N("\nComponent '%s' found in '%s' section '%s'"),
                                            path::GetLastNode(compList), sectionName,
                                            tokenPtr->GetLocation()
                                         );
                        }
                    }
                }
            }
        } //end of if
    } // end of for

    std::string compName;
    std::string compPath;

    compName = path::GetLastNode(compNotList);

    // If componentSearch section is present, list the relative component name.
    // No need to specify full absolute path.
    for (const auto& it : handler.compSearchPath)
    {
        compPath = path::EraseCommonBasePath(compNotList, it, false);
        if (!compPath.empty())
        {
            break;
        }
    }

    if (compPath.empty())
    {
        compPath = path::EraseCommonBasePath(compNotList, handler.absAdefFilePath, true);
    }

    std::string lineToWrite1;
    std::string lineToWrite2;
    std::string lineToWrite3;
    std::string lineToWrite4;

    if (!compList.empty()  && !compNotList.empty())
    {
        // Rename component in adef
        if (foundItem1)
        {
            if (sourcesSectionExist)
            {
                // Append the executable and component path to the adef in the executables: section
                lineToWrite1 =  compPath;
            }
            else
            {
                // Append the component path to the adef in the components: section
                lineToWrite1 = "    " + compPath;
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nRename component to '%s' in components: or executables: "
                                        "section"), compPath
                             );
            }
        }
        else
        {
            // Component to rename not found on the list, throw error.
            throw mk::Exception_t(
                mk::format(LE_I18N(
                           "Component '%s' not listed in either components: or executables: section"
                           " of '%s'."), compList, adefPath)
            );
        }

        for (auto it : exeCompPositionList)
        {
            handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite1,
                                                                       it.foundPos, it.nextPos});
        }

        for (const auto& it : compPositionList)
        {
             handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite1,
                                                                        it.foundPos, it.nextPos});
        }

        if (foundItem3)
        {
            lineToWrite3 = path::GetLastNode(compNotList);

            for (auto it : bindingPositionList)
            {
                handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite3,
                                                                          it.foundPos, it.nextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(LE_I18N("\nRename component to '%s' in bindings: section."),
                                        lineToWrite3
                             );
            }
        }

        if (foundItem4)
        {
            lineToWrite4 = path::GetLastNode(compNotList);

            for (auto it : externPositionList)
            {
                handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite4,
                                                                         it.foundPos, it.nextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(LE_I18N("\nRename component to '%s' in extern: section."),
                                        lineToWrite4
                             );
            }
        }
    }
    else if (!compList.empty() && compNotList.empty())
    {
        // Remove component from adef.
        if (foundItem1)
        {
            for (const auto& it : exeCompPositionList)
            {
                if (!it.isExeMultiComp)
                {
                    handler.linePositionToWrite.push_back(
                            ArgHandler_t::LinePosition_t{"", it.sectionPos, it.sectionNextPos});
                }
                else
                {
                    handler.linePositionToWrite.push_back(
                            ArgHandler_t::LinePosition_t{"", it.foundPos, it.nextPos});
                }
            }

            for (const auto& it : compPositionList)
            {
                 handler.linePositionToWrite.push_back(
                         ArgHandler_t::LinePosition_t{"", it.foundPos, it.nextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nRemove component '%s' from components: or executables: "
                                        "section."), path::GetLastNode(compList)
                             );
            }
        }
        else
        {
            // Component to be removed not found on the list, throw error.
            throw mk::Exception_t(
                mk::format(LE_I18N(
                           "Component '%s' not listed in either components: or executables: section"
                           " of '%s'."), compList, adefPath)
            );
        }

        if (foundItem2)
        {
            for (const auto& it : procRunPositionList)
            {
                handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{"", it.foundPos,
                                                                                   it.nextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << LE_I18N("\nRemove process name from processes: run: section.");
            }

        }

        if (foundItem3)
        {
            for (const auto& it : bindingPositionList)
            {
                handler.linePositionToWrite.push_back(
                                ArgHandler_t::LinePosition_t{"", it.sectionPos, it.sectionNextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nRemove bindings with component '%s' from bindings: "
                                        "section."), path::GetLastNode(compList)
                             );
            }
        }

        if (foundItem4)
        {
            for (const auto& it : externPositionList)
            {
                handler.linePositionToWrite.push_back(
                               ArgHandler_t::LinePosition_t{"", it.sectionPos, it.sectionNextPos});
            }

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nRemove extern with component '%s' from extern: "
                                        "section."), path::GetLastNode(compList)
                             );
            }
        }
    }
    else if (!compNotList.empty())
    {
        if (sourcesSectionExist)
        {
            // Append the executable and component path to the adef in the executables: section
            lineToWrite1 = "    " + compName + "Exe = ( " +
                          compPath + " )";
            lineToWrite2 = "    ( " + compName + "Exe )";

            // Adding component to adef
            handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite1,
                                                                               length1, length1-1});
            handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite2, length2,
                                                                               length2-1});

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nAdd '%s' and '%s' to executables: and processes: run: "
                                        "section."), lineToWrite1, lineToWrite2
                             );
            }
        }
        else
        {
            // Append the component path to the adef in the components: section
            lineToWrite1 = "    " + compPath;
            handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite1,
                                                                               length1, length1-1});
            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nAdd '%s' to components: section."), lineToWrite1
                             );
            }
       }
    }
    else
    {
        // Unhandled case.
        throw mk::Exception_t(
           mk::format(LE_I18N(
                      "Internal error: Unhandled case when getting line to edit in '%s'"), adefPath)
        );
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Evaluate CDEF file path and component's relevant string
 * and its position to write in another CDEF.
 **/
//-------------------------------------------------------------------------------------------------
void GetCdefComponentEditLinePosition
(
    ArgHandler_t& handler,
    const std::string& cdefTestFilePath
)
{
    std::string compMustExist;
    std::string compMustNotExist;

    switch(handler.editActionType)
    {
        case  ArgHandler_t::EditActionType_t::REMOVE:
        {
            compMustExist = handler.absCdefFilePath;
            compMustNotExist = "";
            break;
        }

        case  ArgHandler_t::EditActionType_t::RENAME:
        {
            compMustExist = handler.oldCdefFilePath;
            compMustNotExist = handler.absCdefFilePath;
            break;
        }

        default:
        {
            throw mk::Exception_t(LE_I18N("Internal error: Invalid edit action type."));
            break;
        }
    }
    // Parse the definition files to get the line and its position to write.
    ParseCdefGetEditLinePosition(handler, cdefTestFilePath, compMustExist, compMustNotExist);
}


//--------------------------------------------------------------------------------------------------
/**
 * Evaluate CDEF file path and component's relevant string and its position to write in ADEF.
 **/
//--------------------------------------------------------------------------------------------------
void GetAdefComponentEditLinePosition
(
    ArgHandler_t& handler,
    model::System_t* systemPtr
)
{
    std::string absCdefFile = handler.absCdefFilePath + "/" + COMP_CDEF;
    std::string compMustExist;
    std::string compMustNotExist;

    switch(handler.editActionType)
    {
        case  ArgHandler_t::EditActionType_t::ADD:
        case  ArgHandler_t::EditActionType_t::CREATE:
        {
            compMustExist = "";
            compMustNotExist = handler.absCdefFilePath;
            if (handler.adefFilePath.empty())
            {
               return;
            }
            break;
        }
        case  ArgHandler_t::EditActionType_t::REMOVE:
        {
            compMustExist = handler.absCdefFilePath;
            compMustNotExist = "";
            break;
        }
        case  ArgHandler_t::EditActionType_t::DELETE:
        {
            compMustExist = handler.absCdefFilePath;
            compMustNotExist = "";
            break;
        }

        case  ArgHandler_t::EditActionType_t::RENAME:
        {
            compMustExist = handler.oldCdefFilePath;
            compMustNotExist = handler.absCdefFilePath;

            std::string absOldCdefFile = handler.oldCdefFilePath + "/" + COMP_CDEF;
            absCdefFile = absOldCdefFile;
            break;
        }

        default:
        {
            throw mk::Exception_t(
                    LE_I18N("Internal error: Invalid edit action type.")
            );
            break;
        }
    }

    // Parse the definition files to get the line and its position to write.
    ParseAdefGetEditLinePosition(handler, handler.absAdefFilePath, absCdefFile, compMustExist,
                                 compMustNotExist);
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a particular section in ADEF and evaluate the string to write and its position in ADEF.
 **/
//--------------------------------------------------------------------------------------------------
void GetAdefSectionEditLinePosition
(
    ArgHandler_t& handler,
    std::string section
)
{
    auto adefFilePtr = parser::adef::Parse(handler.absAdefFilePath, false);
    if (adefFilePtr == NULL)
    {
        throw mk::Exception_t(
           mk::format(LE_I18N("Internal error: '%s' file pointer is NULL"), handler.absAdefFilePath)
        );
    }

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(
                        LE_I18N("Searching section '%s' in ADEF file '%s'."),
                        section, handler.absAdefFilePath
                     );
    }

    bool foundSection = false;
    int foundPos, nextPos, endPos = 0;

    // Iterate over the .adef file's list of sections, processing content items.
    for (auto sectionPtr : adefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == section)
        {
            foundSection = true;
            auto foundSectionPtr = dynamic_cast<const parseTree::SimpleSection_t*>(sectionPtr);
            if (foundSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"), sectionName)
                );
            }

            std::string sectionValue = foundSectionPtr->Text();

            if ((sectionName == "sandboxed") && (sectionValue == handler.appSandboxed))
            {
                std::cout << mk::format(LE_I18N("'sandboxed' value is already '%s'.\n"),
                                        handler.appSandboxed);
                return;
            }

            if ((sectionName == "start") && (sectionValue == handler.appStart))
            {
                std::cout << mk::format(LE_I18N("'start' value is already '%s'.\n"),
                                        handler.appStart);
                return;
            }

            foundPos = sectionPtr->lastTokenPtr->curPos;
            nextPos = sectionPtr->lastTokenPtr->nextPtr->curPos;

            if (handler.isPrintLogging())
            {
                std::cout << mk::format(
                                LE_I18N("\nSection '%s' found in '%s'."),
                                section, sectionPtr->lastTokenPtr->GetLocation()
                             );
            }
        }
        endPos = sectionPtr->lastTokenPtr->nextPtr->curPos;
    }

    std::string strWrite;

    if (!foundSection)
    {
        // If section is not found, append section to the end of the definition file.
        if (!handler.appSandboxed.empty())
        {
            strWrite = "\n\nsandboxed: " + handler.appSandboxed + "\n";
        }

        if (!handler.appStart.empty())
        {
            strWrite = "\n\nstart: " + handler.appStart + "\n";
        }

        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{strWrite, endPos,
                                                                           endPos});
        if (handler.isPrintLogging())
        {
            std::cout << mk::format(
                            LE_I18N("\nSection '%s' not found. Append '%s' to the end of file."),
                            section, strWrite
                         );
        }
    }
    else
    {
        // If section is found, update the section's value in place.
        if (section == "sandboxed")
        {
            strWrite = handler.appSandboxed;
        }
        else if (section == "start")
        {
            strWrite = handler.appStart;
        }

        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{strWrite, foundPos,
                                                                           nextPos});
        if (handler.isPrintLogging())
        {
            std::cout << mk::format(LE_I18N("\nUpdate section '%s' to '%s' ."), section, strWrite);
        }
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Evaluate the component definition file to update depending on the edit item type. Parse
 * component defintion file to evaluate the line to be written and the position in component
 * definition file to write.
 **/
//-------------------------------------------------------------------------------------------------
void EvaluateCdefGetEditLinePosition
(
    ArgHandler_t& handler,
    const std::string cdefTestFilePath
)
{
    switch (handler.editItemType)
    {
        case ArgHandler_t::EditItemType_t::COMPONENT:
            GetCdefComponentEditLinePosition(handler, cdefTestFilePath);
            break;

        default:
            throw mk::Exception_t(
                    LE_I18N("Internal error: Invalid edit item type.")
            );
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Evaluate the application definition file to update depending on the edit item type. Parse
 * application defintion file to evaluate the line to be written and the position in application
 * definition file to write.
 **/
//--------------------------------------------------------------------------------------------------
void EvaluateAdefGetEditLinePosition
(
    ArgHandler_t& handler,
    model::System_t* systemPtr
)
{
    switch (handler.editItemType)
    {
        case ArgHandler_t::EditItemType_t::COMPONENT:
            GetAdefComponentEditLinePosition(handler, systemPtr);
            break;

        case ArgHandler_t::EditItemType_t::SANDBOXED:
            GetAdefSectionEditLinePosition(handler, "sandboxed");
            break;

        case ArgHandler_t::EditItemType_t::START:
            GetAdefSectionEditLinePosition(handler, "start");
            break;

        default:
            throw mk::Exception_t(
                    LE_I18N("Internal error: Invalid edit item type.")
            );
            break;
    }
}


} // namespace updateDefs

} // namespace cli
