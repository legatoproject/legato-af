//--------------------------------------------------------------------------------------------------
/**
 * @file system.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_SYSTEM_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_SYSTEM_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single system.
 */
//--------------------------------------------------------------------------------------------------
struct System_t : public HasTargetInfo_t
{
    System_t(parseTree::SdefFile_t* filePtr);

    parseTree::SdefFile_t* defFilePtr;  ///< Pointer to root of parse tree for the .sdef file.

    std::string dir;    ///< Absolute path to the directory containing the .sdef file.

    std::string name;   ///< Name of the system.

    std::map<std::string, App_t*> apps;  ///< Map of apps in this system (key is app name).

    ///< Map of kernel module.
    ///< Key is module name and value is structure of module's pointer and bool 'optional' value.
    std::map<std::string, model::Module_t::ModuleInfoOptional_t> modules;

    std::map<std::string, User_t*> users; ///< Map of non-app users (key is user name).

    std::map<std::string, Command_t*> commands; ///< Map of commands (key is command name).

    std::string externalWatchdogKick; /// < External watchdog kick timer

    App_t* FindApp(const parseTree::Token_t* appTokenPtr);

    ApiServerInterfaceInstance_t* FindServerInterface(const parseTree::Token_t* appTokenPtr,
                                                      const parseTree::Token_t* interfaceTokenPtr);
};


#endif // LEGATO_DEFTOOLS_MODEL_SYSTEM_H_INCLUDE_GUARD
