//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptRtos.cpp
 *
 * Implementation of functions specific to Rtos build scripts.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptRtos.h"


namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Rtos-specific flags
 **/
//--------------------------------------------------------------------------------------------------
void RtosBuildScriptGenerator_t::GenerateIfgenFlags
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // On RTOS, generate local services always
    script << " --local-service";

    // Then call base to generate the rest of the flags.
    BuildScriptGenerator_t::GenerateIfgenFlags();
}


//--------------------------------------------------------------------------------------------------
/**
 * Rtos-specific build rules.
 **/
//--------------------------------------------------------------------------------------------------
void RtosBuildScriptGenerator_t::GenerateBuildRules
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const std::string& cCompilerPath = buildParams.cCompilerPath;
    std::string crossToolPath;

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

    // Generate rules for partial linking C and C++ object code files.
    script << "rule PartialLink\n"
              "  description = Linking Subsystem\n"
              "  command = " << cCompilerPath;
    if (!buildParams.debugDir.empty())
    {
        script << " -g";
    }
    script << " -Wl,-r -nostdlib -Wl,--entry=$entry -o $out $in $ldFlags && $\n"
              "            rename-hidden-symbols $pplFlags $out\n"
              "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * RTOS-specific C flags.
 */
//--------------------------------------------------------------------------------------------------
void RtosBuildScriptGenerator_t::GenerateCFlags
(
    void
)
{
    BuildScriptGenerator_t::GenerateCFlags();

    // Generate per-data & per-function sections so these can be removed if not referenced.
    // ELF file generated is larger, but final link will be smaller when combined with
    // --gc-sections linker flag.
    script << " -fdata-sections -ffunction-sections";
}


} // namespace ninja
