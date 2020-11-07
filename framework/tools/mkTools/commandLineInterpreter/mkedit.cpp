//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkedit" functionality of the "mk" tool.
 *
 *  Run 'mkedit --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <sys/sendfile.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{
// Object of argument handler for mkedit
static ArgHandler_t Handler;


//--------------------------------------------------------------------------------------------------
/**
 * Definition file name must start only with a letter or an underscore. The file name may only
 * contain letters, numbers and underscores.
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateFileName
(
    std::string fileName
)
{
    auto const fileExt = path::GetFileNameExtension(fileName);

    fileName = path::RemoveSuffix(fileName, fileExt);

    char c = fileName[0];

    if (islower(c) || isupper(c) || (c == '_'))
    {
        int length = fileName.length();
        int index;
        for (index = 1; index < length; index++)
        {
            c = fileName[index];

            if (!islower(c) && !isupper(c) && !isdigit(c) && (c != '_'))
            {
                 throw mk::Exception_t(mk::format(
                                             LE_I18N("Unexpected character '%c'.  "
                                                     "Names may only contain letters"
                                                     " ('a'-'z' or 'A'-'Z'),"
                                                     " numbers ('0'-'9')"
                                                     " and underscores ('_')."), c));
            }
        }
    }
    else
    {
        throw mk::Exception_t(mk::format(LE_I18N("Unexpected character '%c'"
                                                             " at beginning of name."
                                                             " Names must start"
                                                             " with a letter ('a'-'z' or 'A'-'Z')"
                                                             " or an underscore ('_')."), c));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the application definition file (adef) extension. Append ".adef" if missing.
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateAdefExtension
(
    std::string& filePath
)
{
    if (filePath.empty())
    {
        return;
    }

    auto const fileExt = path::GetFileNameExtension(filePath);

    if (fileExt.empty())
    {
        filePath += ADEF_EXT;
    }
    else if (fileExt.compare(ADEF_EXT) != 0)
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("Application definition file must have '%s' extension"), ADEF_EXT)
        );
    }

    ValidateFileName(path::GetLastNode(filePath));
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the module definition file (mdef). Append ".mdef" if missing
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateMdefExtension
(
    std::string& filePath
)
{
    if (filePath.empty())
    {
        return;
    }

    auto const fileExt = path::GetFileNameExtension(filePath);

    if (fileExt.empty())
    {
        filePath += MDEF_EXT;
    }
    else if (fileExt.compare(MDEF_EXT) != 0)
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("Module definition file must have '%s' extension"), MDEF_EXT)
        );
    }

    ValidateFileName(path::GetLastNode(filePath));
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the system definition file (sdef). Append ".sdef" if missing.
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateSdefExtension
(
    std::string& filePath
)
{
    if (filePath.empty())
    {
        return;
    }

    auto const fileExt = path::GetFileNameExtension(filePath);

    if (fileExt.empty())
    {
        filePath += SDEF_EXT;
    }
    else if (fileExt.compare(SDEF_EXT) != 0)
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("System definition file must have '%s' extension"), SDEF_EXT)
        );
    }

    ValidateFileName(path::GetLastNode(filePath));
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the app start value.
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateAppStartValue(std::string& appStart)
{
    if ((appStart != "auto") && (appStart != "manual"))
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("'%s' start value is not supported. Set to 'auto' or 'manual'."),
                          appStart)
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the app sandboxed value.
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateAppSandboxedValue(std::string& appSandboxed)
{
    if ((appSandboxed != "true") && (appSandboxed != "false"))
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("'%s' sandboxed value is not supported. Set to 'true' or"
                                  "'false'."), appSandboxed)
        );
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Checking the application definition adef file is able to be updated for the remove/rename
 * component in the components: or executables: section of this adef file.
 **/
//-------------------------------------------------------------------------------------------------
bool CheckAdefForComponentUpdating
(
    ArgHandler_t& handler,
    const std::string& adefPath,
    const std::string& cdefPath
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

    const std::string compList = path::GetContainingDir(cdefPath);

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nSearching component '%s' in ADEF file '%s'."),
                                        compList, adefPath);
    }

    auto adefFilePtr = parser::adef::Parse(adefPath, false);

    // Boolean flag to check if the component is found in the components: or executables: section.
    bool foundItem = false;

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
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                               sectionName)
                );
            }

            for (auto itemPtr : exeSectionPtr->Contents())
            {
                const parseTree::Executable_t* exePtr = dynamic_cast
                                                         <const parseTree::Executable_t*>(itemPtr);
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

                        if (handler.isPrintLogging())
                        {
                            std::cout << mk::format(LE_I18N("\nComponent '%s' found in '%s' "
                                                            "section '%s'"),
                                                    path::GetLastNode(compList), sectionName,
                                                    tokenPtr->GetLocation()
                            );
                        }
                    }
                }
            }
        }

        if (!sourcesSectionExist && (sectionName == "components"))
        {
            auto componentSectionPtr =
                        dynamic_cast<const parseTree::TokenListSection_t*>(sectionPtr);
            if (componentSectionPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"),
                               sectionName)
                );
            }

            for (auto tokenPtr : componentSectionPtr->Contents())
            {
                // Resolve the path to the component.
                std::string componentPath = path::Unquote(DoSubstitution(tokenPtr));

                if (path::GetLastNode(compList).compare(path::GetLastNode(componentPath)) == 0)
                {
                    foundItem = true;
                    if (handler.isPrintLogging())
                    {
                        std::cout << mk::format(LE_I18N("\nComponent '%s' found in '%s' "
                                                        "section at '%s'"),
                                                path::GetLastNode(compList), sectionName,
                                                tokenPtr->GetLocation()
                        );
                    }
                }
            }
        }
    } // end of for

    if (foundItem)
    {
        return true;
    }
    else
    {
        return false;
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Checking the subsection have the remove/rename component as its element.
 **/
//-------------------------------------------------------------------------------------------------
std::string EvaluateSubSection
(
    const parseTree::CompoundItemList_t *subsectionPtr,
    const std::string& sectionName,
    const std::string& subsectionName,
    model::Component_t *itcomp,
    const std::string& compList
)
{
    std::string collectComponent;

    for (auto itSecton = subsectionPtr->Contents().rbegin();
        itSecton != subsectionPtr->Contents().rend();
        ++itSecton)
    {
        auto compSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(subsectionPtr);

        if (compSectionPtr == NULL)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Internal error: '%s' section pointer is NULL"), subsectionName)
            );
        }

        for (auto itemPtr : compSectionPtr->Contents())
        {
            const parseTree::RequiredComponent_t* compPtr = dynamic_cast
                                                <const parseTree::RequiredComponent_t*>(itemPtr);
            if (compPtr == NULL)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: '%s' section content pointer is NULL"),
                               sectionName)
                );
            }

            for (auto tokenPtr : compPtr->Contents())
            {
                // If component to rename/remove is found
                // in the list of components.
                if (path::GetLastNode(compList).compare(path::GetLastNode(tokenPtr->text)) == 0)
                {
                    collectComponent = itcomp->defFilePtr->path;
                    goto stop_in_sub_section;
                }
            }
        }
    }

stop_in_sub_section:
    return collectComponent;
}


//-------------------------------------------------------------------------------------------------
/**
 * Evaluate the subcomponent of the checking component.
 * 1. Checking this component have the remove/rename component as its subcomponent.
 * 2. If this component have the component: section, evaluate this component: subsection.
 **/
//-------------------------------------------------------------------------------------------------
std::string EvaluateSubComponent
(
    model::Component_t *subComponentPtr,
    model::Component_t *itcomp,
    const std::string& compList,
    const std::string& cdefPath
)
{
    std::string collectComponent;
    // If this component have the remove/rename component as its subcomponent
    if (subComponentPtr->defFilePtr->path.compare(cdefPath) == 0)
    {
        // Iterate over the .cdef file's list of sections.
        for (auto sectionPtr : itcomp->defFilePtr->sections)
        {
            const std::string& sectionName = sectionPtr->firstTokenPtr->text;

            if (sectionName == "requires")
            {
                // "requires:" section is comprised of subsections
                for (auto memberPtr : static_cast<const parseTree::ComplexSection_t*>
                                                                        (sectionPtr)->Contents())
                {
                    const std::string& subsectionName = memberPtr->firstTokenPtr->text;

                    if (subsectionName == "component")
                    {
                        auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
                        collectComponent = EvaluateSubSection(subsectionPtr, sectionName,
                                                              subsectionName, itcomp, compList);
                        goto stop_in_sub_component;
                    }
                }
            }
        }
    }

stop_in_sub_component:
    return collectComponent;
}


//-------------------------------------------------------------------------------------------------
/**
 * Find all components that belonging to the adefPath application and depending on the cdefPath
 * component.
 **/
//-------------------------------------------------------------------------------------------------
std::vector<std::string> CollectDependingComponents
(
    ArgHandler_t& handler,
    const std::string& adefPath,
    const std::string& cdefPath
)
{
    std::vector<std::string> collectComponents;
    std::string collectComponent;
    const std::string compList = path::GetContainingDir(cdefPath);

    // Iterate over the list of applications relating to this system.
    for (const auto& it : handler.systemPtr->apps)
    {
        if (it.second->defFilePtr->path == adefPath)
        {
            // Iterate over the list of components relating to this application.
            for (auto const& itcomp : it.second->components)
            {
                // Iterate over the list of subcomponents relating to this component.
                for (auto subComponent : itcomp->subComponents)
                {
                    auto subComponentPtr = model::Component_t::GetComponent(
                                                                   subComponent.componentPtr->dir);
                    if (subComponentPtr != NULL)
                    {
                        collectComponent = EvaluateSubComponent(subComponentPtr, itcomp, compList,
                                                                cdefPath);
                        collectComponents.push_back(collectComponent);
                    }
                }
            }
        }
    }
    return collectComponents;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print logging based on the build parameter gathered from the command line.
 **/
//--------------------------------------------------------------------------------------------------
bool ArgHandler_t::isPrintLogging()
{
    // Print logging if either beVerbose or isDryRun is true.
    if (buildParams.beVerbose || buildParams.isDryRun)
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get corresponding file path based on the type of edit item.
 **/
//--------------------------------------------------------------------------------------------------
std::string ArgHandler_t::GetFileForEditItemType()
{
    std::string filePath;

    switch (editItemType)
    {
        case ArgHandler_t::EditItemType_t::APP:
            filePath = absAdefFilePath;
            break;

        case ArgHandler_t::EditItemType_t::COMPONENT:
            filePath = absCdefFilePath;
            break;

        case ArgHandler_t::EditItemType_t::MODULE:
            filePath = absMdefFilePath;
            break;

        case ArgHandler_t::EditItemType_t::SYSTEM:
            filePath = absSdefFilePath;
            break;

        default:
            throw mk::Exception_t(LE_I18N("Internal error: Invalid edit item type."));
            break;
    }

    return filePath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get corresponding old file path based on the type of edit item.
 **/
//--------------------------------------------------------------------------------------------------
std::string ArgHandler_t::GetOldFileForEditItemType()
{
    std::string oldFilePath;

    switch (editItemType)
    {
        case ArgHandler_t::EditItemType_t::APP:
            oldFilePath = oldAdefFilePath;
            break;

        case ArgHandler_t::EditItemType_t::COMPONENT:
            oldFilePath = oldCdefFilePath;
            break;

        case ArgHandler_t::EditItemType_t::MODULE:
            oldFilePath = oldMdefFilePath;
            break;

        case ArgHandler_t::EditItemType_t::SYSTEM:
            oldFilePath = oldSdefFilePath;
            break;

        default:
            throw mk::Exception_t(LE_I18N("Internal error: Invalid edit item type."));
            break;
    }

    return oldFilePath;
}


//--------------------------------------------------------------------------------------------------
/**
 * For rename action, set definition file path variable based on the edit item type.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::ActionRenameSetDefFilePath(const char* arg)
{
    switch(editItemType)
    {
        case APP:
            if (oldAdefFilePath.empty())
            {
                oldAdefFilePath = arg;
                commandLineNextArgType = EDIT_ITEM_VALUE;
            }
            else
            {
                adefFilePath = arg;
                commandLineNextArgType = NONEDIT_ITEM_KEY;
            }
            break;

        case COMPONENT:
            if (oldCdefFilePath.empty())
            {
                oldCdefFilePath = arg;
                commandLineNextArgType = EDIT_ITEM_VALUE;
            }
            else
            {
                cdefFilePath = arg;
                commandLineNextArgType = NONEDIT_ITEM_KEY;
            }
            break;

        case MODULE:
            if (oldMdefFilePath.empty())
            {
                oldMdefFilePath = arg;
                commandLineNextArgType = EDIT_ITEM_VALUE;
            }
            else
            {
                mdefFilePath = arg;
                commandLineNextArgType = NONEDIT_ITEM_KEY;
            }
            break;

        case SYSTEM:
            if (oldSdefFilePath.empty())
            {
                oldSdefFilePath = arg;
                commandLineNextArgType = EDIT_ITEM_VALUE;
            }
            else
            {
                sdefFilePath = arg;
                commandLineNextArgType = NONEDIT_ITEM_KEY;
            }
            break;

        default:
            throw mk::Exception_t(
                mk::format(LE_I18N("'%s' is invalid target command."), arg)
            );
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * For actions other than rename, set the definition file path variable based on edit item type.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::ActionNotRenameSetDefFilePath(const char* arg)
{
    switch(editItemType)
    {
        case APP:
            adefFilePath = arg;
            commandLineNextArgType = NONEDIT_ITEM_KEY;
            break;

        case COMPONENT:
            cdefFilePath = arg;
            commandLineNextArgType = NONEDIT_ITEM_KEY;
            break;

        case MODULE:
            mdefFilePath = arg;
            commandLineNextArgType = NONEDIT_ITEM_KEY;
            break;

        case SYSTEM:
            sdefFilePath = arg;
            commandLineNextArgType = EDIT_COMPLETE;
            break;

        case APPSEARCH:
        case COMPONENTSEARCH:
        case MODULESEARCH:
        case INTERFACESEARCH:
            searchPath = arg;
            commandLineNextArgType = NONEDIT_ITEM_KEY;
            break;

        case SANDBOXED:
            appSandboxed = arg;
            ValidateAppSandboxedValue(appSandboxed);
            commandLineNextArgType = NONEDIT_ITEM_KEY;
            break;

        case START:
            appStart = arg;
            ValidateAppStartValue(appStart);
            commandLineNextArgType = NONEDIT_ITEM_KEY;
            break;

        default:
            throw mk::Exception_t(
                mk::format(LE_I18N("'%s' is invalid target command."), arg)
            );
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Evaluate the next command line argument type based on the edit item type.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::EvaluateCommandLineNextArgType(const char* arg)
{
    switch (editItemType)
    {
        case APP:
            if (strcmp(arg,"component") == 0)
            {
                commandLineNextArgType = NONEDIT_COMP_VALUE;
            }
            else if (strcmp(arg,"system") == 0)
            {
                commandLineNextArgType = NONEDIT_SYSTEM_VALUE;
            }
            else
            {
                throw mk::Exception_t(
                        mk::format(LE_I18N("'%s' is invalid key command."), arg)
                );
            }
            break;

        case COMPONENT:
            if (strcmp(arg, "app") == 0)
            {
                commandLineNextArgType = NONEDIT_APP_VALUE;
            }
            else if (strcmp (arg, "system") == 0)
            {
                commandLineNextArgType = NONEDIT_SYSTEM_VALUE;
            }
            else
            {
                throw mk::Exception_t(
                        mk::format(LE_I18N("'%s' is invalid key command."), arg)
                );
            }
            break;

        case MODULE:
            if (strcmp(arg, "system") == 0)
            {
                commandLineNextArgType = NONEDIT_SYSTEM_VALUE;
            }
            else
            {
                throw mk::Exception_t(
                        mk::format(LE_I18N("'%s' is invalid key command."), arg)
                );
            }
            break;

        case SYSTEM:
            commandLineNextArgType = NONEDIT_SYSTEM_VALUE;
            break;

        case APPSEARCH:
        case COMPONENTSEARCH:
        case MODULESEARCH:
        case INTERFACESEARCH:
            if (strcmp(arg, "system") == 0)
            {
                commandLineNextArgType = NONEDIT_SYSTEM_VALUE;
            }
            else
            {
                throw mk::Exception_t(
                        mk::format(LE_I18N("'%s' is invalid key command."), arg)
                );
            }
            break;

        case SANDBOXED:
        case START:
            if (strcmp(arg, "app") == 0)
            {
                commandLineNextArgType = NONEDIT_APP_VALUE;
            }
            else
            {
                throw mk::Exception_t(
                        mk::format(LE_I18N("'%s' is invalid key command."), arg)
                );
            }
            break;

        default:
            throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid item type for editing."), arg)
            );
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function call operator() to set the defintion file variables, command line argument enums,
 * edit action type enum, edit item type enums, etc.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::operator()
(
    const char* arg
)
{
    switch (commandLineNextArgType)
    {
        case ACTION_KEY:
        {
            std::map<std::string, EditActionType_t> commands =
            {
                {"add", EditActionType_t::ADD},
                {"create", EditActionType_t::CREATE},
                {"rename", EditActionType_t::RENAME},
                {"remove", EditActionType_t::REMOVE},
                {"delete", EditActionType_t::DELETE}
            };

            if (commands.find(arg) != commands.end())
            {
                editActionType = commands[arg];
                commandLineNextArgType = EDIT_ITEM_KEY;
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid action command."), arg)
                );
            }
            break;
        }

        case EDIT_ITEM_KEY:
        {
            std::map<std::string, EditItemType_t> commands =
            {
                {"app", APP},
                {"component", COMPONENT},
                {"module", MODULE},
                {"system", SYSTEM},
                {"appSearch", APPSEARCH},
                {"componentSearch", COMPONENTSEARCH},
                {"moduleSearch", MODULESEARCH},
                {"interfaceSearch", INTERFACESEARCH},
                {"sandboxed", SANDBOXED},
                {"start", START}
            };

            if (commands.find(arg) != commands.end())
            {
                editItemType = commands[arg];
                commandLineNextArgType = EDIT_ITEM_VALUE;
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

    case EDIT_ITEM_VALUE:
    {
        if (editActionType == RENAME)
        {
            ActionRenameSetDefFilePath(arg);
        }
        else
        {
            ActionNotRenameSetDefFilePath(arg);
        }
        break;
    }

    case NONEDIT_ITEM_KEY:
    {
        EvaluateCommandLineNextArgType(arg);
        break;
    }

    case NONEDIT_APP_VALUE:
    {
        if (adefFilePath.empty())
        {
            adefFilePath = arg;
        }
        else
        {
            throw mk::Exception_t(
                        mk::format(LE_I18N("App name '%s' already provided."), adefFilePath)
                );
        }
        commandLineNextArgType = NONEDIT_ITEM_KEY;
        break;
    }

    case NONEDIT_COMP_VALUE:
    {
        if (cdefFilePath.empty())
        {
            cdefFilePath = arg;
        }
        else
        {
            throw mk::Exception_t(
                        mk::format(LE_I18N("Component name '%s' already provided."), cdefFilePath)
                );
        }
        commandLineNextArgType = NONEDIT_ITEM_KEY;
        break;
    }

    case NONEDIT_SYSTEM_VALUE:
    {
        if (sdefFilePath.empty())
        {
            sdefFilePath = arg;
            commandLineNextArgType = EDIT_COMPLETE;
        }
        else
        {
            throw mk::Exception_t(
                        mk::format(LE_I18N("System name '%s' already provided."), sdefFilePath)
                );
        }
        break;
    }

    default:
        throw mk::Exception_t(
                 mk::format(LE_I18N("Internal error: '%s' is invalid command argument type."), arg)
        );
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add each action to the editActions vector and execute the action. If the action is successful
 * (i.e. no error exceptions occurred) then mark the action as SUCCESS.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::AddAction
(
    std::shared_ptr<EditAction_t> action
)
{
    Handler.SetEditSuccess(ArgHandler_t::EditActionState_t::PENDING);

    // Add the action to the editActions vector
    Handler.editActions.push_back(action);
    // Execute the action
    Handler.editActions.back()->DoAction();

    // No exception thrown from action, mark the action as a success
    Handler.SetEditSuccess(ArgHandler_t::EditActionState_t::SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle create command.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::Create()
{
    switch (editItemType)
    {
        case APP:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, false));
            AddAction(std::make_shared<GenerateComponentTemplateAction_t>(*this));
            AddAction(std::make_shared<GenerateDefTemplateAction_t>(*this));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;

        case COMPONENT:
            AddAction(std::make_shared<CheckDirExistAction_t>(*this, absCdefFilePath, false));
            AddAction(std::make_shared<GenerateComponentTemplateAction_t>(*this));
            if (file::FileExists(absAdefFilePath))
            {
                AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
                AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                               absAdefFilePath));
            }
            break;

        case MODULE:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absMdefFilePath, false));
            AddAction(std::make_shared<GenerateDefTemplateAction_t>(*this));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;

        case SYSTEM:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absSdefFilePath, false));
            AddAction(std::make_shared<GenerateDefTemplateAction_t>(*this));
            break;

        default:
            throw mk::Exception_t(LE_I18N("Internal error: edit item type is invalid."));
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle add command.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::Add()
{
    switch (editItemType)
    {
        case APP:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;

        case COMPONENT:
            AddAction(std::make_shared<CheckDirExistAction_t>(*this, absCdefFilePath, true));
            if (!file::FileExists(absAdefFilePath))
            {
                throw mk::Exception_t(LE_I18N("Application definition file to add is empty."));
            }
            AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absAdefFilePath));
            break;

        case MODULE:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absMdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;

        case SYSTEM:
            throw mk::Exception_t(LE_I18N("Adding system command is not supported."));
            break;

        case APPSEARCH:
        case COMPONENTSEARCH:
        case MODULESEARCH:
        case INTERFACESEARCH:
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;

        case SANDBOXED:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absAdefFilePath));
            break;

        case START:
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absAdefFilePath));
            break;

        default:
            throw mk::Exception_t(LE_I18N("Internal error: edit item type is invalid."));
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check for number of sdefs in current directory.
 **/
//--------------------------------------------------------------------------------------------------
static void CheckCurDirSdef()
{
    std::list<std::string> curDirFiles = file::ListFiles(path::GetCurrentDir());
    std::string sdefFile;
    int count = 0;

    for(const auto &it : curDirFiles)
    {
        if (path::HasSuffix(it, SDEF_EXT))
        {
            count++;
            sdefFile = it;
        }
    }

    if (count > 1)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("More than one sdef found in '%s'. Specify system definition file."),
                       path::GetCurrentDir())
         );
    }
    else
    {
        Handler.sdefFilePath = sdefFile;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check for number of adefs in current directory.
 **/
//--------------------------------------------------------------------------------------------------
static void CheckCurDirAdef()
{
    std::list<std::string> curDirFiles = file::ListFiles(path::GetCurrentDir());
    std::string adefFile;
    int count = 0;

    for(const auto &it : curDirFiles)
    {
        if (path::HasSuffix(it, ADEF_EXT))
        {
            count++;
            adefFile = it;
        }
    }

    // If there is only one ADEF file in the current directory, consider this ADEF file.
    if (count == 1)
    {
        Handler.adefFilePath = path::MakeAbsolute(adefFile);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle rename command.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::Rename()
{
    switch (editItemType)
    {
        case APP:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, oldAdefFilePath, true));
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, false));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameFileAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;
        }

        case COMPONENT:
        {
            AddAction(std::make_shared<CheckDirExistAction_t>(*this, oldCdefFilePath, true));
            AddAction(std::make_shared<CheckDirExistAction_t>(*this, absCdefFilePath, false));

            std::string absOldCdefFile = oldCdefFilePath + "/" + COMP_CDEF;

            // Find apps that lists the components to be renamed and insert in adefFilePathList.
            for (const auto& it : systemPtr->apps)
            {
                for (auto const& itcomp : it.second->components)
                {
                    if (itcomp->defFilePtr->path.compare(absOldCdefFile) == 0)
                    {
                        adefFilePathList.push_back(it.second->defFilePtr->path);
                    }
                }
            }

            if (!adefFilePath.empty())
            {
                // If single app is provided, but multiple apps lists the component.
                if (adefFilePathList.size() > 1)
                {
                    std::cerr << mk::format(
                                    LE_I18N("** WARNING: component '%s' listed in multiple apps:\n"),
                                            oldCdefFilePath);
                    for (const auto& it : adefFilePathList)
                    {
                        std::cerr << mk::format(LE_I18N("%s\n"), it);
                    }
                }

                adefFilePathList.clear();
                adefFilePathList.push_back(absAdefFilePath);
            }

            std::vector<std::string> adefFilePathListUpdate;
            std::vector<std::string> cdefFilePathListUpdate;

            // Check each app in adefFilePathList.
            for (const auto& it : adefFilePathList)
            {
                absAdefFilePath = it;
                if (file::FileExists(absAdefFilePath))
                {
                    bool checkAdef = CheckAdefForComponentUpdating(*this, absAdefFilePath,
                                                                   absOldCdefFile);

                    if (checkAdef)
                    {
                        adefFilePathListUpdate.push_back(absAdefFilePath);
                    }

                    std::vector<std::string> cdefFilePathList = CollectDependingComponents (*this,
                                                                  absAdefFilePath, absOldCdefFile);
                    cdefFilePathListUpdate.insert(cdefFilePathListUpdate.end(),
                                                  cdefFilePathList.begin(),
                                                  cdefFilePathList.end());
                }
            }

            // Update each app in adefFilePathListUpdate.
            for (const auto& it : adefFilePathListUpdate)
            {
                std::string absAdefFilePathUpdate = it;
                if (file::FileExists(absAdefFilePathUpdate))
                {
                    AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
                    AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                        absAdefFilePathUpdate));
                }
            }

            // Update each component in cdefFilePathList.
            for (const auto& it : cdefFilePathListUpdate)
            {
                std::string absCdefFilePathUpdate = it;
                if (file::FileExists(absCdefFilePathUpdate))
                {
                    AddAction(std::make_shared<CreateUpdateTempCdefAction_t>(*this,
                                                                        absCdefFilePathUpdate));
                    AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                        absCdefFilePathUpdate));
                }
            }

            AddAction(std::make_shared<RenameFileAction_t>(*this));
            break;
        }

        case MODULE:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, oldMdefFilePath, true));
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absMdefFilePath, false));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameFileAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;
        }

        case SYSTEM:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, oldSdefFilePath, true));
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absSdefFilePath, false));
            AddAction(std::make_shared<RenameFileAction_t>(*this));
            break;
        }

        default:
        {
            throw mk::Exception_t(LE_I18N("Internal error: edit item type is invalid"));
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler delete command.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::Delete()
{
    switch (editItemType)
    {
        case APP:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RemoveFileAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;
        }

        case COMPONENT:
        {
            std::string absCdefFile = absCdefFilePath + "/" + COMP_CDEF;
            AddAction(std::make_shared<CheckDirExistAction_t>(*this, absCdefFilePath, true));

            // Find apps that list the components to be removed and insert in adefFilePathList.
            for (const auto& it : systemPtr->apps)
            {
                for (auto const& itcomp : it.second->components)
                {
                    if (itcomp->defFilePtr->path.compare(absCdefFile) == 0)
                    {
                        adefFilePathList.push_back(it.second->defFilePtr->path);
                    }
                }
            }

           // Boolean flag to track if the specified component directory is safe to be removed.
           bool isCompSafeToRemove = false;

            // The component directory is safe to be removed if:
            // - Only the specified app refers to the specified component.
            // - No app is specified.
            if (!adefFilePath.empty())
            {
                // If app name is specified and no other apps list the specified component.
                if (adefFilePathList.size() == 0)
                {
                    isCompSafeToRemove = true;
                }

                // If app name is specified and only that app lists the specified component.
                if (adefFilePathList.size() == 1)
                {
                    if (adefFilePathList.back().compare(absAdefFilePath) == 0)
                    {
                        isCompSafeToRemove = true;
                    }
                }

                adefFilePathList.clear();
                // Push the specified app to the vector.
                adefFilePathList.push_back(absAdefFilePath);
            }
            else
            {
                // If no app name is specified, remove the component directory.
                isCompSafeToRemove = true;
            }

            std::vector<std::string> adefFilePathListUpdate;
            std::vector<std::string> cdefFilePathListUpdate;

            // Check each app in adefFilePathList.
            for (const auto& it : adefFilePathList)
            {
                absAdefFilePath = it;
                if (file::FileExists(absAdefFilePath))
                {
                    bool checkAdef = CheckAdefForComponentUpdating(*this, absAdefFilePath,
                                                                   absCdefFile);
                    if (checkAdef)
                    {
                        adefFilePathListUpdate.push_back(absAdefFilePath);
                    }
                    std::vector<std::string> cdefFilePathList = CollectDependingComponents(*this,
                                                                      absAdefFilePath,absCdefFile);
                    cdefFilePathListUpdate.insert(cdefFilePathListUpdate.end(),
                                                  cdefFilePathList.begin(),
                                                  cdefFilePathList.end());
                }
            }

            // Update each app in adefFilePathListUpdate.
            for (const auto& it : adefFilePathListUpdate)
            {
                std::string absAdefFilePathUpdate = it;

                if (file::FileExists(absAdefFilePathUpdate))
                {
                    AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
                    AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                        absAdefFilePathUpdate));
                }
            }

            // Update each component in cdefFilePathList.
            for (const auto& it : cdefFilePathListUpdate)
            {
                std::string absCdefFilePathUpdate = it;

                if (file::FileExists(absCdefFilePathUpdate))
                {
                    AddAction(std::make_shared<CreateUpdateTempCdefAction_t>(*this,
                                                                        absCdefFilePathUpdate));
                    AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                        absCdefFilePathUpdate));
                }
            }

            if (isCompSafeToRemove)
            {
                 AddAction(std::make_shared<RemoveDirAction_t>(*this));
            }
            break;
        }

        case MODULE:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absMdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RemoveFileAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;
        }

        case SYSTEM:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absSdefFilePath, true));
            AddAction(std::make_shared<RemoveFileAction_t>(*this));
            break;
        }

        default:
        {
            throw mk::Exception_t(LE_I18N("Internal error: edit item type is invalid"));
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler remove command.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::Remove()
{
    switch (editItemType)
    {
        case APP:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absAdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;
        }

        case COMPONENT:
        {
            std::string absCdefFile = absCdefFilePath + "/" + COMP_CDEF;
            AddAction(std::make_shared<CheckDirExistAction_t>(*this, absCdefFilePath, true));

            if (!adefFilePath.empty())
            {
                // Push the specified app to the vector.
                adefFilePathList.push_back(absAdefFilePath);
            }
            else
            {
                // Find apps that list the components to be removed and insert in adefFilePathList.
                for (const auto& it : systemPtr->apps)
                {
                    for (auto const& itcomp : it.second->components)
                    {
                        if (itcomp->defFilePtr->path.compare(absCdefFile) == 0)
                        {
                            adefFilePathList.push_back(it.second->defFilePtr->path);
                        }
                    }
                }
            }

            std::vector<std::string> adefFilePathListUpdate;
            std::vector<std::string> cdefFilePathListUpdate;

            // Check each app in adefFilePathList.
            for (const auto& it : adefFilePathList)
            {
                absAdefFilePath = it;
                if (file::FileExists(absAdefFilePath))
                {
                    bool checkAdef = CheckAdefForComponentUpdating(*this, absAdefFilePath,
                                                                   absCdefFile);
                    if (checkAdef)
                    {
                        adefFilePathListUpdate.push_back(absAdefFilePath);
                    }
                    std::vector<std::string> cdefFilePathList = CollectDependingComponents(*this,
                                                                      absAdefFilePath,absCdefFile);
                    cdefFilePathListUpdate.insert(cdefFilePathListUpdate.end(),
                                                  cdefFilePathList.begin(),
                                                  cdefFilePathList.end());
                }
            }

            // Update each app in adefFilePathListUpdate.
            for (const auto& it : adefFilePathListUpdate)
            {
                std::string absAdefFilePathUpdate = it;

                if (file::FileExists(absAdefFilePathUpdate))
                {
                    AddAction(std::make_shared<CreateUpdateTempAdefAction_t>(*this));
                    AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                        absAdefFilePathUpdate));
                }
            }

            // Update each component in cdefFilePathList.
            for (const auto& it : cdefFilePathListUpdate)
            {
                std::string absCdefFilePathUpdate = it;

                if (file::FileExists(absCdefFilePathUpdate))
                {
                    AddAction(std::make_shared<CreateUpdateTempCdefAction_t>(*this,
                                                                        absCdefFilePathUpdate));
                    AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this,
                                                                        absCdefFilePathUpdate));
                }
            }

            break;
        }

        case MODULE:
        {
            AddAction(std::make_shared<CheckDefFileExistAction_t>(*this, absMdefFilePath, true));
            AddAction(std::make_shared<CreateUpdateTempSdefAction_t>(*this));
            AddAction(std::make_shared<RenameTempWorkToActiveFileAction_t>(*this, absSdefFilePath));
            break;
        }

        case SYSTEM:
        {
            throw mk::Exception_t(LE_I18N("The remove action is not supported for system"));
            break;
        }

        default:
        {
            throw mk::Exception_t(LE_I18N("Internal error: edit item type is invalid"));
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Append appropriate def file extention.
 **/
//--------------------------------------------------------------------------------------------------
void AddXdefExtension
(
    ArgHandler_t& handler
)
{
    if ((!handler.adefFilePath.empty()) &&
        (path::GetFileNameExtension(handler.adefFilePath).empty()))
    {
        handler.adefFilePath += ADEF_EXT;
    }

    if ((!handler.mdefFilePath.empty()) &&
        (path::GetFileNameExtension(handler.mdefFilePath).empty()))
    {
        handler.mdefFilePath += MDEF_EXT;
    }

    if ((!handler.sdefFilePath.empty()) &&
        (path::GetFileNameExtension(handler.sdefFilePath).empty()))
    {
        handler.sdefFilePath += SDEF_EXT;
    }

    if ((!handler.oldAdefFilePath.empty()) &&
        (path::GetFileNameExtension(handler.oldAdefFilePath).empty()))
    {
        handler.oldAdefFilePath += ADEF_EXT;
    }

    if ((!handler.oldMdefFilePath.empty()) &&
        (path::GetFileNameExtension(handler.oldMdefFilePath).empty()))
    {
        handler.oldMdefFilePath += MDEF_EXT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process all the action commands.
 **/
//--------------------------------------------------------------------------------------------------
void ProcessCommand()
{
    AddXdefExtension(Handler);

    if (!Handler.adefFilePath.empty() ||
        !Handler.cdefFilePath.empty() ||
        !Handler.mdefFilePath.empty())
    {
        // Parse Sdef to read the appSearch and componentSearch list
        updateDefs::ParseSdefReadSearchPath(Handler);

        // Read the search paths to calculate the absolute paths
        if (!path::IsAbsolute(Handler.adefFilePath))
        {
            if (Handler.appSearchPath.empty())
            {
                Handler.absAdefFilePath = path::Combine(path::GetCurrentDir(),
                                                        Handler.adefFilePath);
            }
        }
        else
        {
           Handler.absAdefFilePath = path::MakeAbsolute(Handler.adefFilePath);
        }

        // Read the search paths to calculate the absolute paths
        if (!path::IsAbsolute(Handler.cdefFilePath))
        {
            if (Handler.compSearchPath.empty() && !Handler.cdefFilePath.empty())
            {
                Handler.absCdefFilePath = path::Combine(path::GetCurrentDir(),
                                                        Handler.cdefFilePath);
            }
        }
        else
        {
           Handler.absCdefFilePath = path::MakeAbsolute(Handler.cdefFilePath);
        }

        // Read the search paths to calculate the absolute paths
        if (!path::IsAbsolute(Handler.mdefFilePath))
        {
            if (Handler.moduleSearchPath.empty())
            {
                Handler.absMdefFilePath = path::Combine(path::GetCurrentDir(),
                                                        Handler.mdefFilePath);
            }
        }
        else
        {
           Handler.absMdefFilePath = path::MakeAbsolute(Handler.mdefFilePath);
        }
    }

    Handler.absSdefFilePath = path::MakeAbsolute(Handler.sdefFilePath);

    // Process command line arguments to add/create/rename/remove app, comp, module, system.
    if (Handler.editActionType == ArgHandler_t::EditActionType_t::ADD)
    {
        Handler.Add();
    }
    else if (Handler.editActionType == ArgHandler_t::EditActionType_t::CREATE)
    {
        Handler.Create();
    }
    else if (Handler.editActionType == ArgHandler_t::EditActionType_t::RENAME)
    {
        Handler.Rename();
    }
    else if (Handler.editActionType == ArgHandler_t::EditActionType_t::REMOVE)
    {
        Handler.Remove();
    }
    else if (Handler.editActionType == ArgHandler_t::EditActionType_t::DELETE)
    {
        Handler.Delete();
    }
    else
    {
        throw mk::Exception_t(LE_I18N("Internal error: Invalid argument state to handle."));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Validate command line arguments.
 **/
//--------------------------------------------------------------------------------------------------
void ValidateCommandLineArguments()
{
    if (Handler.editActionType == ArgHandler_t::EditActionType_t::INVALID_ACTION)
    {
        throw mk::Exception_t(LE_I18N("Edit action command is missing."));
    }

    if (Handler.editItemType == ArgHandler_t::EditItemType_t::INVALID_ITEM)
    {
        throw mk::Exception_t(LE_I18N("Item to edit is missing."));
    }

    // Check if the command line arguments are complete or not. Print error if any missing.
    if ((Handler.commandLineNextArgType >= ArgHandler_t::CommandLineNextArgType_t::EDIT_ITEM_VALUE)
        && (Handler.commandLineNextArgType < ArgHandler_t::CommandLineNextArgType_t::EDIT_COMPLETE))
    {
        switch (Handler.commandLineNextArgType)
        {
            case ArgHandler_t::CommandLineNextArgType_t::EDIT_ITEM_VALUE:
            {
                std::string missingItemType;
                switch (Handler.editItemType)
                {
                    case ArgHandler_t::EditItemType_t::APP:
                        missingItemType = "App";
                        break;

                    case ArgHandler_t::EditItemType_t::COMPONENT:
                        missingItemType = "Component";
                        break;

                    case ArgHandler_t::EditItemType_t::MODULE:
                        missingItemType = "Module";
                        break;

                    case ArgHandler_t::EditItemType_t::SYSTEM:
                        missingItemType = "System";
                        break;
                    default:
                        break;
                }

                throw mk::Exception_t(mk::format(LE_I18N("%s name missing."), missingItemType));
                break;
            }
            case ArgHandler_t::CommandLineNextArgType_t::NONEDIT_APP_VALUE:
                if (Handler.adefFilePath.empty())
                {
                    throw mk::Exception_t(LE_I18N("Application name is missing."));
                }
                break;

            case ArgHandler_t::CommandLineNextArgType_t::NONEDIT_COMP_VALUE:
                if (Handler.cdefFilePath.empty())
                {
                    throw mk::Exception_t(LE_I18N("Component name is missing."));
                }
                break;

            case ArgHandler_t::CommandLineNextArgType_t::NONEDIT_SYSTEM_VALUE:
                if (Handler.sdefFilePath.empty())
                {
                    throw mk::Exception_t(LE_I18N("System name missing."));
                }
                break;

            default:
                throw mk::Exception_t(LE_I18N("Internal error: Invalid command line argument type "
                                              "to handle."));
                break;
        }

    }

    // Check for valid definition filename extensions.
    ValidateAdefExtension(Handler.adefFilePath);
    ValidateMdefExtension(Handler.mdefFilePath);
    ValidateSdefExtension(Handler.sdefFilePath);

    ValidateAdefExtension(Handler.oldAdefFilePath);
    ValidateMdefExtension(Handler.oldMdefFilePath);
    ValidateSdefExtension(Handler.oldSdefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments and update the static operating parameters variables.
 *
 * Throws a std::runtime_error exception on failure.
 **/
//--------------------------------------------------------------------------------------------------
static void GetCommandLineArgs
(
    int argc,
    const char** argv
)
//--------------------------------------------------------------------------------------------------
{
    // Lambda function that gets called once for each argument on the command line.
    auto argHandle = [&] (const char* arg)
        {
            Handler(arg);
        };

    // Lambda function that gets called once for each occurrence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path)
        {
            Handler.buildParams.interfaceDirs.push_back(path);
        };


    // Lambda function that gets called once for each occurrence of the source search path
    // argument on the command line.
    auto sourcePathPush = [&](const char* path)
        {
            // In order to preserve original command line functionality, we push this new path into
            // all of the various search paths.
            Handler.buildParams.moduleDirs.push_back(path);
            Handler.buildParams.appDirs.push_back(path);
            Handler.buildParams.componentDirs.push_back(path);
            Handler.buildParams.sourceDirs.push_back(path);
        };

    args::AddOptionalString(&Handler.buildParams.target,
                            "localhost",
                            't',
                            "target",
                            LE_I18N("Set the compile target (e.g., localhost or ar7)."));

    args::AddMultipleString('i',
                            "interface-search",
                            LE_I18N("Add a directory to the interface search path."),
                            ifPathPush);

    args::AddMultipleString('s',
                            "source-search",
                            LE_I18N("Add a directory to the source search path."),
                            sourcePathPush);

    args::AddOptionalFlag(&Handler.buildParams.beVerbose,
                          'v',
                          "verbose",
                          LE_I18N("Set into verbose mode for extra diagnostic information."));

    args::AddOptionalInt(&Handler.buildParams.jobCount,
                         0,
                         'j',
                         "jobs",
                         LE_I18N("Run N jobs in parallel (default derived from CPUs available)"));

    args::AddOptionalFlag(&Handler.buildParams.isDryRun,
                          'd',
                          "dry-run",
                          LE_I18N("Dry run for testing before real process execution."));

    Handler.commandLineNextArgType = ArgHandler_t::CommandLineNextArgType_t::ACTION_KEY;

    // Set loose arguments such as add, system, etc.
    args::SetLooseArgHandler(argHandle);

    // Scan the arguments.
    args::Scan(argc, argv);

    // Validate command line arguments.
    ValidateCommandLineArguments();

    // Finish setting build parameters
    Handler.buildParams.FinishConfig();

    // Add the directory containing the .sdef file to the list of source search directories
    // and the list of interface search directories.
    std::string sdefFileDir = path::GetContainingDir(Handler.sdefFilePath);
    Handler.buildParams.moduleDirs.push_back(sdefFileDir);
    Handler.buildParams.appDirs.push_back(sdefFileDir);
    Handler.buildParams.componentDirs.push_back(sdefFileDir);
    Handler.buildParams.sourceDirs.push_back(sdefFileDir);
    Handler.buildParams.interfaceDirs.push_back(sdefFileDir);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Implements the mkedit functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeEdit
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    FindToolChain(Handler.buildParams);

    Handler.buildParams.argc = argc;
    Handler.buildParams.argv = argv;
    Handler.buildParams.readOnly = true;
    Handler.buildParams.isRelaxedStrictness = true;

    // Get tool chain info from environment variables.
    // (Must be done after command-line args parsing and before setting target-specific env vars.)
    envVars::SetTargetSpecific(Handler.buildParams);

    GetCommandLineArgs(argc, argv);

    if (Handler.editItemType != ArgHandler_t::EditItemType_t::SYSTEM)
    {
        if (Handler.sdefFilePath.empty())
        {
            if (path::HasSuffix(envVars::Get("LEGATO_DEF_FILE"), SDEF_EXT))
            {
                Handler.sdefFilePath = envVars::Get("LEGATO_DEF_FILE");
            }
            else
            {
                CheckCurDirSdef();
            }
        }

        if (!Handler.sdefFilePath.empty())
        {
            // Do not load the model for create commands
            Handler.systemPtr = modeller::GetSystem(Handler.sdefFilePath, Handler.buildParams);
        }
    }

    if (Handler.adefFilePath.empty())
    {
        if (path::HasSuffix(envVars::Get("LEGATO_DEF_FILE"), ADEF_EXT))
        {
            Handler.adefFilePath = envVars::Get("LEGATO_DEF_FILE");
        }
        else
        {
            CheckCurDirAdef();
        }
    }

    ProcessCommand();
}

} // namespace cli
