//--------------------------------------------------------------------------------------------------
/**
 * @file exportedAdefGenerator.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_EXPORTED_ADEF_GENERATOR_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_EXPORTED_ADEF_GENERATOR_H_INCLUDE_GUARD

namespace adefGen
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate a new .adef file based on the given app model.  This new .adef file will be suitable for
 * shipping with a binary only app.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateExportedAdef
(
    model::App_t* appPtr,       ///< Generate an exported adef for this application.
    const mk::BuildParams_t& buildParams
);


} // namespace adefGen

#endif // LEGATO_MKTOOLS_EXPORTED_ADEF_GENERATOR_H_INCLUDE_GUARD
