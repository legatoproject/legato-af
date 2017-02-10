//--------------------------------------------------------------------------------------------------
/**
 * @file interfacesHeaderGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate an interfaces.h file for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateCLangInterfacesHeader
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string outputDir = path::Minimize(buildParams.workingDir
                                        + '/'
                                        + componentPtr->workingDir
                                        + "/src");
    std::string filePath = outputDir + "/interfaces.h";

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating interfaces.h for component '%s' in '%s'."),
                                componentPtr->name, filePath)
                  << std::endl;
    }

    // Make sure the working file output directory exists.
    file::MakeDir(outputDir);

    // Open the interfaces.h file for writing.
    std::ofstream fileStream(filePath, std::ofstream::trunc);
    if (!fileStream.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), filePath)
        );
    }

    std::string includeGuardName = "__" + componentPtr->name
                                        + "_COMPONENT_INTERFACE_H_INCLUDE_GUARD";

    fileStream << "/*\n"
                  " * AUTO-GENERATED interface.h for the " << componentPtr->name << " component.\n"
                  "\n"
                  " * Don't bother hand-editing this file.\n"
                  " */\n"
                  "\n"
                  "#ifndef " << includeGuardName << "\n"
                  "#define " << includeGuardName << "\n"
                  "\n"
                  "#ifdef __cplusplus\n"
                  "extern \"C\" {\n"
                  "#endif\n"
                  "\n";

    model::InterfaceCFiles_t cFiles;

    // #include the client-side .h for each of the .api files from which only data types are used.
    for (auto interfacePtr : componentPtr->typesOnlyApis)
    {
        interfacePtr->GetInterfaceFiles(cFiles);
        fileStream << "#include \""
                   << cFiles.interfaceFile
                   << "\"\n";
    }

    // For each of the component's client-side interfaces, #include the client-side .h file.
    for (auto interfacePtr : componentPtr->clientApis)
    {
        interfacePtr->GetInterfaceFiles(cFiles);
        fileStream << "#include \""
                   << cFiles.interfaceFile
                   << "\"\n";
    }

    // For each of the component's server-side interfaces, #include the server-side .h file.
    for (auto interfacePtr : componentPtr->serverApis)
    {
        interfacePtr->GetInterfaceFiles(cFiles);
        fileStream << "#include \""
                   << cFiles.interfaceFile
                   << "\"\n";
    }

    // Put the finishing touches on interfaces.h.
    fileStream << "\n"
                  "#ifdef __cplusplus\n"
                  "}\n"
                  "#endif\n"
                  "\n"
                  "#endif // " << includeGuardName << "\n";
}


} // namespace code
