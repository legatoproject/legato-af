//--------------------------------------------------------------------------------------------------
/**
 * @file modellerCommon.h
 *
 * Functions shared by multiple modeller modules.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODELLER_COMMON_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODELLER_COMMON_H_INCLUDE_GUARD

namespace modeller
{

//--------------------------------------------------------------------------------------------------
/**
 * Set permissions inside a Permissions_t object based on the contents of a FILE_PERMISSIONS token.
 */
//--------------------------------------------------------------------------------------------------
void GetPermissions
(
    model::Permissions_t& permissions,  ///< [OUT]
    const parseTree::Token_t* tokenPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given bundled file or directory in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetBundledItem
(
    const parseTree::TokenList_t* itemPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given file in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetRequiredFile
(
    const parseTree::TokenList_t* itemPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given directory in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetRequiredDir
(
    const parseTree::TokenList_t* itemPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given device in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetRequiredDevice
(
    const parseTree::TokenList_t* itemPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the signed integer value from a simple (name: value) section.
 *
 * @return the value.
 *
 * @throw mk::Exception if out of range.
 */
//--------------------------------------------------------------------------------------------------
ssize_t GetInt
(
    const parseTree::SimpleSection_t* sectionPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the integer value from a simple (name: value) section and verifies that it is
 * non-negative.
 *
 * @return the value.
 *
 * @throw mk::Exception if out of range.
 */
//--------------------------------------------------------------------------------------------------
size_t GetNonNegativeInt
(
    const parseTree::SimpleSection_t* sectionPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the integer value from a simple (name: value) section and verifies that it is
 * positive.
 *
 * @return the value.
 *
 * @throw mk::Exception if out of range.
 */
//--------------------------------------------------------------------------------------------------
size_t GetPositiveInt
(
    const parseTree::SimpleSection_t* sectionPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Print permissions to stdout.
 **/
//--------------------------------------------------------------------------------------------------
void PrintPermissions
(
    const model::Permissions_t& permissions
);


//--------------------------------------------------------------------------------------------------
/**
 * Function to remove angle brackets from around a non-app user name specification in an IPC_AGENT
 * token's text.
 *
 * E.g., if the agentName is "<root>", then "root" will be returned.
 *
 * @return The user name, without the angle brackets.
 */
//--------------------------------------------------------------------------------------------------
std::string RemoveAngleBrackets
(
    const std::string& agentName    ///< The user name with angle brackets around it.
);


//--------------------------------------------------------------------------------------------------
/**
 * Makes the application a member of groups listed in a given "groups" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
void AddGroups
(
    model::App_t* appPtr,
    const parseTree::TokenListSection_t* sectionPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets whether the Supervisor will start the application automatically at system start-up,
 * or only when asked to do so, based on the contents of a "start:" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
void SetStart
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets whether the Supervisor will load the module automatically at system start-up,
 * or only when asked to do so, based on the contents of a "load:" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
void SetLoad
(
    model::Module_t* modulePtr,
    const parseTree::SimpleSection_t* sectionPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the app-level watchdog action setting.
 */
//--------------------------------------------------------------------------------------------------
void SetWatchdogAction
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the app-level watchdog timeout setting.
 */
//--------------------------------------------------------------------------------------------------
void SetWatchdogTimeout
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the app-level watchdog timeout setting.
 */
//--------------------------------------------------------------------------------------------------
void SetMaxWatchdogTimeout
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the API File object for a given .api file path.
 **/
//--------------------------------------------------------------------------------------------------
model::ApiFile_t* GetApiFilePtr
(
    const std::string& apiFile,
    const std::list<std::string>& searchList,   ///< List of dirs to search for .api files.
    const parseTree::Token_t* tokenPtr  ///< Token to use to throw error exceptions.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add required kernel module section from "requires:" "kernelModules:" section to a given object.
 **/
//--------------------------------------------------------------------------------------------------
void AddRequiredKernelModules
(
    std::map<std::string, model::Module_t::ModuleInfoOptional_t>& requiredModules,
    model::Module_t* modulePtr,
    const std::list<const parseTree::CompoundItem_t*>& reqKernelModulesSections,
    const mk::BuildParams_t& buildParams
);

} // namespace modeller

#endif // LEGATO_DEFTOOLS_MODELLER_COMMON_H_INCLUDE_GUARD
