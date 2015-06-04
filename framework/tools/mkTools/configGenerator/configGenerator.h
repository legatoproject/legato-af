//--------------------------------------------------------------------------------------------------
/**
 * @file configGenerator.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_CONFIG_GENERATOR_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_CONFIG_GENERATOR_H_INCLUDE_GUARD

namespace config
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration that the framework needs for a given app.  This is the configuration
 * that will be installed in the system configuration tree by the installer when the app is
 * installed on the target.  It will be output to a file called "root.cfg" in the app's staging
 * directory.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::App_t* appPtr,       ///< The app to generate the configuration for.
    const mk::BuildParams_t& buildParams
);


} // namespace config

#endif // LEGATO_MKTOOLS_CONFIG_GENERATOR_H_INCLUDE_GUARD
