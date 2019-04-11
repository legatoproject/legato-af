//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef MKEDIT_H_INCLUDE_GUARD
#define MKEDIT_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Class to handler arguments passed on the command line.
 */
//--------------------------------------------------------------------------------------------------
class ArgHandler_t
{
    public:

        std::string adefFilePath;       ///< adef file passed as argument
        std::string cdefFilePath;       ///< cdef file passed as argument
        std::string mdefFilePath;       ///< mdef file passed as argument
        std::string sdefFilePath;       ///< sdef file passed as argument

        std::string absAdefFilePath;    ///< Absolute path of adef
        std::string absCdefFilePath;    ///< Absolute path of cdef
        std::string absMdefFilePath;    ///< Absolute path of mdef

        std::string oldAdefFilePath;    ///< Old adef file name
        std::string oldCdefFilePath;    ///< Old component folder name
        std::string oldMdefFilePath;    ///< Old mdef file name
        std::string oldSdefFilePath;    ///< Old sdef file name

        bool isAppSearchPath;           ///< True if appSearch section is present in sdef
        bool isCompSearchPath;          ///< True if componentSearch section is present in sdef
        bool isModSearchPath;           ///< True if moduleSearch section is present in sdef

        enum class State_t              ///< State of commands passed as arguments
        {
            None,

            Add,
            AddApp,
            AddAppName,
            AddAppNameSystem,
            AddComponent,
            AddComponentName,
            AddComponentNameApp,
            AddComponentNameSystem,
            AddComponentNameAppName,
            AddComponentNameAppNameSystem,
            AddModule,
            AddModuleName,
            AddModuleNameSystem,
            AddSystem,

            Create,
            CreateApp,
            CreateAppName,
            CreateAppNameComponent,
            CreateAppNameComponentName,
            CreateAppNameComponentNameSystem,
            CreateAppNameSystem,
            CreateComponent,
            CreateComponentName,
            CreateComponentNameApp,
            CreateComponentNameSystem,
            CreateModule,
            CreateModuleName,
            CreateModuleNameSystem,
            CreateSystem,

            Rename,
            RenameApp,
            RenameAppOld,
            RenameAppOldNew,
            RenameAppOldNewSystem,
            RenameComponent,
            RenameComponentOld,
            RenameComponentOldNew,
            RenameComponentOldNewApp,
            RenameComponentOldNewSystem,
            RenameComponentOldNewAppName,
            RenameComponentOldNewAppNameSystem,
            RenameModule,
            RenameModuleOld,
            RenameModuleOldNew,
            RenameModuleOldNewSystem,
            RenameSystem,
            RenameSystemOld,
            RenameSystemOldNew,

            Remove,
            RemoveApp,
            RemoveAppName,
            RemoveAppNameSystem,
            RemoveComponent,
            RemoveComponentName,
            RemoveComponentNameApp,
            RemoveComponentNameSystem,
            RemoveComponentNameAppName,
            RemoveComponentNameAppNameSystem,
            RemoveModule,
            RemoveModuleName,
            RemoveModuleNameSystem,
            RemoveSystem
        };

        State_t MyState = State_t::None;

        void operator()(const char* arg);
};


//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkedit functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeEdit
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
);

#endif // MKEDIT_H_INCLUDE_GUARD
