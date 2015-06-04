//--------------------------------------------------------------------------------------------------
/**
 * @file avManifestGenerator.h
 *
 * Copyright (C), Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_AV_MANIFEST_GENERATOR_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_AV_MANIFEST_GENERATOR_H_INCLUDE_GUARD

namespace airVantage
{

//--------------------------------------------------------------------------------------------------
/**
 * Generates an Air Vantage manifest XML file for a given app.  The file is output in the
 * build's working directory.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateManifest
(
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
);


} // namespace airVantage

#endif // LEGATO_MKTOOLS_AV_MANIFEST_GENERATOR_H_INCLUDE_GUARD
