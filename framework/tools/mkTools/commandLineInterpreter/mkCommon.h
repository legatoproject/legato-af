//--------------------------------------------------------------------------------------------------
/**
 * @file mkCommon.h
 *
 * Functions shared by different command-line tools.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Run the Ninja build tool.  Executes the build.ninja script in the root of the working directory
 * tree, if it exists.
 *
 * If the build.ninja file exists, this function will never return.  If the build.ninja file doesn't
 * exist, then this function WILL (quietly) return.
 *
 * @throw mktools::Exception if the build.ninja file exists but ninja can't be run.
 */
//--------------------------------------------------------------------------------------------------
void RunNinja
(
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for a given component.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given set.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
(
    const std::set<model::Component_t*>& components,  ///< Set of components to generate code for.
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given map.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
(
    const std::map<std::string, model::Component_t*>& components,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code specific to an individual app (excluding code for the components).
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
);



#endif // LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD
