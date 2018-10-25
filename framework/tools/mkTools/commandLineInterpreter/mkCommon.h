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
 * Figure out what compiler, linker, etc. to use based on the target device type and store that
 * info in the @c buildParams object.
 *
 * If a given tool is not specified through the means documented in
 * @ref buildToolsmk_ToolChainConfig, then the corresponding entry in @c buildParams will be
 * left empty.
 */
//--------------------------------------------------------------------------------------------------
void FindToolChain
(
    mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks the info in @c buildParams object for IMA signing. If nothing specified in @c buildParams
 * object then check environment variables for IMA signing and update @c buildParams object
 * accordingly.
 */
//--------------------------------------------------------------------------------------------------
void CheckForIMASigning
(
    mk::BuildParams_t& buildParams
);


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
void GenerateLinuxCode
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for a given component.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosCode
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given map.
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinuxCode
(
    const std::map<std::string, model::Component_t*>& components,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given map.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosCode
(
    const std::map<std::string, model::Component_t*>& components,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code specific to an individual app (excluding code for the components).
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinuxCode
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate code specific to an individual app (excluding code for the components).
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosCode
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
);



#endif // LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD
