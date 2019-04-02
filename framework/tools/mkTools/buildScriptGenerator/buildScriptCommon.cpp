//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptCommon.cpp
 *
 * Implementation of common functions shared by the build script generators.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include <dirent.h>

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
        std::cout << mk::format(LE_I18N("Generating ninja build script: '%s'."), filePath)
                  << std::endl;
    }

    file::MakeDir(path::GetContainingDir(filePath));

    script.open(filePath, std::ofstream::trunc);
    if (!script.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), filePath)
        );
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
        throw mk::Exception_t(LE_I18N("Failed to close file."));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Escape ninja special characters (e.g. $) within a string.
 */
//--------------------------------------------------------------------------------------------------
std::string EscapeString
(
    const std::string &inputString
)
//--------------------------------------------------------------------------------------------------
{
    std::string escapedString;

    for (char c: inputString)
    {
        if (c == '$')
        {
            escapedString.push_back('$');
        }

        escapedString.push_back(c);
    }

    return escapedString;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a build script file
 */
//--------------------------------------------------------------------------------------------------
BuildScriptGenerator_t::BuildScriptGenerator_t
(
    const std::string scriptPath,
    const mk::BuildParams_t& buildParams
)
:
buildParams(buildParams),
scriptPath(scriptPath)
{
    OpenFile(script, scriptPath, buildParams.beVerbose);
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the build script file after generator is finished.
 */
//--------------------------------------------------------------------------------------------------
BuildScriptGenerator_t::~BuildScriptGenerator_t
(
)
{
    CloseFile(script);
}

//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ifgenFlags variable definition.
 **/
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateIfgenFlags
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Add the interface search directories to ifgen's command-line.
    for (const auto& dir : buildParams.interfaceDirs)
    {
        script << " --import-dir " << dir;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write out to a given script file a space-separated list of paths to all the .api files needed
 * by a given .api file (specified through USETYPES statements in the .api files).
 **/
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateIncludedApis
(
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto includedApiPtr : apiFilePtr->includes)
    {
        script << " " << includedApiPtr->path;

        // Recurse.
        GenerateIncludedApis(includedApiPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate C flags.
 *
 * Generates flags for the C compiler.  These flags are also used for the C++ compiler.
 */
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateCFlags
(
    void
)
{
    const std::string& target = buildParams.target;
    std::string sysrootOption;

    if (!buildParams.sysrootDir.empty())
    {
        sysrootOption = "--sysroot=" + buildParams.sysrootDir;
    }

    script << " " << sysrootOption <<
              " -MMD -MF $out.d -c $in -o $out"
              " -DLE_FILENAME=`basename $in`" // Define the file name for the log macros.
              " -Wall" // Enable all warnings.
              " -Werror" // Treat all warnings as errors.
              " -fvisibility=hidden" // Prevent exporting of symbols by default.
              " -DMK_TOOLS_BUILD"; // Indicate build is being done by the mk tools.
    if (target != "localhost")
    {
        script << "  -DLEGATO_EMBEDDED";    // Indicate target is an embedded device (not a PC).
    }
    if (!buildParams.debugDir.empty())
    {
        script << " -g";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the PATH environment variable declaration.
 **/
//--------------------------------------------------------------------------------------------------
std::string BuildScriptGenerator_t::GetPathEnvVarDecl
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    std::string pathStr = "PATH=\"$${LEGATO_ROOT}/bin";

    for (const auto &crossToolPath : buildParams.crossToolPaths)
    {
        pathStr += ":" + crossToolPath;
    }

    pathStr += ":$$PATH\"";

    return pathStr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate generic build rules.
 **/
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateBuildRules
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& cCompilerPath = buildParams.cCompilerPath;
    const std::string& cxxCompilerPath = buildParams.cxxCompilerPath;
    const std::string& compilerCachePath = buildParams.compilerCachePath;
    const std::string& preProcessorPath = buildParams.cppPath;
    std::string sysrootOption;
    std::string crossToolPath;

    if (!buildParams.sysrootDir.empty())
    {
        sysrootOption = "--sysroot=" + buildParams.sysrootDir;
    }

    if (!cCompilerPath.empty())
    {
        crossToolPath = path::GetContainingDir(cCompilerPath);

        // "." is not valid for our purposes.
        if (crossToolPath == ".")
        {
            crossToolPath = "";
        }
    }

    // Generate rule for compiling a C source code file.
    script << "rule CompileC\n"
              "  description = Compiling C source\n"
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << compilerCachePath << " " << cCompilerPath;
    GenerateCFlags();
    script << " $cFlags" // Include user-provided CFLAGS last so other settings can be overridden.
              "\n\n";

    // Generate rule for compiling a C++ source code file.
    script << "rule CompileCxx\n"
              "  description = Compiling C++ source\n"
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << compilerCachePath << " " << cxxCompilerPath;
    GenerateCFlags();
    script << " $cxxFlags" // Include user-provided CXXFLAGS last so
                           // other settings can be overridden
              "\n\n";

    script << "rule ProcessConfig\n"
        << "  description = Merging config file\n"
        "  depfile = $in.d\n" // Tell ninja where gcc will put the dependencies.
        "  command = " << preProcessorPath << " -MMD -MF $in.d -MQ $out"
                     " -P -I $builddir -E $in -o $out\n\n";

    // Generate rules for running external tools
    script << "rule BuildExternal\n"
              "  description = Running external build step\n"
              "  command = cd $builddir/$workingdir && $\n"
              "            export CFLAGS=\"" << sysrootOption << " $cFlags\" $\n"
              "            CXXFLAGS=\"" << sysrootOption << " $cxxFlags\" $\n"
              "            LDFLAGS=\"" << sysrootOption << " $ldFlags\" $\n"
              "            CPPFLAGS=\"" << sysrootOption << " $cppFlags\" $\n"
              "            " << GetPathEnvVarDecl() << " $\n"
              "            && $\n"
              "            $externalCommand\n"
              "\n";

    // Generate a rule for running ifgen.
    script << "rule GenInterfaceCode\n"
              "  description = Generating IPC interface code\n"
              "  command = ifgen --output-dir $outputDir $ifgenFlags $in\n"
              "\n";

    // Generate a rule for generating a Python C Extension .c file for an API
    script << "rule GenPyApiCExtension\n"
              "  description = Generating Python API C Extension\n"
              "  command = " << "cextgenerator.py $in -o $workDir > /dev/null\n";

    // Generate a rule for copying a file.
    script << "rule CopyFile\n"
              "  description = Copying file\n"
              "  command = cp -d -f -T $in $out\n"
              "\n";

    // Generate a rule for re-building the build.ninja script when it is out of date.
    script << "rule RegenNinjaScript\n"
              "  description = Regenerating build script\n"
              "  generator = 1\n"
              "  command = " << buildParams.argv[0] << " --dont-run-ninja";
    for (int i = 1; i < buildParams.argc; i++)
    {
        if (strcmp(buildParams.argv[i], "--dont-run-ninja") != 0)
        {
            script << " \"" << buildParams.argv[i] << '"';
        }
    }
    script << "\n"
              "\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    const std::set<std::string>& dependencies
)
{
    script << "build " << scriptPath << ": RegenNinjaScript |";

    // Write the dependencies to the script.
    for (auto const &dep : dependencies)
    {
        script << " " << dep;
    }
    script << "\n\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a permission string for chmod based on the permissions we want to set on the target
 * file.
 **/
//--------------------------------------------------------------------------------------------------
std::string BuildScriptGenerator_t::PermissionsToModeFlags
(
    model::Permissions_t permissions  ///< The permission flags to set on the given file(s).
)
//--------------------------------------------------------------------------------------------------
{
    std::string flags;
    std::string executableFlag = (permissions.IsExecutable()?"+x":"-x");

    flags = "u+rw" + executableFlag +
            ",g+r" + executableFlag +
            ",o" + executableFlag;

    if (permissions.IsReadable())
    {
        flags += "+r";
    }
    else
    {
        flags += "-r";
    }

    if (permissions.IsWriteable())
    {
        flags += "+w";
    }
    else
    {
        flags += "-w";
    }

    return flags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 *
 * Adds the absolute destination file path to the bundledFiles set.
 **/
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateFileBundleBuildStatement
(
    const model::FileSystemObject_t& fileObject,  ///< File object to generate
    model::FileSystemObjectSet_t& bundledFiles    ///< Set to fill with bundled file paths.
)
//--------------------------------------------------------------------------------------------------
{
    auto bundledFileIter = bundledFiles.find(fileObject);

    // If the same file is already in the list, don't insert it again.
    if (bundledFileIter == bundledFiles.end())
    {
        script << "build " << fileObject.destPath << " : BundleFile " << fileObject.srcPath << "\n"
               << "  modeFlags = " << PermissionsToModeFlags(fileObject.permissions) << "\n";

        bundledFiles.insert(fileObject);
    }
    // We assume that files bundled from ADEF are added first:
    // as required by documentation, they take precedence over files bundled from SDEF.
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling files from a directory into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
void BuildScriptGenerator_t::GenerateDirBundleBuildStatements
(
    const model::FileSystemObject_t& fileObject, ///< File system object to bundle
    model::FileSystemObjectSet_t& bundledFiles   ///< Set to fill with bundled file paths.
)
//--------------------------------------------------------------------------------------------------
{
    // Attempt to open the source as a directory stream.
    DIR* dir = opendir(fileObject.srcPath.c_str());
    if (dir == NULL)
    {
        // If failed for some reason other than this just not being a directory,
        if (errno != ENOTDIR)
        {
            int err = errno;
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("Can't access file or directory '%s' (%s)"),
                           fileObject.srcPath,
                           strerror(err))
            );
        }
        // If the source is not a directory,
        else
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("Not a directory: '%s'."), fileObject.srcPath)
            );
        }
    }

    // Loop over list of directory contents.
    for (;;)
    {
        // Setting errno so as to be able to detect errors from end of directory
        // (as recommended in the documentation).
        errno = 0;

        // Read an entry from the directory.
        struct dirent* entryPtr = readdir(dir);

        if (entryPtr == NULL)
        {
            if (errno != 0)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: readdir() failed.  Errno = %s"),
                               strerror(errno))
                );
            }
            else
            {
                // Hit end of the directory.  Nothing more to do.
                break;
            }
        }
        // Skip "." and ".."
        else if ((strcmp(entryPtr->d_name, ".") != 0) && (strcmp(entryPtr->d_name, "..") != 0))
        {
            auto entrySrcPath = path::Combine(fileObject.srcPath, entryPtr->d_name);
            auto entryDestPath = path::Combine(fileObject.destPath, entryPtr->d_name);

            // If this is a directory, then recursively descend into it.
            if (file::DirectoryExists(entrySrcPath))
            {
                GenerateDirBundleBuildStatements(model::FileSystemObject_t(entrySrcPath,
                                                                           entryDestPath,
                                                                           fileObject.permissions,
                                                                           &fileObject),
                                                 bundledFiles);
            }
            // If this is a file, create a build statement for it.
            else if (file::FileExists(entrySrcPath))
            {
                GenerateFileBundleBuildStatement(model::FileSystemObject_t(entrySrcPath,
                                                                           entryDestPath,
                                                                           fileObject.permissions,
                                                                           &fileObject),
                                                 bundledFiles);
            }
            // If this is anything else, we don't support it.
            else
            {
                fileObject.parseTreePtr->ThrowException(
                    mk::format(LE_I18N("File system object is not a directory or a file: '%s'."),
                               entrySrcPath)
                );
            }
        }
    }

    closedir(dir);
}

} // namespace ninja
