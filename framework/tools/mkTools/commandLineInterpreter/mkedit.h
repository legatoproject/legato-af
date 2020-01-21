//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef MKEDIT_H_INCLUDE_GUARD
#define MKEDIT_H_INCLUDE_GUARD

#include "editAction.h"


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
        std::string absSdefFilePath;    ///< Absolute path of sdef

        std::string oldAdefFilePath;    ///< Old adef file name
        std::string oldCdefFilePath;    ///< Old component folder name
        std::string oldMdefFilePath;    ///< Old mdef file name
        std::string oldSdefFilePath;    ///< Old sdef file name

        std::string tempWorkDefFilePath;  ///< Temporary working file of the definition file to edit

        std::string appSandboxed;       ///< App's sandboxed value
        std::string appStart;           ///< App's start value

        std::vector<std::string> adefFilePathList; // List of apps that need to be updated

        std::list<std::string> appSearchPath;       ///< List of app search path in active sdef
        std::list<std::string> compSearchPath;      ///< List of comp search path in active sdef
        std::list<std::string> moduleSearchPath;    ///< List of module search path in active sdef

        std::string searchPath;         ///< App, component, module, interface search path

        model::System_t* systemPtr;     ///< Pointer for system model
        mk::BuildParams_t buildParams;  ///< Object that stores the gathered build parameters

        enum CommandLineNextArgType_t { ///< Enum to track the command line arguments
            INVALID_ARG = 0,
            ACTION_KEY,
            EDIT_ITEM_KEY,
            NONEDIT_ITEM_KEY,
            EDIT_ITEM_VALUE,
            NONEDIT_APP_VALUE,
            NONEDIT_COMP_VALUE,
            NONEDIT_SYSTEM_VALUE,
            EDIT_COMPLETE
        };

        enum EditActionType_t {         ///< Enum for edit action type
            INVALID_ACTION = 0,
            ADD,
            CREATE,
            RENAME,
            REMOVE,
            DELETE
        };

        enum EditItemType_t {           ///< Enum for edit item type
            INVALID_ITEM = 0,
            APP,
            COMPONENT,
            MODULE,
            SYSTEM,

            APPSEARCH,
            COMPONENTSEARCH,
            INTERFACESEARCH,
            MODULESEARCH,

            SANDBOXED,
            START
        };

        enum EditActionState_t {        ///< Enum for tracking the state of each edit action
            INIT = 0,
            PENDING,
            SUCCESS
        };

        CommandLineNextArgType_t commandLineNextArgType;
        EditActionType_t editActionType;
        EditItemType_t editItemType;
        EditActionState_t editActionState;

        void operator()(const char* arg);

        std::string GetFileForEditItemType();    ///< Get file path for corresponding edit item type
        std::string GetOldFileForEditItemType(); ///< Get old file path for corresponding item type

        void Add();                     ///< Add action
        void Create();                  ///< Create action
        void Remove();                  ///< Remove action
        void Delete();                  ///< Delete action
        void Rename();                  ///< Rename action

        // Vector to store the series of edit actions to complete a full edit
        std::vector<std::shared_ptr<EditAction_t> > editActions;

        struct LinePosition_t
        {
            std::string lineToWrite;  ///< Line to write to definition file
            int beforePos;            ///< Position in definition file before the line to write
            int afterPos;             ///< Position in definition file after the line to write
        };

        std::vector<LinePosition_t> linePositionToWrite; ///< Vector of line and position to write

        void SetEditSuccess(EditActionState_t value) noexcept
        {
            editActionState = value;
        }

        // Set definition file path for rename action
        void ActionRenameSetDefFilePath(const char* arg);

        // Set definition file path for non-rename action
        void ActionNotRenameSetDefFilePath(const char* arg);

        // Evaluate the next command line argument type
        void EvaluateCommandLineNextArgType(const char* arg);

        // Add action to the edit action vector
        void AddAction(std::shared_ptr<EditAction_t> action);

        // Print logs based on buildParams value
        bool isPrintLogging();

        // Constructor
        ArgHandler_t()
            : commandLineNextArgType(INVALID_ARG),
            editActionType(INVALID_ACTION),
            editItemType(INVALID_ITEM),
            editActionState(INIT)
        {}

        // Destructor
        virtual ~ArgHandler_t()
        {
            // If the action is PENDING, it means an error exception occurred.
            // Execute undo action for each edit action.
            if (editActionState == PENDING)
            {
                while (!editActions.empty())
                {
                    editActions.back()->UndoAction();
                    editActions.pop_back();
                }
            }
        }
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
