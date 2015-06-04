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
 * Get the absolute path to a given C or C++ source code file belonging to a given component.
 *
 * @return the absolute path.
 **/
//--------------------------------------------------------------------------------------------------
std::string GetAbsoluteSourcePath
(
    const std::string& sourceFile,  ///< Path (possibly relative) to the source code file.
    const model::Component_t* componentPtr  ///< The component that the source code belongs to.
)
//--------------------------------------------------------------------------------------------------
{
    // If the source file path is absolute, then use it as-is.  Otherwise, we need to prefix
    // it with the component's directory path, because it's relative to that.
    if (path::IsAbsolute(sourceFile))
    {
        return sourceFile;
    }
    else
    {
        return path::Combine(componentPtr->dir, sourceFile);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the object code file for a given C or C++ source code file.
 **/
//--------------------------------------------------------------------------------------------------
std::string GetObjectFile
(
    const std::string& sourceFile   ///< Path to the source code file.
)
//--------------------------------------------------------------------------------------------------
{
    // The object file directory is named after an MD5 hash of the source code file path.
    // This ensures different source code files that share the same name don't end up
    // sharing the same object file path.

    auto sourceFileName = path::GetLastNode(sourceFile);

    return "$builddir/" + path::Combine("obj", md5(sourceFile)) + "/" + sourceFileName + ".o";
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
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << cCompilerPath << " -MMD -MF $out.d $cFlags -c $in -o $out"
              " -Wall" // Enable all warnings.
              " -fPIC" // Compile to position-independent code for linking into a shared library.
              " -Werror" // Treat all warnings as errors.
              " -DMK_TOOLS_BUILD"
              "\n\n";

    // Generate rule for compiling a C++ source code file.
    script << "rule CompileCxx\n"
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << cxxCompilerPath << " -MMD -MF $out.d $cxxFlags -c $in -o $out"
              " -Wall" // Enable all warnings.
              " -fPIC" // Compile to position-independent code for linking into a shared library.
              " -Werror" // Treat all warnings as errors.
              " -DMK_TOOLS_BUILD"
              "\n\n";

    // Generate rules for linking C and C++ object code files into shared libraries.
    script << "rule LinkCLib\n"
              "  command = " << cCompilerPath << " -shared -o $out $in $ldFlags\n"
              "\n";

    script << "rule LinkCxxLib\n"
              "  command = " << cxxCompilerPath << " -shared -o $out $in $ldFlags\n"
              "\n";

    script << "rule LinkCExe\n"
              "  command = " << cCompilerPath << " -o $out $in $ldFlags\n"
              "\n";

    script << "rule LinkCxxExe\n"
              "  command = " << cxxCompilerPath << " -o $out $in $ldFlags\n"
              "\n";

    // Generate a rule for running ifgen.
    script << "rule GenInterfaceCode\n"
              "  command = mkdir -p $outputDir && ifgen --output-dir $outputDir $ifgenFlags $in"
              "\n"
              "\n";

    // Generate a rule for re-building the build.ninja script when it is out of date.
    script << "rule RegenNinjaScript\n"
              "  generator = 1\n"
              "  command = " << argv[0] << " --dont-run-ninja";
    for (int i = 1; i < argc; i++)
    {
        script << " \"" << argv[i] << '"';
    }
    script << "\n\n";
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
        script << ":$$LEGATO_BUILD/bin/lib";
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
 * Print to a given build script a rule for building the interface header file for a given
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
 * Print to a given build script a rule for building the interface header file for a given
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
 * Print to a given build script a rule for building the header file for a given types-only
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
 * Add to a given set the paths for all the client-side interface .h files generated for all
 * .api files that a given .api file includes through USETYPES statements.
 *
 * @note These paths are all relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GetClientUsetypesApiHeaders
(
    std::set<std::string>& apiHeaders,
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto includedApiPtr : apiFilePtr->includes)
    {
        apiHeaders.insert(includedApiPtr->GetClientInterfaceFile(includedApiPtr->defaultPrefix) );

        // Recurse.
        GetClientUsetypesApiHeaders(apiHeaders, includedApiPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a rule for building the object file for a given client-side API
 * interface.
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
    if (generatedSet.find(ifPtr->objectFile) == generatedSet.end())
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
        GetClientUsetypesApiHeaders(apiHeaders, ifPtr->apiFilePtr);

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
 * Add to a given set the paths for all the server-side interface .h files generated for all
 * .api files that a given .api file includes through USETYPES statements.
 *
 * @note These paths are all relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GetServerUsetypesApiHeaders
(
    std::set<std::string>& apiHeaders,
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto includedApiPtr : apiFilePtr->includes)
    {
        apiHeaders.insert(includedApiPtr->GetServerInterfaceFile(includedApiPtr->defaultPrefix) );

        GetServerUsetypesApiHeaders(apiHeaders, includedApiPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a rule for building the object file for a given server-side API
 * interface.
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
    if (generatedSet.find(ifPtr->objectFile) == generatedSet.end())
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
        GetServerUsetypesApiHeaders(apiHeaders, ifPtr->apiFilePtr);

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



} // namespace ninja
