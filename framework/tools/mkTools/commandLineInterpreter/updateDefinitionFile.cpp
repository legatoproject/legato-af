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
 * 1. Create a temporary file where the appropriate section is updated.
 * 2. Copy the contents of the original definition file to the temporary file until 'firstPosition'.
 * 3. Update the temporary file with the line that need to be written.
 * 4. Copy the remaining contents of the original definition file from 'secondPosition' till the end
 *    of the file.
 * 5. After the temporary file is successfully updated, rename the temporary file to the original
 *    file name such that the orignal file is updated.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateDefinitionFile
(
    std::string sourceFile,
    int firstPosition,
    int secondPosition,
    std::string lineToWrite
)
{
    std::string destpath = "mkedit_temp.sdef";

    std::ifstream source(sourceFile, std::ios::binary);
    std::ofstream dest(destpath, std::ios::binary);

    if (!source)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for input."), sourceFile)
        );
    }

    if (!dest)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for output."), destpath)
        );
    }

    source.seekg(0, source.end);
    int totalLength = source.tellg();

    // Copy first part of sdef
    source.seekg(0, source.beg);

    char *firstBuffer = new char [firstPosition];

    source.read(firstBuffer, firstPosition-1);

    if (!dest.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), destpath)
        );
    }

    dest.write(firstBuffer, firstPosition-1);

    if (!lineToWrite.empty())
    {
        // Strip off '.adef' or '.mdef' optional suffix before writing to the definition file.
        if (path::HasSuffix(lineToWrite, ADEF_EXT))
        {
            lineToWrite = path::RemoveSuffix(lineToWrite, ADEF_EXT);
        }
        else if (path::HasSuffix(lineToWrite, MDEF_EXT))
        {
            lineToWrite = path::RemoveSuffix(lineToWrite, MDEF_EXT);
        }

        // Write the line to the destination file.
        dest << lineToWrite << std::endl;
    }
    else
    {
       dest << "" <<std::endl;
    }

    // Copy the second part of the sdef
    source.seekg(secondPosition);

    int remainLength = totalLength-secondPosition;

    char *secondBuffer = new char [remainLength];
    source.read(secondBuffer, remainLength);

    dest.write(secondBuffer, remainLength);

    source.close();
    dest.close();

    delete[] firstBuffer;
    delete[] secondBuffer;

    // After successfully updating the definition file, rename temporary destination file to
    // the original file.
    file::RenameFile(destpath, sourceFile);
}


//--------------------------------------------------------------------------------------------------
/**
 * Use this function if two lines need to be updated in the definition file.
 *
 * Update a given definition file with two lines of string to write to the definition file at two
 * given positions.
 * 1. Create a temporary file where the appropriate section is updated.
 * 2. Copy contents of the original definition file to the temporary file until 'firstPosition1'.
 * 3. Update the temporary file with the line that need to be written.
 * 4. Copy remaining contents of the original definition file from 'secondPosition2' until
 *    'firstPosition2'.
 * 5. Copy remaining contents of the original definition file from 'secondPosition2' till the end
 *    of the file.
 * 6. After the temporary file is successfully updated, rename the temporary file to the original
 *    file name such that the orignal file is updated.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateDefinitionFile
(
    std::string sourceFile,
    int firstPosition1,
    int secondPosition1,
    int firstPosition2,
    int secondPosition2,
    std::string lineToWrite1,
    std::string lineToWrite2
)
{
    std::string destpath = "mkedit_temp.sdef";

    std::ifstream source(sourceFile, std::ios::binary);
    std::ofstream dest(destpath, std::ios::binary);

    if (!source)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for input."), sourceFile)
        );
    }

    if (!dest)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for output."), destpath)
        );
    }

    source.seekg(0, source.end);
    int totalLength = source.tellg();

    // Copy first part of sdef
    source.seekg(0, source.beg);

    char *firstBuffer = new char [firstPosition1];

    source.read(firstBuffer, firstPosition1-1);

    if (!dest.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), destpath)
        );
    }

    dest.write(firstBuffer, firstPosition1-1);

    if (!lineToWrite1.empty())
    {
        // Strip off '.adef' or '.mdef' optional suffix before writing to the definition file.
        if (path::HasSuffix(lineToWrite1, ADEF_EXT))
        {
            lineToWrite1 = path::RemoveSuffix(lineToWrite1, ADEF_EXT);
        }
        else if (path::HasSuffix(lineToWrite1, MDEF_EXT))
        {
            lineToWrite1 = path::RemoveSuffix(lineToWrite1, MDEF_EXT);
        }

        // Write the line to the destination file.
        dest << lineToWrite1 << std::endl;
    }
    else
    {
       dest << "" <<std::endl;
    }

    // Copy the second part of the sdef
    source.seekg(secondPosition1);

    int tempLength = firstPosition2 - secondPosition1;
    char *tempBuffer = new char [tempLength];
    source.read(tempBuffer, tempLength-1);

    dest.write(tempBuffer, tempLength-1);

    if (!lineToWrite2.empty())
    {
        // Strip off '.adef' or '.mdef' optional suffix before writing to the definition file.
        if (path::HasSuffix(lineToWrite2, ADEF_EXT))
        {
            lineToWrite2 = path::RemoveSuffix(lineToWrite2, ADEF_EXT);
        }
        else if (path::HasSuffix(lineToWrite2, MDEF_EXT))
        {
            lineToWrite2 = path::RemoveSuffix(lineToWrite2, MDEF_EXT);
        }

        // Write the line to the destination file.
        dest << lineToWrite2 << std::endl << "    ";
    }
    else
    {
       dest << "" <<std::endl;
    }

    source.seekg(secondPosition2);

    int remainLength = totalLength-secondPosition2;

    char *secondBuffer = new char [remainLength];
    source.read(secondBuffer, remainLength);

    dest.write(secondBuffer, remainLength);

    source.close();
    dest.close();

    delete[] firstBuffer;
    delete[] tempBuffer;
    delete[] secondBuffer;

    // After successfully updating the definition file, rename temporary destination file to
    // the original file.
    file::RenameFile(destpath, sourceFile);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the search path directory list pointed to by sectionPtr and add to the searchPathList
 */
//--------------------------------------------------------------------------------------------------
static void ReadSearchDirs
(
    std::list<std::string>& searchPathList,   ///< And add the new search paths to this list.
    const parseTree::TokenList_t* sectionPtr  ///< From the search paths from this section.
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

    std::list<std::string> appSearchPathList;
    std::list<std::string> componentSearchPathList;
    std::list<std::string> moduleSearchPathList;

    for (auto sectionPtr : sdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "appSearch")
        {
             ReadSearchDirs(appSearchPathList, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "componentSearch")
        {
            ReadSearchDirs(componentSearchPathList, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "moduleSearch")
        {
            ReadSearchDirs(moduleSearchPathList, ToTokenListPtr(sectionPtr));
        }
    }

    // Use the search paths to get absolute paths for apps, components, modules.
    if (!handler.adefFilePath.empty() || !handler.cdefFilePath.empty())
    {
        for (auto const& it : appSearchPathList)
        {
            if (it.find(path::GetContainingDir(sdefPath)) != std::string::npos)
            {
                if (path::GetLastNode(it) == "apps")
                {
                    handler.absAdefFilePath = it + "/" + handler.adefFilePath;
                    if (!handler.oldAdefFilePath.empty())
                    {
                        handler.oldAdefFilePath = it + "/" + handler.oldAdefFilePath;
                    }
                    handler.isAppSearchPath = true;
                    break;
                }
            }
        }

        for (auto const& it : componentSearchPathList)
        {
            if (it.find(path::GetContainingDir(sdefPath)) != std::string::npos)
            {
                if (path::GetLastNode(it) == "components")
                {
                    if (handler.cdefFilePath.empty())
                    {
                        handler.absCdefFilePath = it + "/"
                                               + path::GetLastNode(path::RemoveSuffix(
                                                                    handler.adefFilePath, ADEF_EXT))
                                               + "Component";
                    }
                    else
                    {
                        handler.absCdefFilePath = it + "/" + handler.cdefFilePath;
                    }

                    if (!handler.oldCdefFilePath.empty())
                    {
                        handler.oldCdefFilePath = it + "/" + handler.oldCdefFilePath;
                    }

                    handler.isCompSearchPath = true;
                    break;
                }
            }
        }
    }

    if (!handler.mdefFilePath.empty())
    {
        for (auto const& it : moduleSearchPathList)
        {
            if (it.find(path::GetContainingDir(sdefPath)) != std::string::npos)
            {
                if (path::GetLastNode(it) == "modules")
                {
                    handler.absMdefFilePath = it + "/" + handler.mdefFilePath;

                    if (!handler.oldMdefFilePath.empty())
                    {
                        handler.oldMdefFilePath = it + "/" + handler.oldMdefFilePath;
                    }

                    handler.isModSearchPath = true;
                    break;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the system definition file to update the apps: section.
 **/
//--------------------------------------------------------------------------------------------------
static void ParseSdefUpdateApp
(
    std::string sdefPath,
    std::string appList,
    std::string appNotList,
    std::string lineToWrite
)
{
    // Parse the sdef file and look for the apps: section to update
    const auto sdefFilePtr = parser::sdef::Parse(sdefPath, false);

    bool foundSection = false;

    for (auto sectionPtr : sdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "apps")
        {
            // There can be multiple files with apps: section included in the active sdef.
            // Need to make sure only the active sdef is looked into for updating apps.
            std::size_t found = sectionPtr->lastTokenPtr->GetLocation().find(sdefFilePtr->path);

            if (found != std::string::npos)
            {
                foundSection = true;
                bool foundItem = false;
                int foundPos, nextPos = 0;
                int length = sectionPtr->lastTokenPtr->curPos;

                auto appsSectionPtr =
                            dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);

                for (auto itemPtr : appsSectionPtr->Contents())
                {
                    const parseTree::App_t* appPtr = dynamic_cast<const parseTree::App_t*>(itemPtr);
                    const auto appSpec = path::Unquote(DoSubstitution(appPtr->firstTokenPtr));
                    std::string appSpecStrip, appListStrip, appNotListStrip;

                    appSpecStrip = path::GetLastNode(appSpec);
                    appListStrip = path::GetLastNode(appList);

                    if (path::HasSuffix(appSpecStrip, ADEF_EXT))
                    {
                        appSpecStrip = path::RemoveSuffix(appSpecStrip, ADEF_EXT);
                    }

                    if (path::HasSuffix(appListStrip, ADEF_EXT))
                    {
                        appListStrip = path::RemoveSuffix(appListStrip, ADEF_EXT);
                    }

                    if (path::HasSuffix(appNotList, ADEF_EXT))
                    {
                        appNotListStrip = path::RemoveSuffix(appNotList, ADEF_EXT);
                    }

                    std::size_t appListFound = appSpecStrip.find(appListStrip);
                    if (appListFound != std::string::npos)
                    {
                        foundItem = true;
                        foundPos = itemPtr->lastTokenPtr->curPos;
                        nextPos = itemPtr->firstTokenPtr->nextPtr->curPos;
                    }

                    if (!appNotList.empty())
                    {
                        std::size_t appNotListFound = appSpecStrip.find(appNotListStrip);
                        if (appNotListFound != std::string::npos)
                        {
                            throw mk::Exception_t(
                                   mk::format(LE_I18N("App already listed: '%s'"),
                                              itemPtr->lastTokenPtr->GetLocation())
                            );
                        }
                    }
                }

                if (!appList.empty() && foundItem)
                {
                    // Rename or remove app from sdef
                    UpdateDefinitionFile(sdefPath, foundPos, nextPos, lineToWrite);
                }
                else if (!appList.empty() && !foundItem)
                {
                    // If the app to be renamed or removed is not found on the list, print error.
                    throw mk::Exception_t(
                           mk::format( LE_I18N("App '%s' not listed in apps: section."), appList)
                    );
                }
                else if (!appNotList.empty())
                {
                    // Add app to sdef
                    UpdateDefinitionFile(sdefPath, length, length-1, lineToWrite);
                }
            } //end of if
        } // end of if
    } // end of for


    // If the section is not found then add new section and append to the end of the sdef file
    if (!foundSection && !appNotList.empty() && appList.empty())
    {
        std::string destpath = "mkedit_temp.sdef";

        std::ifstream source(sdefPath, std::ios::binary);
        std::ofstream dest(destpath, std::ios::binary);

        dest << source.rdbuf();

        std::fstream dest1;
        dest1.open(destpath, std::ios::in | std::ios::out | std::ios::ate);

        if (!dest1.is_open())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to open file '%s' for writing."), destpath)
            );
        }

        std::string strWrite;

        strWrite = "\napps:\n{\n" + lineToWrite + "\n}\n";

        dest1.write(strWrite.c_str(), strWrite.size());
        dest1.close();

        source.close();
        dest.close();

        file::RenameFile(destpath, sdefPath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the system definition file to update the kernelModule(s): section.
 **/
//--------------------------------------------------------------------------------------------------
static void ParseSdefUpdateKernelModules
(
    std::string sdefPath,
    std::string moduleList,
    std::string moduleNotList,
    std::string lineToWrite
)
{
    bool foundSection = false;
    bool foundItem = false;

    // Parse the sdef file and look for the kernelModule(s): section to update
    const auto sdefFilePtr = parser::sdef::Parse(sdefPath, false);

    for (auto sectionPtr : sdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (parser::IsNameSingularPlural(sectionName, "kernelModule"))
        {
            // There can be multiple files with kernelModules: section included with active sdef.
            // Need to make sure only the active sdef is looked into for updating apps.
            std::size_t found = sectionPtr->lastTokenPtr->GetLocation().find(sdefFilePtr->path);

            int length = sectionPtr->lastTokenPtr->curPos;

            if (found != std::string::npos)
            {
                int foundPos, nextPos = 0;

                foundSection = true;
                auto moduleSectionPtr =
                            dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);
                for (auto itemPtr : moduleSectionPtr->Contents())
                {
                    const parseTree::RequiredModule_t* modulePtr =
                                        dynamic_cast<const parseTree::RequiredModule_t*>(itemPtr);
                    const auto moduleSpec = path::Unquote(DoSubstitution(modulePtr->firstTokenPtr));

                    std::string moduleSpecStrip, moduleListStrip, moduleNotListStrip;

                    moduleSpecStrip = path::GetLastNode(moduleSpec);
                    moduleListStrip = path::GetLastNode(moduleList);

                    if (path::HasSuffix(moduleSpecStrip, MDEF_EXT))
                    {
                        moduleSpecStrip = path::RemoveSuffix(moduleSpecStrip, MDEF_EXT);
                    }

                    if (path::HasSuffix(moduleListStrip, MDEF_EXT))
                    {
                        moduleListStrip = path::RemoveSuffix(moduleListStrip, MDEF_EXT);
                    }

                    if (path::HasSuffix(moduleNotList, MDEF_EXT))
                    {
                        moduleNotListStrip = path::RemoveSuffix(moduleNotList, MDEF_EXT);
                    }

                    std::size_t moduleListFound = moduleSpecStrip.find(moduleListStrip);
                    if (moduleListFound != std::string::npos)
                    {
                        foundItem = true;
                        foundPos = itemPtr->lastTokenPtr->curPos;
                        nextPos = itemPtr->firstTokenPtr->nextPtr->curPos;
                    }

                    if (!moduleNotList.empty())
                    {
                        std::size_t moduleNotListFound = moduleSpecStrip.find(moduleNotListStrip);
                        if (moduleNotListFound != std::string::npos)
                        {
                            throw mk::Exception_t(
                                   mk::format(LE_I18N("Module already listed: '%s'"),
                                              itemPtr->lastTokenPtr->GetLocation())
                            );
                        }
                    }
               }

                if (!moduleList.empty() && foundItem)
                {
                    // Renaming module in sdef
                    UpdateDefinitionFile(sdefPath, foundPos, nextPos, lineToWrite);
                }
                else if (!moduleList.empty() && !foundItem)
                {
                    // If the app to be renamed or removed is not found on the list, print error.
                    throw mk::Exception_t(
                          mk::format(LE_I18N("Module '%s' not listed in kernelModule(s): section."),
                                     moduleList)
                    );
                }
                else if (!moduleNotList.empty())
                {
                    // Adding module to sdef
                    UpdateDefinitionFile(sdefPath, length, length-1, lineToWrite);
                }
            } // end of if
        } // end of if
    } // end of for

    // If a module is being added to the sdef but the section is not found, then create a new
    // section at the end of the sdef file.
    if (!foundSection && !moduleNotList.empty() && moduleList.empty())
    {
        std::string destpath = "mkedit_temp.sdef";

        std::ifstream source(sdefPath, std::ios::binary);
        std::ofstream dest(destpath, std::ios::binary);

        dest << source.rdbuf();

        std::fstream dest1;
        dest1.open(destpath, std::ios::in | std::ios::out | std::ios::ate);

        if (!dest1.is_open())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to open file '%s' for writing."), destpath)
            );
        }

        std::string strWrite;

        strWrite = "\nkernelModules:\n{\n" + lineToWrite + "\n}\n";

        dest1.write(strWrite.c_str(), strWrite.size());
        dest1.close();

        source.close();
        dest.close();

        file::RenameFile(destpath, sdefPath);
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
static void ParseAdefUpdateComponent
(
    std::string adefPath,
    std::string cdefPath,
    std::string compList,
    std::string compNotList,
    bool isCompSearchPath
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

            for (auto itemPtr : exeSectionPtr->Contents())
            {
                const parseTree::Executable_t* exePtr =
                                    dynamic_cast<const parseTree::Executable_t*>(itemPtr);

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

            for (auto subsectionPtr : processesSectionPtr->Contents())
            {
                auto& subsectionName = subsectionPtr->firstTokenPtr->text;

                if (subsectionName == "run")
                {
                    auto runSectionPtr =
                            dynamic_cast<const parseTree::CompoundItemList_t*>(subsectionPtr);

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

    compPath = compNotList;
    compName = path::GetLastNode(compPath);

    // If componentSearch section is present, list component name (no need to specify full path)
    if (isCompSearchPath)
    {
        compPath = compName;
    }
    else
    {
        std::string adefContainDir = path::GetContainingDir(adefPath);

        std::size_t foundAdefContainDir = compPath.find(adefContainDir);
        if (foundAdefContainDir != std::string::npos)
        {
            compPath.erase(foundAdefContainDir, adefContainDir.length()+1);
        }
    }

    std::string lineToWrite;
    std::string lineToWrite2;

    if (!compList.empty()  && !compNotList.empty() && foundItem && foundItem2)
    {
        if (sourcesSectionExist)
        {
            // Append the executable and component path to the adef in the executables: section
            lineToWrite = compName + "Exe = ( " +
                          compPath + " )";
            lineToWrite2 = "( " + compName + "Exe )";
        }
        else
        {
            // Append the component path to the adef in the components: section
             lineToWrite = "    " + compPath;
        }

        // Renaming components in adef
        UpdateDefinitionFile(adefPath, foundPos1, nextPos1, foundPos2, nextPos2, lineToWrite,
                             lineToWrite2);

    }
    else if (!compList.empty() && compNotList.empty() && foundItem && foundItem2)
    {
        //Remove component
        UpdateDefinitionFile(adefPath, foundPos1, nextPos1, foundPos2, nextPos2, "", "");
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
            lineToWrite = "    " + compName + "Exe = ( " +
                          compPath + " )";
            lineToWrite2 = "    ( " + compName + "Exe )";
        }
        else
        {
            // Append the component path to the adef in the components: section
             lineToWrite = "    " + compPath;
        }

        // Adding component to adef
        UpdateDefinitionFile(adefPath, length1, length1-1, length2, length2-1, lineToWrite,
                             lineToWrite2);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the system definition file (sdef) to add/rename/remove an application.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateSdefApp
(
    ArgHandler_t& handler
)
{
    std::string srcpath;
    std::string fullAppPath;

    if (handler.sdefFilePath.empty())
    {
        return;
    }
    else
    {
        srcpath = path::MakeAbsolute(handler.sdefFilePath);
        if (!path::IsAbsolute(handler.adefFilePath))
        {
            fullAppPath = handler.absAdefFilePath;
        }
        else
        {
            fullAppPath = handler.adefFilePath;
        }

        if (handler.isAppSearchPath)
        {
            // If appSearch section is present, list just app name, no need to specify full path.
            fullAppPath = path::GetLastNode(handler.adefFilePath);
        }
        else
        {
            std::string sdefContainDir = path::GetContainingDir(
                                                    path::MakeAbsolute(handler.sdefFilePath)
                                                );

            std::size_t foundsdefContainDir = (handler.absAdefFilePath).find(sdefContainDir);
            if (foundsdefContainDir != std::string::npos)
            {
                fullAppPath.erase(foundsdefContainDir, sdefContainDir.length()+1);
            }
        }
    }

    std::string lineToWrite = "    " + fullAppPath;
    ParseSdefUpdateApp(srcpath, "", handler.absAdefFilePath, lineToWrite);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the system definition file (sdef) to add/rename/remove the kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateSdefModule
(
    ArgHandler_t& handler
)
{
    std::string srcpath;
    std::string fullModulePath;

    if (handler.sdefFilePath.empty())
    {
        return;
    }
    else
    {
        srcpath = path::MakeAbsolute(handler.sdefFilePath);

        if (!path::IsAbsolute(handler.mdefFilePath))
        {
            fullModulePath = handler.absMdefFilePath;
        }
        else
        {
            fullModulePath = handler.mdefFilePath;
        }

        if (handler.isModSearchPath)
        {
            // If moduleSearch section is present, list module name, no need to specify full path.
            fullModulePath = path::GetLastNode(handler.mdefFilePath);
        }
        else
        {
            std::string sdefContainDir = path::GetContainingDir(
                                                    path::MakeAbsolute(handler.sdefFilePath)
                                                );

            std::size_t foundsdefContainDir = (handler.absMdefFilePath).find(sdefContainDir);
            if (foundsdefContainDir != std::string::npos)
            {
                fullModulePath.erase(foundsdefContainDir, sdefContainDir.length()+1);
            }
        }
    }

    std::string lineToWrite = "    " + fullModulePath;
    ParseSdefUpdateKernelModules(srcpath, "", handler.absMdefFilePath, lineToWrite);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the application definition file (adef) by updating component.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateAdefComponent
(
    ArgHandler_t& handler
)
{
    if (handler.adefFilePath.empty())
    {
        if (path::HasSuffix(envVars::Get("LEGATO_DEF_FILE"), ADEF_EXT))
        {
            handler.adefFilePath = envVars::Get("LEGATO_DEF_FILE");
        }
        else
        {
            return;
        }
    }

    std::string absCdefFile = handler.absCdefFilePath + "/"+ COMP_CDEF;
    ParseAdefUpdateComponent(handler.absAdefFilePath, absCdefFile, "", handler.absCdefFilePath,
                             handler.isCompSearchPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the system definition file (sdef) by removing application.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateSdefRemoveApp
(
    ArgHandler_t handler
)
{
    std::string srcpath;

    if (handler.sdefFilePath.empty())
    {
        return;
    }
    else
    {
       srcpath = path::MakeAbsolute(handler.sdefFilePath);
    }

    ParseSdefUpdateApp(srcpath, handler.adefFilePath, "", "");
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the system definition file (sdef) by removing the kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateSdefRemoveModule
(
    ArgHandler_t& handler
)
{
    std::string srcpath;

    if (handler.sdefFilePath.empty())
    {
        return;
    }
    else
    {
       srcpath = path::MakeAbsolute(handler.sdefFilePath);
    }

    ParseSdefUpdateKernelModules(srcpath, handler.absMdefFilePath, "", "");
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the application definition file (adef) by removing the component.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateAdefRemoveComponent
(
    ArgHandler_t& handler
)
{
    if (handler.adefFilePath.empty())
    {
        return;
    }

    std::string absCdefFile = handler.absCdefFilePath + "/" + COMP_CDEF;
    ParseAdefUpdateComponent(handler.absAdefFilePath, absCdefFile, handler.absCdefFilePath, "",
                             handler.isCompSearchPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the system definition file (sdef) by updating the apps section.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateSdefRenameApp
(
    ArgHandler_t handler
)
{
    std::string srcpath;
    std::string fullAppPath;

    if (handler.sdefFilePath.empty())
    {
        return;
    }
    else
    {
        srcpath = path::MakeAbsolute(handler.sdefFilePath);

        if (!path::IsAbsolute(handler.adefFilePath))
        {
            fullAppPath = handler.absAdefFilePath;
        }
        else
        {
            fullAppPath = handler.adefFilePath;
        }

        if (handler.isAppSearchPath)
        {
            // If appSearch section is present, list just app name, no need to specify full path.
            fullAppPath = path::GetLastNode(handler.adefFilePath);
        }
        else
        {
            std::string sdefContainDir = path::GetContainingDir(
                                                    path::MakeAbsolute(handler.sdefFilePath)
                                                );

            std::size_t foundsdefContainDir = (handler.absAdefFilePath).find(sdefContainDir);
            if (foundsdefContainDir != std::string::npos)
            {
                fullAppPath.erase(foundsdefContainDir, sdefContainDir.length()+1);
            }
        }
    }

    ParseSdefUpdateApp(srcpath, handler.oldAdefFilePath, handler.absAdefFilePath, fullAppPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the system definition file (sdef) by renaming the kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateSdefRenameModule
(
    ArgHandler_t& handler
)
{
    std::string srcpath;
    std::string fullModulePath;

    if (handler.sdefFilePath.empty())
    {
        return;
    }
    else
    {
        srcpath = path::MakeAbsolute(handler.sdefFilePath);
        if (!path::IsAbsolute(handler.mdefFilePath))
        {
            fullModulePath = handler.absMdefFilePath;
        }
        else
        {
            fullModulePath = handler.mdefFilePath;
        }

        if (handler.isModSearchPath)
        {
            // If moduleSearch section is present, list just module name, no need to specify full path.
            fullModulePath = path::GetLastNode(handler.mdefFilePath);
        }
        else
        {
            std::string sdefContainDir = path::GetContainingDir(
                                                    path::MakeAbsolute(handler.sdefFilePath)
                                                );

            std::size_t foundsdefContainDir = (handler.absMdefFilePath).find(sdefContainDir);
            if (foundsdefContainDir != std::string::npos)
            {
                fullModulePath.erase(foundsdefContainDir, sdefContainDir.length()+1);
            }
        }
    }

    ParseSdefUpdateKernelModules(srcpath, handler.oldMdefFilePath, handler.absMdefFilePath,
                                 fullModulePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update application definition file (adef) by renaming the component.
 **/
//--------------------------------------------------------------------------------------------------
void UpdateAdefRenameComponent
(
    ArgHandler_t& handler,
    model::System_t* systemPtr
)
{
    std::string absAdefFile;
    std::string absCdefFile = handler.absCdefFilePath + "/" + COMP_CDEF;
    std::string absOldCdefFile = handler.oldCdefFilePath + "/" + COMP_CDEF;

    if (handler.adefFilePath.empty())
    {
        for(const auto& it : systemPtr->apps)
        {
            absAdefFile = it.second->defFilePtr->path;
            for (auto const& itcomp : it.second->components)
            {
                if (itcomp->defFilePtr->path.compare(absOldCdefFile) == 0)
                {
                    ParseAdefUpdateComponent(absAdefFile, absCdefFile, handler.oldCdefFilePath,
                                             handler.cdefFilePath, handler.isCompSearchPath);
                }
            }
        }
    }
    else
    {
        ParseAdefUpdateComponent(handler.absAdefFilePath, absCdefFile, handler.oldCdefFilePath,
                                 handler.absCdefFilePath, handler.isCompSearchPath);
    }
}

} // namespace updateDefs

} // namespace cli
