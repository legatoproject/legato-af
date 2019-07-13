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
    std::string sourceFile,                 ///< Original definition file is the source file
    std::string tempWorkingFile,            ///< Temporary working file where the section is updated
    std::vector<ArgHandler_t::LinePosition_t>& writePos   ///< Vector containing line to write and
                                                          ///< the line position
)
{
    std::ifstream source(sourceFile);
    std::ofstream dest(tempWorkingFile);

    if (!source)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for input."), sourceFile)
        );
    }

    if (!dest)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for output."), tempWorkingFile)
        );
    }

    source.seekg(0, source.end);
    int totalLength = source.tellg();

    // Copy first part of sdef
    source.seekg(0, source.beg);

    int startPos = 0;
    for (auto& it : writePos)
    {
        int length = it.beforePos - startPos;
        std::unique_ptr<char> firstBuffer(new char[length]);

        source.read(firstBuffer.get(), length-1);

        if (!dest.is_open())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to open file '%s' for writing."), tempWorkingFile)
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

            // Write the line to the destination file.
            dest << it.lineToWrite << std::endl;
        }
        else
        {
           dest << "" <<std::endl;
        }
        startPos = it.afterPos;

        source.seekg(startPos);
    }

    // Copy the remaining part of sdef
    source.seekg(startPos);

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
            // Create directory if it does not exist.
            if (!file::DirectoryExists(handler.appSearchPath.front()))
            {
                file::MakeDir(handler.appSearchPath.front());
            }

            handler.absAdefFilePath = path::MakeCanonical(handler.appSearchPath.front()) + "/" +
                                      handler.adefFilePath;
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

            // When component is being created ...
            if (compFoundPath.empty() && !handler.compSearchPath.empty())
            {
                // Create directory if it does not exist.
                if (!file::DirectoryExists(handler.compSearchPath.front()))
                {
                    file::MakeDir(handler.compSearchPath.front());
                }

                handler.absCdefFilePath = path::MakeCanonical(handler.compSearchPath.front()) +
                                          "/" + handler.cdefFilePath;
            }
        }

        // Component is created when app is created
        if (handler.cdefFilePath.empty() && !handler.compSearchPath.empty())
        {
            // Create directory if it does not exist.
            if (!file::DirectoryExists(handler.compSearchPath.front()))
            {
                file::MakeDir(handler.compSearchPath.front());
            }

            handler.absCdefFilePath = path::MakeCanonical(handler.compSearchPath.front()) + "/"
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
            // Create directory if it does not exist.
            if (!file::DirectoryExists(handler.moduleSearchPath.front()))
            {
                file::MakeDir(handler.moduleSearchPath.front());
            }

            handler.absMdefFilePath = path::MakeCanonical(handler.moduleSearchPath.front()) + "/" +
                                      handler.mdefFilePath;
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

    if (handler.editActionType == ArgHandler_t::EditActionType_t::REMOVE)
    {
        lineToWrite = "";
        return lineToWrite;
    }

    switch (handler.editItemType)
    {
        case ArgHandler_t::EditItemType_t::APP:
            defFile = handler.absAdefFilePath;
            // If appSearch section is present, list just relative app name, no need to specify
            // the absolute path.
            for (const auto& it : handler.appSearchPath)
            {
                writePath = path::EraseCommonBasePath(handler.absAdefFilePath, it, false);
                if (!writePath.empty())
                {
                    break;
                }
            }
            break;

        case ArgHandler_t::EditItemType_t::MODULE:
            defFile = handler.absMdefFilePath;
            // If moduleSearch section is present, list just relative module name, no need to
            // specify full absolute path.
            for (const auto& it : handler.moduleSearchPath)
            {
                writePath = path::EraseCommonBasePath(handler.absMdefFilePath, it, false);
                if (!writePath.empty())
                {
                    break;
                }
            }
            break;

        default:
            throw mk::Exception_t(
                mk::format(LE_I18N("Internal error: '%s' edit item type is invalid"),
                           handler.editItemType)
            );
            break;
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
            default:
                  throw mk::Exception_t(
                            mk::format(LE_I18N("Internal: '%s' edit item type is invalid."),
                                       handler.editItemType)
                  );
                  break;
        }

        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{strWrite, endPos,
                                                                           endPos});
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the application definition file to update the components: or executables: section.
 * 1. Parse the cdef file to check if cdef contains .c files listed in sources: section.
 * 2. Parse the adef file for parsing.
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

    auto adefFilePtr = parser::adef::Parse(adefPath, false);

    // Position for the component in components: or executables: section.
    int foundPos1, nextPos1, length1 = 0;
    bool foundItem = false;

    // Position for the component in processes: run: section.
    int foundPos2, nextPos2, length2 = 0;
    bool foundItem2 = false;

    std::string exeName;

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

                for (auto tokenPtr : exePtr->Contents())
                {
                    // Resolve the path to the component.
                    std::string componentPath = path::Unquote(DoSubstitution(tokenPtr));
                    if (path::GetLastNode(compList).compare(path::GetLastNode(componentPath)) == 0)
                    {
                        foundItem = true;
                        foundPos1 = itemPtr->firstTokenPtr->curPos;
                        nextPos1 = itemPtr->lastTokenPtr->nextPtr->curPos;
                        exeName = itemPtr->firstTokenPtr->text;
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

                if (path::MakeAbsolute(compList).compare(componentPath) == 0)
                {
                    foundItem = true;
                    foundPos1 = tokenPtr->curPos;
                    nextPos1 = tokenPtr->nextPtr->curPos;
                }

                if (path::MakeAbsolute(compNotList).compare(componentPath) == 0)
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
                            if (exeName.compare(procName) == 0)
                            {
                                foundItem2 = true;
                                foundPos2 = itemPtr->firstTokenPtr->curPos;
                                nextPos2 = itemPtr->lastTokenPtr->nextPtr->curPos;
                            }
                        }
                    }
                    length2 = subsectionPtr->lastTokenPtr->curPos;
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
        compPath = path::EraseCommonBasePath(compNotList, handler.absAdefFilePath, true);
    }

    std::string lineToWrite1;
    std::string lineToWrite2;

    if (!compList.empty()  && !compNotList.empty() && foundItem && foundItem2)
    {
        if (sourcesSectionExist)
        {
            // Append the executable and component path to the adef in the executables: section
            lineToWrite1 = compName + "Exe = ( " +
                          compPath + " )";
            lineToWrite2 = "( " + compName + "Exe )";
        }
        else
        {
            // Append the component path to the adef in the components: section
             lineToWrite1 = "    " + compPath;
        }

        // Renaming components in adef
        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite1,
                                                                           foundPos1, nextPos1});
        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite2,
                                                                           foundPos2, nextPos2});

    }
    else if (!compList.empty() && compNotList.empty() && foundItem && foundItem2)
    {
        //Remove component
        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{"", foundPos1,
                                                                           nextPos1});
        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{"", foundPos2,
                                                                           nextPos2});

    }
    else if (!compList.empty() && !foundItem && !foundItem2)
    {
        // If the component to be renamed or removed is not found on the list, print error.
        throw mk::Exception_t(
               mk::format(LE_I18N(
                          "Component '%s' not listed in either components: or executables: section"
                           " of '%s'."), compList, adefPath)
        );
    }
    else if (!compNotList.empty())
    {
        if (sourcesSectionExist)
        {
            // Append the executable and component path to the adef in the executables: section
            lineToWrite1 = "    " + compName + "Exe = ( " +
                          compPath + " )";
            lineToWrite2 = "    ( " + compName + "Exe )";
        }
        else
        {
            // Append the component path to the adef in the components: section
             lineToWrite1 = "    " + compPath;
        }

        // Adding component to adef
        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite1, length1,
                                                                           length1-1});
        handler.linePositionToWrite.push_back(ArgHandler_t::LinePosition_t{lineToWrite2, length2,
                                                                           length2-1});
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Evaluate CDEF file path and component's relevant string and ts position to write in ADEF.
 **/
//--------------------------------------------------------------------------------------------------
void GetAdefComponentEditLinePosition
(
    ArgHandler_t& handler,
    model::System_t* systemPtr
)
{
    std::string absAdefFile;
    std::string absCdefFile = handler.absCdefFilePath + "/" + COMP_CDEF;
    std::string compMustExist;
    std::string compMustNotExist;

    switch(handler.editActionType)
    {
        case  ArgHandler_t::EditActionType_t::ADD:
        case  ArgHandler_t::EditActionType_t::CREATE:
            compMustExist = "";
            compMustNotExist = handler.absCdefFilePath;
            if (handler.adefFilePath.empty())
            {
                return;
            }
            else
            {
                absAdefFile = handler.absAdefFilePath;
            }
            break;

        case  ArgHandler_t::EditActionType_t::REMOVE:
        {
            compMustExist = handler.absCdefFilePath;
            compMustNotExist = "";

            if (handler.adefFilePath.empty())
            {
                for(const auto& it : systemPtr->apps)
                {
                    for (auto const& itcomp : it.second->components)
                    {
                        if (itcomp->defFilePtr->path.compare(absCdefFile) == 0)
                        {
                            absAdefFile = it.second->defFilePtr->path;
                        }
                    }
                }
                handler.absAdefFilePath = absAdefFile;
            }
            else
            {
                absAdefFile = handler.absAdefFilePath;
            }
            break;
        }

        case  ArgHandler_t::EditActionType_t::RENAME:
        {
            compMustExist = handler.oldCdefFilePath;
            compMustNotExist = handler.absCdefFilePath;

            std::string absOldCdefFile = handler.oldCdefFilePath + "/" + COMP_CDEF;
            absCdefFile = absOldCdefFile;

            if (handler.adefFilePath.empty())
            {
                for(const auto& it : systemPtr->apps)
                {
                    for (auto const& itcomp : it.second->components)
                    {
                        if (itcomp->defFilePtr->path.compare(absOldCdefFile) == 0)
                        {
                            absAdefFile = it.second->defFilePtr->path;
                        }
                    }
                }
                handler.absAdefFilePath = absAdefFile;
            }
            else
            {
                absAdefFile = handler.absAdefFilePath;
            }
            break;
        }

        default:
            throw mk::Exception_t(
                    LE_I18N("Internal error: Invalid edit action type.")
            );
            break;
    }

    // Parse the definition files to get the line and its position to write.
    ParseAdefGetEditLinePosition(handler, absAdefFile, absCdefFile, compMustExist,
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
