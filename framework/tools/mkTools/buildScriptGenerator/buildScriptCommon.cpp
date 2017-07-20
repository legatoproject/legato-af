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
void BuildScriptGenerator_t::GenerateIfgenFlagsDef
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    script << "ifgenFlags = ";

    // Add the interface search directories to ifgen's command-line.
    for (const auto& dir : buildParams.interfaceDirs)
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
void BuildScriptGenerator_t::GenerateBuildRules
(
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& target = buildParams.target;
    const std::string& cCompilerPath = buildParams.cCompilerPath;
    const std::string& cxxCompilerPath = buildParams.cxxCompilerPath;
    std::string sysrootOption;
    std::string crossToolPath;

    if (!buildParams.sysrootPath.empty())
    {
        sysrootOption = "--sysroot=" + buildParams.sysrootPath;
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
              "  command = " << cCompilerPath << " " << sysrootOption <<
              " -MMD -MF $out.d -c $in -o $out"
              " -DLE_FILENAME=`basename $in`" // Define the file name for the log macros.
              " -Wall" // Enable all warnings.
              " -fPIC" // Compile to position-independent code for linking into a shared library.
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
    script << " $cFlags" // Include user-provided CFLAGS last so other settings can be overridden.
              "\n\n";

    // Generate rule for compiling a C++ source code file.
    script << "rule CompileCxx\n"
              "  description = Compiling C++ source\n"
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = " << cxxCompilerPath << " " << sysrootOption <<
              " -MMD -MF $out.d -c $in -o $out"
              " -DLE_FILENAME=`basename $in`" // Define the file name for the log macros.
              " -Wall" // Enable all warnings.
              " -fPIC" // Compile to position-independent code for linking into a shared library.
              " -Werror" // Treat all warnings as errors.
              " -fvisibility=hidden " // Prevent exporting of symbols by default.
              " -DMK_TOOLS_BUILD"; // Indicate build is being done by the mk tools.
    if (target != "localhost")
    {
        script << "  -DLEGATO_EMBEDDED";    // Indicate target is an embedded device (not a PC).
    }
    if (!buildParams.debugDir.empty())
    {
        script << " -g";
    }
    script << " $cxxFlags" // Include user-provided CXXFLAGS last so
                           // other settings can be overridden
              "\n\n";

    // Generate rules for linking C and C++ object code files into shared libraries.
    script << "rule LinkCLib\n"
              "  description = Linking C library\n"
              "  command = " << cCompilerPath << " " << sysrootOption;
    if (!buildParams.debugDir.empty())
    {
        script << " -Wl,--build-id -g";
    }
    script << " -shared -o $out $in $ldFlags";
    if (!buildParams.debugDir.empty())
    {
        script << " $\n"
                  "      && splitdebug -d " << buildParams.debugDir <<
                  " $out";
    }
    script << "\n\n";

    script << "rule LinkCxxLib\n"
              "  description = Linking C++ library\n"
              "  command = " << cxxCompilerPath << " " << sysrootOption;
    if (!buildParams.debugDir.empty())
    {
        script << " -Wl,--build-id -g";
    }
    script << " -shared -o $out $in $ldFlags";
    if (!buildParams.debugDir.empty())
    {
        script << " $\n"
                  "      && splitdebug -d " << buildParams.debugDir <<
                  " $out";
    }
    script << "\n\n";

    script << "rule LinkCExe\n"
              "  description = Linking C executable\n"
              "  command = " << cCompilerPath << " " << sysrootOption;
    if (!buildParams.debugDir.empty())
    {
        script << " -Wl,--build-id -g";
    }
    script << " -o $out $in $ldFlags";
    if (!buildParams.debugDir.empty())
    {
        script << " -g $\n"
                  "      && splitdebug -d " << buildParams.debugDir <<
                  " $out";
    }
    script << "\n\n";

    script << "rule LinkCxxExe\n"
              "  description = Linking C++ executable\n"
              "  command = " << cxxCompilerPath << " " << sysrootOption;
    if (!buildParams.debugDir.empty())
    {
        script << " -Wl,--build-id -g";
    }
    script << " -o $out $in $ldFlags";
    if (!buildParams.debugDir.empty())
    {
        script << " -g $\n"
                  "      && splitdebug -d " << buildParams.debugDir <<
                  " $out";
    }
    script << "\n\n";

    // Generate rules for compiling Java code.
    script << "rule CompileJava\n"
              "  description = Compiling Java source\n"
              "  command = javac -cp $classPath -d `dirname $out` $in && touch $out\n"
              "\n";

    script << "rule MakeJar\n"
              "  description = Making JAR file\n"
              "  command = INDIR=`dirname $in`; find $$INDIR -name '*.class' "
                             "-printf \"-C $$INDIR\\n%P\\n\""
                             "|xargs jar -cf $out\n"
              "\n";

    // Generate rules for building drivers.
    script << "rule MakeKernelModule\n"
              "  description = Build kernel driver module\n"
              "  command = make -C $in\n"
              "\n";

    // Generate rules for running external tools
    script << "rule BuildExternal\n"
              "  description = Running external build step\n"
              "  command = cd $builddir/$workingdir; $\n"
              "            env CFLAGS=\"" << sysrootOption << " $cFlags\" $\n"
              "            CXXFLAGS=\"" << sysrootOption << " $cxxFlags\" $\n"
              "            LDFLAGS=\"" << sysrootOption << " $ldFlags\" $\n";
    if (!crossToolPath.empty())
    {
        script << "            PATH=\"" << crossToolPath << ":$$PATH\" $\n";
    }
    script << "            sh -c \'$externalCommand\'\n";

    // Generate a rule for running ifgen.
    script << "rule GenInterfaceCode\n"
              "  description = Generating IPC interface code\n"
              "  command = ifgen --output-dir $outputDir $ifgenFlags $in\n"
              "\n";

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
    for (auto dep : dependencies)
    {
        script << " " << dep;
    }
    script << "\n\n";
}


} // namespace ninja
