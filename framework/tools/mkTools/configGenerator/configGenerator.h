//--------------------------------------------------------------------------------------------------
/**
 * @file configGenerator.h
 *
 * Copyright (C) Sierra Wireless Inc.
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


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration that the framework needs for a given system.  This is the
 * configuration that will be installed in the system configuration tree by the installer when
 * the system starts for the first time on the target.  It will be output to two files called
 * "apps.cfg" and "users.cfg" in the "config" directory under the system's staging directory.
 *
 * @note This assumes that the "root.cfg" config files for all the apps have already been
 *       generated in the apps' staging directories.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::System_t* systemPtr,     ///< The system to generate the configuration for.
    const mk::BuildParams_t& buildParams
);


/// Convenience typedef for constructing vector of pair of string and Token_t*.
typedef std::vector<std::pair<std::string, parseTree::Token_t*>> VectorPairStringToken_t;

} // namespace config

#endif // LEGATO_MKTOOLS_CONFIG_GENERATOR_H_INCLUDE_GUARD
