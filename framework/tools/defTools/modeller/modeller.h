//--------------------------------------------------------------------------------------------------
/**
 * @file modeller.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODELLER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODELLER_H_INCLUDE_GUARD

namespace modeller
{


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a system whose .sdef file can be found at a given path.
 *
 * @return Pointer to the system object.
 */
//--------------------------------------------------------------------------------------------------
model::System_t* GetSystem
(
    const std::string& sdefPath,    ///< Path to the system's .sdef file.
    mk::BuildParams_t& buildParams  ///< [in,out]
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single application whose .adef file can be found at a given path.
 *
 * @return Pointer to the application object.
 */
//--------------------------------------------------------------------------------------------------
model::App_t* GetApp
(
    const std::string& adefPath,    ///< Path to the application's .adef file.
    const mk::BuildParams_t& buildParams,
    bool isPreBuilt = false         ///< Is this a pre-built app?
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a given directory.
 *
 * @return Pointer to the component object.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const std::string& componentDir,    ///< Directory the component is found in.
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a given directory.
 *
 * @return Pointer to the component object.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const std::string& componentDir,    ///< Directory the component is found in.
    const mk::BuildParams_t& buildParams,
    bool isStandAloneComp
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds an instance of a given component to a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void AddComponentInstance
(
    model::Exe_t* exePtr,
    model::Component_t* componentPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Verifies that all client-side interfaces of all applications in a system have been bound
 * to something.  Will auto-bind any unbound le_cfg or le_wdog interfaces it finds.
 *
 * @throw mk::Exception_t if any client-side interface is unbound.
 */
//--------------------------------------------------------------------------------------------------
void EnsureClientInterfacesBound
(
    model::System_t* systemPtr ///< Pointer to the system object
);


//--------------------------------------------------------------------------------------------------
/**
 * Verifies that all client-side interfaces of an application have either been bound to something
 * or marked as an external interface to be bound at the system level.  Will auto-bind any unbound
 * le_cfg or le_wdog interfaces it finds.
 *
 * @throw mk::Exception_t if any client-side interface is found to be unsatisfied.
 */
//--------------------------------------------------------------------------------------------------
void EnsureClientInterfacesSatisfied
(
    model::App_t* appPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Print a summary of an application object.
 **/
//--------------------------------------------------------------------------------------------------
void PrintSummary
(
    model::App_t* appPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Print a summary of a kernel module object.
 **/
//--------------------------------------------------------------------------------------------------
void PrintSummary
(
    model::Module_t* modulePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Print a summary of a system object.
 **/
//--------------------------------------------------------------------------------------------------
void PrintSummary
(
    model::System_t* systemPtr
);

} // namespace modeller

#endif // LEGATO_DEFTOOLS_MODELLER_H_INCLUDE_GUARD
