//--------------------------------------------------------------------------------------------------
/**
 * @file avManifestGenerator.cpp
 *
 * Copyright (C), Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace airVantage
{


//--------------------------------------------------------------------------------------------------
/**
 * Convert the assets action type to a string suitable for the manifest.
 **/
//--------------------------------------------------------------------------------------------------
static std::string AssetActionTypeToStr
(
    model::AssetField_t::ActionType_t actionType
)
//--------------------------------------------------------------------------------------------------
{
    switch (actionType)
    {
        case model::AssetField_t::ActionType_t::TYPE_SETTING:
            return "setting";

        case model::AssetField_t::ActionType_t::TYPE_VARIABLE:
            return "variable";

        case model::AssetField_t::ActionType_t::TYPE_COMMAND:
            return "command";

        case model::AssetField_t::ActionType_t::TYPE_UNSET:
            throw mk::Exception_t(LE_I18N("Internal error: asset actionType has been left unset."));
    }

    throw mk::Exception_t(LE_I18N("Internal error: unexpected value for asset actionType."));
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert the mk asset field name to the manifests version.
 *
 * @return: AV compatible name for the asset field data type name.
 **/
//--------------------------------------------------------------------------------------------------
static std::string AssetDataTypeToAvDt
(
    const std::string& assetDataType
)
//--------------------------------------------------------------------------------------------------
{
    static const std::map<std::string, std::string> dataTypeMap =
        {
            { "bool",   "boolean" },
            { "int",    "int"     },
            { "float",  "double"  },
            { "string", "string"  }
        };

    const auto& found = dataTypeMap.find(assetDataType);

    if (found == dataTypeMap.end())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Internal error: unexpected data type: '%s'."), assetDataType)
        );
    }

    return found->second;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate all of the standard and custom assets for the application.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAssets
(
    std::ofstream& outFile,     ///< The file the asset list is being generated to.
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Start the "assets" section and add hard-coded object instances 0 and 1 (the "Application"
    // and "Process" standard objects).
    outFile << "  <capabilities>" << std::endl
            << "    <communication use=\"legato\"/>" << std::endl
            << "    <data>" << std::endl
            << "      <encoding type=\"LWM2M\">" << std::endl
            << "        <asset default-label=\"Application Objects\" id=\"le_" << appPtr->name << "\">" << std::endl
            << "          <node default-label=\"Application Object\" path=\"0\">" << std::endl
            << "            <variable default-label=\"Version\" path=\"0\" type=\"string\"/>" << std::endl
            << "            <variable default-label=\"Name\" path=\"1\" type=\"string\"/>" << std::endl
            << "            <variable default-label=\"State\" path=\"2\" type=\"int\"/>" << std::endl
            << "            <variable default-label=\"StartMode\" path=\"3\" type=\"int\"/>" << std::endl
            << "          </node>" << std::endl
            << "          <node default-label=\"Process Object\" path=\"1\">" << std::endl
            << "            <variable default-label=\"Name\" path=\"0\" type=\"string\"/>" << std::endl
            << "            <variable default-label=\"ExecName\" path=\"1\" type=\"string\"/>" << std::endl
            << "            <variable default-label=\"State\" path=\"2\" type=\"int\"/>" << std::endl
            << "            <variable default-label=\"FaultAction\" path=\"3\" type=\"int\"/>" << std::endl
            << "            <variable default-label=\"FaultCount\" path=\"4\" type=\"int\"/>" << std::endl
            << "            <variable default-label=\"FaultLogs\" path=\"5\" type=\"string\"/>" << std::endl
            << "          </node>" << std::endl;

    // Custom Objects, starting at 1000.
    unsigned int assetId = 1000;
    for (const auto& componentPtr : appPtr->components)
    {
        for (const auto& asset : componentPtr->assets)
        {
            unsigned int fieldId = 0;

            outFile << "          <node default-label=\"" << asset->GetName()
                    << "\" path=\"" << assetId << "\">"
                    << std::endl;

            for (const auto& fieldPtr : asset->fields)
            {
                outFile << "            <" << AssetActionTypeToStr(fieldPtr->GetActionType()) << " "
                        << "default-label=\"" << fieldPtr->GetName() << "\" "
                        << "path=\"" << fieldId << "\"";

                if (fieldPtr->GetDataType().size() != 0)
                {
                    outFile << " type=\"" << AssetDataTypeToAvDt(fieldPtr->GetDataType()) << "\"";
                }

                outFile << "/>" << std::endl;

                ++fieldId;
            }

            outFile << "          </node>" << std::endl;
            ++assetId;
        }
    }

    outFile << "        </asset>" << std::endl
            << "      </encoding>" << std::endl
            << "    </data>" << std::endl
            << "  </capabilities>" << std::endl;
}


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
    std::string filePath = path::Combine(path::Combine(buildParams.workingDir, appPtr->workingDir),
                                         "manifest.app");

    // Get the application's version.
    std::string versionStr = appPtr->version;
    if (versionStr == "")
    {
        versionStr = "unknown";
    }

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating Air Vantage manifest: %s"), filePath)
                  << std::endl;
    }

    // Open the file for writing.
    std::ofstream outFile(filePath);
    if (outFile.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open, '%s' for writing."), filePath)
        );
    }

    // Write the file's contents.
    outFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
            << "<app:application "
            << "xmlns:app=\"http://www.sierrawireless.com/airvantage/application/1.0\" "
            << "name=\"" << appPtr->name << "\" "
            << "type=\"\" "
            << "revision=\"" << versionStr << "\">" << std::endl;

    outFile << "  <application-manager use=\"LWM2M_SW\"/>" << std::endl;

    GenerateAssets(outFile, appPtr);

    outFile << "</app:application>" << std::endl;
}


} // namespace airVantage
