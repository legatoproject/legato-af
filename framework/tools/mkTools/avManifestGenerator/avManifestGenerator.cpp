//--------------------------------------------------------------------------------------------------
/**
 * @file avManifestGenerator.cpp
 *
 * Copyright (C), Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace airVantage
{


//--------------------------------------------------------------------------------------------------
/**
 * Generates an Air Vantage manifest XML file for a given app.  The file is output into an
 * "airVantage" subdirectory under the build's working directory.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateManifest
(
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Generate the file path.
    std::string filePath = path::Combine(buildParams.workingDir, "manifest.app");

    // Get the application's version.
    std::string versionStr = appPtr->version;
    if (versionStr == "")
    {
        versionStr = "unknown";
    }

    if (buildParams.beVerbose)
    {
        std::cout << "Generating Air Vantage manifest: " << filePath << std::endl;
    }

    // Open the file for writing.
    std::ofstream outFile(filePath);
    if (outFile.is_open() == false)
    {
        throw mk::Exception_t("Could not open, '" + filePath + ",' for writing.");
    }

    // Write the file's contents.
    outFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
            << "<app:application "
            << "xmlns:app=\"http://www.sierrawireless.com/airvantage/application/1.0\" "
            << "name=\"" << appPtr->name << "\" "
            << "type=\"\" "
            << "revision=\"" << versionStr << "\">" << std::endl;

    outFile << "  <application-manager use=\"LWM2M_SW\"/>" << std::endl;

    outFile << "</app:application>" << std::endl;
}


} // namespace airVantage
