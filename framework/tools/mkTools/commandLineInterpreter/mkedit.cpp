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

static mk::BuildParams_t BuildParams;

static ArgHandler_t handler;

static model::System_t* systemPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Definition file name must start only with a letter or an underscore. The file name may only
 * contain letters, numbers and underscores.
 **/
//--------------------------------------------------------------------------------------------------
static void ValidateFileName(std::string fileName)
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
static void ValidateAdefExtension(std::string& filePath)
{
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
static void ValidateMdefExtension(std::string& filePath)
{
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
static void ValidateSdefExtension(std::string& filePath)
{
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
 * Check if the definition file exists or not.
 **/
//--------------------------------------------------------------------------------------------------
static void CheckDefFileExist(std::string filePath, bool fileMustExist)
{
    if (fileMustExist)
    {
        if (!file::FileExists(filePath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Definition file '%s' does not exist."), filePath)
            );
        }
    }
    else
    {
        if (file::FileExists(filePath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Definition file '%s' already exist."), filePath)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the component directory.
 **/
//--------------------------------------------------------------------------------------------------
static void CheckCompDirExist(std::string &dirPath, bool checkDirExist)
{
    if (checkDirExist)
    {
        dirPath = path::MakeCanonical(dirPath);
        if (!file::DirectoryExists(dirPath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Component directory '%s' does not exist."), dirPath)
            );
        }
    }
    else
    {
        if (file::DirectoryExists(dirPath))
        {
             throw mk::Exception_t(
                   mk::format(LE_I18N("Component directory '%s' already exist."), dirPath)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Overload operator and provide functionality for the given command.
 **/
//--------------------------------------------------------------------------------------------------
void ArgHandler_t::operator()(const char* arg)
{
    switch (MyState)
    {
        case State_t::None:
        {
            std::map<std::string, State_t> commands =
                {
                    {"add", State_t::Add},
                    {"create", State_t::Create},
                    {"rename", State_t::Rename},
                    {"remove", State_t::Remove}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid action command."), arg)
                );
            }
            break;
        }

        case State_t::Add:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::AddApp},
                    {"component", State_t::AddComponent},
                    {"module", State_t::AddModule},
                    {"system", State_t::AddSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::AddApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::AddAppName;
            break;
        }

        case State_t::AddAppName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::AddAppNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::AddAppNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::AddComponent:
        {
            cdefFilePath = arg;
            MyState = State_t::AddComponentName;
            break;
        }

        case State_t::AddComponentName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::AddComponentNameApp},
                    {"system", State_t::AddComponentNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::AddComponentNameApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::AddComponentNameAppName;
            break;
        }

        case State_t::AddComponentNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::AddComponentNameAppName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::AddComponentNameAppNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::AddComponentNameAppNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::AddModule:
        {
            mdefFilePath = arg;
            ValidateMdefExtension(mdefFilePath);
            MyState = State_t::AddModuleName;
            break;
        }

        case State_t::AddModuleName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::AddModuleNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::AddModuleNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::AddSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::Create:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::CreateApp},
                    {"component", State_t::CreateComponent},
                    {"module", State_t::CreateModule},
                    {"system", State_t::CreateSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::CreateSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::CreateApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::CreateAppName;
            break;
        }

        case State_t::CreateAppName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"component", State_t::CreateAppNameComponent},
                    {"system", State_t::CreateAppNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::CreateAppNameComponent:
        {
            cdefFilePath = arg;
            MyState = State_t::CreateAppNameComponentName;
            break;
        }

        case State_t::CreateAppNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::CreateAppNameComponentName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::CreateAppNameComponentNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::CreateAppNameComponentNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::CreateComponent:
        {
            cdefFilePath = arg;
            MyState = State_t::CreateComponentName;
            break;
        }

        case State_t::CreateComponentName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::CreateComponentNameApp},
                    {"system", State_t::CreateComponentNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::CreateComponentNameApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            break;
        }

        case State_t::CreateComponentNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::CreateModule:
        {
            mdefFilePath = arg;
            ValidateMdefExtension(mdefFilePath);
            MyState = State_t::CreateModuleName;
            break;
        }

        case State_t::CreateModuleName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::CreateModuleNameSystem},
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }

            break;
        }

        case State_t::CreateModuleNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::Rename:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::RenameApp},
                    {"component", State_t::RenameComponent},
                    {"module", State_t::RenameModule},
                    {"system", State_t::RenameSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RenameApp:
        {
            oldAdefFilePath = arg;
            ValidateAdefExtension(oldAdefFilePath);
            MyState = State_t::RenameAppOld;
            break;
        }

        case State_t::RenameAppOld:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::RenameAppOldNew;
            break;
        }

        case State_t::RenameAppOldNew:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::RenameAppOldNewSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RenameAppOldNewSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RenameComponent:
        {
            oldCdefFilePath = arg;
            MyState = State_t::RenameComponentOld;
            break;
        }

        case State_t::RenameComponentOld:
        {
            cdefFilePath = arg;
            MyState = State_t::RenameComponentOldNew;
            break;
        }

        case State_t::RenameComponentOldNew:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::RenameComponentOldNewApp},
                    {"system", State_t::RenameComponentOldNewSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RenameComponentOldNewApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::RenameComponentOldNewAppName;
            break;
        }

        case State_t::RenameComponentOldNewAppName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::RenameComponentOldNewAppNameSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RenameComponentOldNewSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RenameComponentOldNewAppNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RenameModule:
        {
            oldMdefFilePath = arg;
            ValidateMdefExtension(oldMdefFilePath);
            MyState = State_t::RenameModuleOld;
            break;
        }

        case State_t::RenameModuleOld:
        {
            mdefFilePath = arg;
            ValidateMdefExtension(mdefFilePath);
            MyState = State_t::RenameModuleOldNew;
            break;
        }

        case State_t::RenameModuleOldNew:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::RenameModuleOldNewSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RenameModuleOldNewSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RenameSystem:
        {
            oldSdefFilePath = arg;
            ValidateSdefExtension(oldSdefFilePath);
            MyState = State_t::RenameSystemOld;
            break;
        }

        case State_t::RenameSystemOld:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            MyState = State_t::RenameSystemOldNew;
            break;
        }

        case State_t::Remove:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::RemoveApp},
                    {"component", State_t::RemoveComponent},
                    {"module", State_t::RemoveModule},
                    {"system", State_t::RemoveSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RemoveApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::RemoveAppName;
            break;
        }

        case State_t::RemoveAppName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::RemoveAppNameSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RemoveAppNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RemoveComponent:
        {
            cdefFilePath = arg;
            MyState = State_t::RemoveComponentName;
            break;
        }

        case State_t::RemoveComponentName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"app", State_t::RemoveComponentNameApp},
                    {"system", State_t::RemoveComponentNameSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RemoveComponentNameApp:
        {
            adefFilePath = arg;
            ValidateAdefExtension(adefFilePath);
            MyState = State_t::RemoveComponentNameAppName;
            break;
        }

        case State_t::RemoveComponentNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RemoveComponentNameAppName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::RemoveComponentNameAppNameSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RemoveComponentNameAppNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RemoveModule:
        {
            mdefFilePath = arg;
            ValidateMdefExtension(mdefFilePath);
            MyState = State_t::RemoveModuleName;
            break;
        }

        case State_t::RemoveModuleName:
        {
            std::map<std::string, State_t> commands =
                {
                    {"system", State_t::RemoveModuleNameSystem}
                };

            if (commands.find(arg) != commands.end())
            {
                MyState = commands[arg];
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("'%s' is invalid target command."), arg)
                );
            }
            break;
        }

        case State_t::RemoveModuleNameSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        case State_t::RemoveSystem:
        {
            sdefFilePath = arg;
            ValidateSdefExtension(sdefFilePath);
            break;
        }

        default:
        {
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler create command.
 **/
//--------------------------------------------------------------------------------------------------
static void Create
(
    ArgHandler_t handler
)
{
    if ((handler.MyState >= ArgHandler_t::State_t::CreateApp) &&
        (handler.MyState <= ArgHandler_t::State_t::CreateAppNameSystem))
    {
        CheckDefFileExist(handler.absAdefFilePath, false);
        updateDefs::UpdateSdefApp(handler);
        defs::GenerateCdefTemplate(handler);
        defs::GenerateAdefTemplate(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::CreateComponent)  &&
             (handler.MyState <= ArgHandler_t::State_t::CreateComponentNameSystem))
    {
        CheckCompDirExist(handler.absCdefFilePath, false);
        defs::GenerateCdefTemplate(handler);
        updateDefs::UpdateAdefComponent(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::CreateModule) &&
             (handler.MyState <= ArgHandler_t::State_t::CreateModuleNameSystem))
    {
        CheckDefFileExist(handler.absMdefFilePath, false);
        updateDefs::UpdateSdefModule(handler);
        defs::GenerateMdefTemplate(handler);
    }
    else if (handler.MyState == ArgHandler_t::State_t::CreateSystem)
    {
        CheckDefFileExist(handler.sdefFilePath, false);
        defs::GenerateSdefTemplate(handler);
    }
    else
    {
        throw mk::Exception_t(LE_I18N("Internal: Invalid argument state value"));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle add command.
 **/
//--------------------------------------------------------------------------------------------------
static void Add
(
    ArgHandler_t handler
)
{
    if ((handler.MyState >= ArgHandler_t::State_t::AddApp) &&
        (handler.MyState <= ArgHandler_t::State_t::AddAppNameSystem))
    {
        CheckDefFileExist(handler.absAdefFilePath, true);
        updateDefs::UpdateSdefApp(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::AddComponentNameApp) &&
             (handler.MyState <= ArgHandler_t::State_t::AddComponentNameAppNameSystem))
    {
        CheckCompDirExist(handler.absCdefFilePath, true);
        updateDefs::UpdateAdefComponent(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::AddModule) &&
            (handler.MyState <= ArgHandler_t::State_t::AddModuleNameSystem))
    {
        CheckDefFileExist(handler.absMdefFilePath, true);
        updateDefs::UpdateSdefModule(handler);
    }
    else if (handler.MyState == ArgHandler_t::State_t::AddSystem)
    {
       std::cout<<"\nCommand Not Supported.\n";
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
            mk::format(LE_I18N("More than one sdef found in '%s'. Specify a system definition file."),
                      path::GetCurrentDir())
         );
    }
    else
    {
        handler.sdefFilePath = sdefFile;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check for number of sdefs in current directory.
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

    if (count > 1)
    {
        return;
    }
    else
    {
        handler.adefFilePath = adefFile;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler rename command.
 **/
//--------------------------------------------------------------------------------------------------
static void Rename
(
    ArgHandler_t handler
)
{
    if ((handler.MyState >= ArgHandler_t::State_t::RenameApp) &&
        (handler.MyState <= ArgHandler_t::State_t::RenameAppOldNewSystem))
    {
        CheckDefFileExist(handler.oldAdefFilePath, true);
        CheckDefFileExist(handler.absAdefFilePath, false);
        updateDefs::UpdateSdefRenameApp(handler);
        file::RenameFile(handler.oldAdefFilePath, handler.absAdefFilePath);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::RenameComponent) &&
             (handler.MyState <= ArgHandler_t::State_t::RenameComponentOldNewAppNameSystem))
    {
        CheckCompDirExist(handler.oldCdefFilePath, true);
        CheckCompDirExist(handler.absCdefFilePath, false);
        file::RenameFile(handler.oldCdefFilePath, handler.absCdefFilePath);
        updateDefs::UpdateAdefRenameComponent(handler, systemPtr);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::RenameModule) &&
             (handler.MyState <= ArgHandler_t::State_t::RenameModuleOldNewSystem))
    {
        CheckDefFileExist(handler.oldMdefFilePath, true);
        CheckDefFileExist(handler.absMdefFilePath, false);
        file::RenameFile(handler.oldMdefFilePath, handler.absMdefFilePath);
        updateDefs::UpdateSdefRenameModule(handler);
    }
    else if (handler.MyState == ArgHandler_t::State_t::RenameSystemOldNew)
    {
        CheckDefFileExist(handler.oldSdefFilePath, true);
        CheckDefFileExist(handler.sdefFilePath, false);
        file::RenameFile(handler.oldSdefFilePath, handler.sdefFilePath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler remove command.
 **/
//--------------------------------------------------------------------------------------------------
static void Remove
(
    ArgHandler_t handler
)
{
    if ((handler.MyState >= ArgHandler_t::State_t::RemoveApp) &&
        (handler.MyState <= ArgHandler_t::State_t::RemoveAppNameSystem))
    {
        CheckDefFileExist(handler.absAdefFilePath, true);
        updateDefs::UpdateSdefRemoveApp(handler);
        file::RemoveFile(handler.absAdefFilePath);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::RemoveComponent) &&
             (handler.MyState <= ArgHandler_t::State_t::RemoveComponentNameAppNameSystem))
    {
        CheckCompDirExist(handler.absCdefFilePath, true);
        updateDefs::UpdateAdefRemoveComponent(handler);
        file::DeleteDir(handler.absCdefFilePath);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::RemoveModule) &&
             (handler.MyState <= ArgHandler_t::State_t::RemoveModuleNameSystem))
    {
        CheckDefFileExist(handler.absMdefFilePath, true);
        updateDefs::UpdateSdefRemoveModule(handler);
        file::RemoveFile(handler.absMdefFilePath);
    }
    else if (handler.MyState == ArgHandler_t::State_t::RemoveSystem)
    {
        CheckDefFileExist(handler.sdefFilePath, true);
        file::RemoveFile(handler.sdefFilePath);
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
    AddXdefExtension(handler);

    if (!handler.adefFilePath.empty() ||
        !handler.cdefFilePath.empty() ||
        !handler.mdefFilePath.empty())
    {
        // Read the search paths to calculate the absolute paths
        if (!path::IsAbsolute(handler.adefFilePath) ||
            !path::IsAbsolute(handler.cdefFilePath) ||
            !path::IsAbsolute(handler.mdefFilePath))
        {
            if (handler.MyState != ArgHandler_t::State_t::CreateSystem)
            {
                // Parse Sdef to read the appSearch and componentSearch list
                updateDefs::ParseSdefReadSearchPath(handler);
            }

            if (!handler.isAppSearchPath)
            {
                handler.absAdefFilePath = path::Combine(path::GetCurrentDir(), handler.adefFilePath);
            }

            if (!handler.isCompSearchPath)
            {
                handler.absCdefFilePath = path::Combine(path::GetCurrentDir(), handler.cdefFilePath);
            }

            if (!handler.isModSearchPath)
            {
                handler.absMdefFilePath = path::Combine(path::GetCurrentDir(), handler.mdefFilePath);
            }
        }
        else
        {
           handler.absAdefFilePath = path::MakeAbsolute(handler.adefFilePath);
           handler.absCdefFilePath = path::MakeAbsolute(handler.cdefFilePath);
           handler.absMdefFilePath = path::MakeAbsolute(handler.mdefFilePath);
        }
    }

    // Process command line arguments to add/create/rename/remove app, comp, module, system.
    if ((handler.MyState >= ArgHandler_t::State_t::Add) &&
        (handler.MyState <= ArgHandler_t::State_t::AddSystem))
    {
        Add(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::Create) &&
        (handler.MyState <= ArgHandler_t::State_t::CreateSystem))
    {
        Create(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::Rename) &&
        (handler.MyState <= ArgHandler_t::State_t::RenameSystemOldNew))

    {
        Rename(handler);
    }
    else if ((handler.MyState >= ArgHandler_t::State_t::Remove) &&
             (handler.MyState <= ArgHandler_t::State_t::RemoveSystem))
    {
        Remove(handler);
    }
    else
    {
        throw mk::Exception_t(LE_I18N("Internal: Invalid argument state to handle."));
    }
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
            handler(arg);
        };

    // Lambda function that gets called once for each occurrence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path)
        {
            BuildParams.interfaceDirs.push_back(path);
        };

    // Lambda function that gets called once for each occurrence of the source search path
    // argument on the command line.
    auto sourcePathPush = [&](const char* path)
        {
            // In order to preserve original command line functionality, we push this new path into
            // all of the various search paths.
            BuildParams.moduleDirs.push_back(path);
            BuildParams.appDirs.push_back(path);
            BuildParams.componentDirs.push_back(path);
            BuildParams.sourceDirs.push_back(path);
        };

    args::AddOptionalString(&BuildParams.target,
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

    args::AddOptionalFlag(&BuildParams.beVerbose,
                          'v',
                          "verbose",
                          LE_I18N("Set into verbose mode for extra diagnostic information."));

    args::AddOptionalInt(&BuildParams.jobCount,
                         0,
                         'j',
                         "jobs",
                         LE_I18N("Run N jobs in parallel (default derived from CPUs available)"));

    // Set loose arguments such as add, system, etc.
    args::SetLooseArgHandler(argHandle);

    // Scan the arguments.
    args::Scan(argc, argv);

    // Add the directory containing the .sdef file to the list of source search directories
    // and the list of interface search directories.
    std::string sdefFileDir = path::GetContainingDir(handler.sdefFilePath);
    BuildParams.moduleDirs.push_back(sdefFileDir);
    BuildParams.appDirs.push_back(sdefFileDir);
    BuildParams.componentDirs.push_back(sdefFileDir);
    BuildParams.sourceDirs.push_back(sdefFileDir);
    BuildParams.interfaceDirs.push_back(sdefFileDir);
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
    FindToolChain(BuildParams);

    BuildParams.argc = argc;
    BuildParams.argv = argv;
    BuildParams.readOnly = true;

    // Get tool chain info from environment variables.
    // (Must be done after command-line args parsing and before setting target-specific env vars.)
    envVars::SetTargetSpecific(BuildParams);

    GetCommandLineArgs(argc, argv);

    handler.isAppSearchPath = false;
    handler.isCompSearchPath = false;

    if ((handler.MyState != ArgHandler_t::State_t::RenameSystemOldNew) &&
        (handler.MyState != ArgHandler_t::State_t::CreateSystem))
    {
        if (handler.sdefFilePath.empty())
        {
            if (path::HasSuffix(envVars::Get("LEGATO_DEF_FILE"), SDEF_EXT))
            {
                handler.sdefFilePath = envVars::Get("LEGATO_DEF_FILE");
            }
            else
            {
                if (!handler.cdefFilePath.empty() && handler.adefFilePath.empty())
                {
                    // Do Nothing
                }
                else
                {
                    CheckCurDirSdef();
                }
            }
        }

        if (!handler.sdefFilePath.empty())
        {
            // Do not load the model for create commands
            systemPtr = modeller::GetSystem(handler.sdefFilePath, BuildParams);
        }
    }

    if (handler.adefFilePath.empty())
    {
        if (path::HasSuffix(envVars::Get("LEGATO_DEF_FILE"), ADEF_EXT))
        {
            handler.adefFilePath = envVars::Get("LEGATO_DEF_FILE");
        }
        else
        {
            CheckCurDirAdef();
        }
    }

    ProcessCommand();
}

} // namespace cli
