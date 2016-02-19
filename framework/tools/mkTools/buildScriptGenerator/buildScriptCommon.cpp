//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptCommon.cpp
 *
 * Implementation of common functions shared by the build script generators.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"


namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Create a build script file and open it.
 **/
//--------------------------------------------------------------------------------------------------
void OpenFile
(
    std::ofstream& script,
    const std::string& filePath,
    bool beVerbose
)
//--------------------------------------------------------------------------------------------------
{
    if (beVerbose)
    {
        std::cout << "Generating ninja build script: '" << filePath << "'." << std::endl;
    }

    file::MakeDir(path::GetContainingDir(filePath));

    script.open(filePath, std::ofstream::trunc);
    if (!script.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePath + "' for writing.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Close a build script file and check for errors.
 **/
//--------------------------------------------------------------------------------------------------
void CloseFile
(
    std::ofstream& script
)
//--------------------------------------------------------------------------------------------------
{
    script.close();
    if (script.fail())
    {
        throw mk::Exception_t("Failed to close file.");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ifgenFlags variable definition.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateIfgenFlagsDef
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const std::list<std::string>& interfaceDirs ///< List of directories to search for .api files.
)
//--------------------------------------------------------------------------------------------------
{
    script << "ifgenFlags = ";

    // Add the interface search directories to ifgen's command-line.
    for (const auto& dir : interfaceDirs)
    {
        script << " --import-dir " << dir;
    }

    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate generic build rules.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildRules
(
    std::ofstream& script,      ///< Ninja script to write rules to.
    const std::string& target,  ///< Build target (e.g., "localhost");
    int argc,                   ///< Count of the number of command line parameters.
    const char** argv           ///< Pointer to array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    std::string cCompilerPath = GetCCompilerPath(target);
    std::string cxxCompilerPath = GetCxxCompilerPath(target);

    // Generate rule for compiling a C source code file.
    script << "rule CompileC\n"
              "  description = Compiling C source\n"
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << cCompilerPath << " -MMD -MF $out.d -c $in -o $out"
              " -Wall" // Enable all warnings.
              " -fPIC" // Compile to position-independent code for linking into a shared library.
              " -Werror" // Treat all warnings as errors.
              " -fvisibility=hidden" // Prevent exporting of symbols by default.
              " -DMK_TOOLS_BUILD"; // Indicate build is being done by the mk tools.
    if (target != "localhost")
    {
        script << "  -DLEGATO_EMBEDDED";    // Indicate target is an embedded device (not a PC).
    }
    script << " $cFlags" // Include user-provided CFLAGS last so other settings can be overridden.
              "\n\n";

    // Generate rule for compiling a C++ source code file.
    script << "rule CompileCxx\n"
              "  description = Compiling C++ source\n"
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << cxxCompilerPath << " -MMD -MF $out.d -c $in -o $out"
              " -Wall" // Enable all warnings.
              " -fPIC" // Compile to position-independent code for linking into a shared library.
              " -Werror" // Treat all warnings as errors.
              " -fvisibility=hidden " // Prevent exporting of symbols by default.
              " -DMK_TOOLS_BUILD"; // Indicate build is being done by the mk tools.
    if (target != "localhost")
    {
        script << "  -DLEGATO_EMBEDDED";    // Indicate target is an embedded device (not a PC).
    }
    script << " $cxxFlags"// Include user-provided CXXFLAGS last so other settings can be overridden
              "\n\n";

    // Generate rules for linking C and C++ object code files into shared libraries.
    script << "rule LinkCLib\n"
              "  description = Linking C library\n"
              "  command = " << cCompilerPath << " -shared -o $out $in $ldFlags\n"
              "\n";

    script << "rule LinkCxxLib\n"
              "  description = Linking C++ library\n"
              "  command = " << cxxCompilerPath << " -shared -o $out $in $ldFlags\n"
              "\n";

    script << "rule LinkCExe\n"
              "  description = Linking C executable\n"
              "  command = " << cCompilerPath << " -o $out $in $ldFlags\n"
              "\n";

    script << "rule LinkCxxExe\n"
              "  description = Linking C++ executable\n"
              "  command = " << cxxCompilerPath << " -o $out $in $ldFlags\n"
              "\n";

    // Generate a rule for running ifgen.
    script << "rule GenInterfaceCode\n"
              "  description = Generating IPC interface code\n"
              "  command = mkdir -p $outputDir && ifgen --output-dir $outputDir $ifgenFlags $in\n"
              "\n";

    // Generate a rule for creating a directory.
    script << "rule MakeDir\n"
              "  description = Creating directory\n"
              "  command = mkdir -p $out\n\n";

    // Generate a rule for creating a hard link.
    script << "rule HardLink\n"
              "  description = Creating hard link\n"
              "  command = ln -T -f $in $out\n"
              "\n";

    // Generate a rule for re-building the build.ninja script when it is out of date.
    script << "rule RegenNinjaScript\n"
              "  description = Regenerating build script\n"
              "  generator = 1\n"
              "  command = " << argv[0] << " --dont-run-ninja";
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--dont-run-ninja") != 0)
        {
            script << " \"" << argv[i] << '"';
        }
    }
    script << "\n"
              "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Stream out (to a given ninja script) the compiler command line arguments required
 * to Set the DT_RUNPATH variable inside the executable's ELF headers to include the expected
 * on-target runtime locations of the libraries needed.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRunPathLdFlags
(
    std::ofstream& script,
    const std::string& target
)
//--------------------------------------------------------------------------------------------------
{
    // DT_RUNPATH is set using linker parameters --enable-new-dtags and -rpath.
    // $ORIGIN is a way of referring to the location of the executable (or shared library) file
    // when it is loaded by the dynamic linker/loader at runtime.
    script << " -Wl,--enable-new-dtags,-rpath=\"\\$$ORIGIN/../lib";

    // When building for execution on the build host, add the localhost bin/lib directory.
    if (target == "localhost")
    {
        script << ":$$LEGATO_BUILD/framework/lib";
    }

    script << "\"";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write out to a given script file a space-separated list of paths to all the .api files needed
 * by a given .api file (specified through USETYPES statements in the .api files).
 **/
//--------------------------------------------------------------------------------------------------
static void GetIncludedApis
(
    std::ofstream& script,  ///< Build script to write to.
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto includedApiPtr : apiFilePtr->includes)
    {
        script << " " << includedApiPtr->path;

        // Recurse.
        GetIncludedApis(script, includedApiPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the interface header file for a given
 * .api file that has been referred to by a USETYPES statement in another .api file used by
 * a client-side interface.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateClientUsetypesHFileBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::ApiFile_t* apiFilePtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
)
//--------------------------------------------------------------------------------------------------
{
    std::string headerFile = apiFilePtr->GetClientInterfaceFile(apiFilePtr->defaultPrefix);

    if (generatedSet.find(headerFile) == generatedSet.end())
    {
        generatedSet.insert(headerFile);

        script << "build $builddir/" << headerFile <<
                  ": GenInterfaceCode " << apiFilePtr->path << " |";
        GetIncludedApis(script, apiFilePtr);
        script << "\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(headerFile) << "\n"
                  "  ifgenFlags = --gen-interface $ifgenFlags\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the interface header file for a given
 * .api file that has been referred to by a USETYPES statement in another .api file used by
 * a server-side interface.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateServerUsetypesHFileBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::ApiFile_t* apiFilePtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
)
//--------------------------------------------------------------------------------------------------
{
    std::string headerFile = apiFilePtr->GetServerInterfaceFile(apiFilePtr->defaultPrefix);

    if (generatedSet.find(headerFile) == generatedSet.end())
    {
        generatedSet.insert(headerFile);

        script << "build $builddir/" << headerFile <<
                  ": GenInterfaceCode " << apiFilePtr->path << " |";
        GetIncludedApis(script, apiFilePtr);
        script << "\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(headerFile) << "\n"
                  "  ifgenFlags = --gen-server-interface $ifgenFlags\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the header file for a given types-only
 * included API interface.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::ApiTypesOnlyInterface_t* ifPtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
)
//--------------------------------------------------------------------------------------------------
{
    if (generatedSet.find(ifPtr->interfaceFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->interfaceFile);

        script << "build $builddir/" << ifPtr->interfaceFile << ":"
                  " GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(script, ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags = --gen-interface"
                  " --name-prefix " << ifPtr->internalName <<
                  " --file-prefix " << ifPtr->internalName << " $ifgenFlags\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(ifPtr->interfaceFile) << "\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the object file for a given client-side
 * API interface.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::ApiClientInterface_t* ifPtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (!buildParams.codeGenOnly)
        && (generatedSet.find(ifPtr->objectFile) == generatedSet.end())  )
    {
        generatedSet.insert(ifPtr->objectFile);

        // .o file
        script << "build $builddir/" << ifPtr->objectFile << ":"
                  " CompileC $builddir/" << ifPtr->sourceFile;

        // Add dependencies on the generated .h files for this interface so we make sure
        // those get built first.
        script << " | $builddir/" << ifPtr->internalHFile << " $builddir/" << ifPtr->interfaceFile;

        // Build a set containing all the .h files that will be included by the .h file generated
        // for this .api file.
        std::set<std::string> apiHeaders;
        ifPtr->apiFilePtr->GetClientUsetypesApiHeaders(apiHeaders);

        // If there are some, add them as order-only dependencies.
        for (auto hFilePath : apiHeaders)
        {
            script << " $builddir/" << hFilePath;
        }

        // Define a cFlags variable that tells the compiler where to look for the interface
        // headers needed due to USETYPES statements.
        script << "\n"
                  "  cFlags = $cFlags";
        std::set<std::string> includeDirs;
        for (auto hFilePath : apiHeaders)
        {
            auto dirPath = path::GetContainingDir(hFilePath);
            if (includeDirs.find(dirPath) == includeDirs.end())
            {
                includeDirs.insert(dirPath);
                script << " -I$builddir/" << dirPath;
            }
        }
        script << "\n\n";
    }

    // .c file and .h files
    std::string generatedFiles;
    std::string ifgenFlags;
    if (generatedSet.find(ifPtr->sourceFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->sourceFile);
        generatedFiles += " $builddir/" + ifPtr->sourceFile;
        ifgenFlags += " --gen-client";
    }
    if (generatedSet.find(ifPtr->interfaceFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->interfaceFile);
        generatedFiles += " $builddir/" + ifPtr->interfaceFile;
        ifgenFlags += " --gen-interface";
    }
    if (generatedSet.find(ifPtr->internalHFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->internalHFile);
        generatedFiles += " $builddir/" + ifPtr->internalHFile;
        ifgenFlags += " --gen-local";
    }
    if (!generatedFiles.empty())
    {
        ifgenFlags += " --name-prefix " + ifPtr->internalName
                   +  " --file-prefix " + ifPtr->internalName;
        script << "build" << generatedFiles <<
                  ": GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(script, ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags =" << ifgenFlags << " $ifgenFlags\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(ifPtr->sourceFile) << "\n\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the object file for a given server-side
 * API interface.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::ApiServerInterface_t* ifPtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (!buildParams.codeGenOnly)
        && (generatedSet.find(ifPtr->objectFile) == generatedSet.end())  )
    {
        generatedSet.insert(ifPtr->objectFile);

        // .o file
        script << "build $builddir/" << ifPtr->objectFile << ":"
                  " CompileC $builddir/" << ifPtr->sourceFile;

        // Add order-only dependencies on the generated .h files for this interface so we make sure
        // those get built first.
        script << " | $builddir/" << ifPtr->internalHFile << " $builddir/" << ifPtr->interfaceFile;

        // Build a set containing all the .h files that will be included (via USETYPES statements)
        // by the .h file generated for this .api file.
        std::set<std::string> apiHeaders;
        ifPtr->apiFilePtr->GetServerUsetypesApiHeaders(apiHeaders);

        // If there are some, add them as order-only dependencies.
        for (auto hFilePath : apiHeaders)
        {
            script << " $builddir/" << hFilePath;
        }

        // Define a cFlags variable that tells the compiler where to look for the interface
        // headers needed due to USETYPES statements.
        script << "\n"
                  "  cFlags = $cFlags";
        std::set<std::string> includeDirs;
        for (auto hFilePath : apiHeaders)
        {
            auto dirPath = path::GetContainingDir(hFilePath);
            if (includeDirs.find(dirPath) == includeDirs.end())
            {
                includeDirs.insert(dirPath);
                script << " -I$builddir/" << dirPath;
            }
        }
        script << "\n\n";
    }

    // .c file and .h files
    std::string generatedFiles;
    std::string ifgenFlags;
    if (generatedSet.find(ifPtr->sourceFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->sourceFile);
        generatedFiles += " $builddir/" + ifPtr->sourceFile;
        ifgenFlags += " --gen-server";
    }
    if (generatedSet.find(ifPtr->interfaceFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->interfaceFile);
        generatedFiles += " $builddir/" + ifPtr->interfaceFile;
        ifgenFlags += " --gen-server-interface";
    }
    if (generatedSet.find(ifPtr->internalHFile) == generatedSet.end())
    {
        generatedSet.insert(ifPtr->internalHFile);
        generatedFiles += " $builddir/" + ifPtr->internalHFile;
        ifgenFlags += " --gen-local";
    }
    if (!generatedFiles.empty())
    {
        if (ifPtr->async)
        {
            ifgenFlags += " --async-server";
        }
        ifgenFlags += " --name-prefix " + ifPtr->internalName
                   +  " --file-prefix " + ifPtr->internalName;
        script << "build" << generatedFiles << ":"
                  " GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(script, ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags =" << ifgenFlags << " $ifgenFlags\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(ifPtr->sourceFile) << "\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateIpcBuildStatements
(
    std::ofstream& script,
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
)
//--------------------------------------------------------------------------------------------------
{
    for (auto typesOnlyApi : componentPtr->typesOnlyApis)
    {
        GenerateBuildStatement(script, typesOnlyApi, buildParams, generatedSet);
    }

    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        GenerateClientUsetypesHFileBuildStatement(script, apiFilePtr, buildParams, generatedSet);
    }

    for (auto apiFilePtr : componentPtr->serverUsetypesApis)
    {
        GenerateServerUsetypesHFileBuildStatement(script, apiFilePtr, buildParams, generatedSet);
    }

    for (auto clientApi : componentPtr->clientApis)
    {
        GenerateBuildStatement(script, clientApi, buildParams, generatedSet);
    }

    for (auto serverApi : componentPtr->serverApis)
    {
        GenerateBuildStatement(script, serverApi, buildParams, generatedSet);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by all components in the model.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateIpcBuildStatements
(
    std::ofstream& script,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // It's possible that multiple components in the same system will share the same interface.
    // To prevent the generation of multiple build statements (which would cause ninja to fail),
    // we use a set containing the output file paths to keep track of what build statements we've
    // already generated.

    std::set<std::string> generatedSet;

    for (auto& mapEntry : model::Component_t::GetComponentMap())
    {
        GenerateIpcBuildStatements(script, mapEntry.second, buildParams, generatedSet);
    }
}



} // namespace ninja
