//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptLinux.cpp
 *
 * Implementation of functions specific to Linux build scripts.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptLinux.h"


namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Generate Linux C flags.
 *
 * Linux C flags add -fPIC to the generic C flags
 */
//--------------------------------------------------------------------------------------------------
void LinuxBuildScriptGenerator_t::GenerateCFlags
(
    void
)
{
    BuildScriptGenerator_t::GenerateCFlags();

    script << " -fPIC";
}

//--------------------------------------------------------------------------------------------------
/**
 * Linux-specific build rules.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxBuildScriptGenerator_t::GenerateBuildRules
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& cCompilerPath = buildParams.cCompilerPath;
    const std::string& cxxCompilerPath = buildParams.cxxCompilerPath;
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

    // First generate common build rules
    BuildScriptGenerator_t::GenerateBuildRules();

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
    if (!buildParams.noPie)
    {
      script << " -fPIE -pie";
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
    if (!buildParams.noPie)
    {
      script << " -fPIE -pie";
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
              "  depfile = $out.d\n" // Tell ninja where gcc will put the dependencies.
              "  command = make -C $in\n"
              "\n";
}

} // namespace ninja
