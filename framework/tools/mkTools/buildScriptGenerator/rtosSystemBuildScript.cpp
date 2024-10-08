//--------------------------------------------------------------------------------------------------
/**
 * @file rtosSystemBuildScript.cpp
 *
 * Implementation of the build script generator for Rtos-based systems.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "rtosSystemBuildScript.h"

namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * There are no system-specific build steps on RTOS
 */
//--------------------------------------------------------------------------------------------------
void RtosSystemBuildScriptGenerator_t::GenerateSystemBuildRules
(
    model::System_t* systemPtr
)
{
    // Create Legato system staging directory.  All apps directories are
    // included directly in the Legato system directory instead of being
    // symlinks.
    script <<
        "rule StageLegato\n"
        "  description = Creating Legato system staging directory\n"
        "  command = $\n"
        "    set -e; $\n"
        "    app_staging=$$(dirname $out)/staging/apps; $\n"
        "    mkdir -p $$app_staging; $\n"
        "    for app_touch in $in; do $\n"
        "      app_dir=$$(dirname $$app_touch); $\n"
        "      app_name=$$(basename $$app_dir); $\n"
        "      [ ! -d $$app_dir/staging ]"
               " || cp -rfT $$app_dir/staging $$app_staging/$$app_name; $\n"
        "    done; $\n"
        "    touch $out\n"
        "\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate linker flags needed for linking an RTOS system.
 */
//--------------------------------------------------------------------------------------------------
void RtosSystemBuildScriptGenerator_t::GenerateLdFlags
(
    void
)
{
    // Link with the Legato runtime library.
    script <<
        "\n"
        "  ldFlags = ";

    // For each component in the system.
    for (auto& componentEntry : model::Component_t::GetComponentMap())
    {
        auto componentPtr = componentEntry.second;

        // Add the ldflags from the Component.cdef file.
        for (auto& arg : componentPtr->ldFlags)
        {
            script << " " << arg;
        }
    }

    script << " $ldFlags";

    if (buildParams.compilerType == mk::BuildParams_t::COMPILER_ARM_RVCT)
    {
        script << " $$LEGATO_BUILD/framework/lib-static/liblegato.a\n";
    }
    else
    {
        script << " -Wl,-Map=" <<
            path::MakeAbsolute(path::Combine(buildParams.outputDir, "$target.map")) <<
            " -Wl,--gc-sections \"-L$$LEGATO_BUILD/framework/lib-static\" -llegato ";
        if (envVars::GetConfigBool("LE_CONFIG_MEM_HIBERNATION"))
        {
            script << " -T $builddir/src/legato.ld\n";
        }
        else
        {
            script << "\n";
        }
    }

    script <<
        "  entry=le_microSupervisor_Main\n"
        "  pplFlags=--entry=le_microSupervisor_Main\n"
        "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Pack build into a single file.
 *
 * On RTOS this generates a single file containing all the binaries needed to build an RTOS
 * system.
 */
//--------------------------------------------------------------------------------------------------
void RtosSystemBuildScriptGenerator_t::GenerateSystemPackBuildStatement
(
    model::System_t* systemPtr
)
{
    auto outputFile = path::MakeAbsolute(path::Combine(buildParams.outputDir,
                                                       "$target.o"));
    auto tasksOutputFile = "$builddir/obj/tasks.c.o";
    auto rpcServicesOutputFile = "$builddir/obj/rpcServices.c.o";
    std::string archiveStr = tasksOutputFile, spaceStr = " ";
    auto outputArFile = path::MakeAbsolute(path::Combine(buildParams.outputDir, "$target.a"));

    // Build task list file
    script << "build " << tasksOutputFile << ":"
              "  CompileC " << path::Combine(buildParams.workingDir, "src/tasks.c") << "\n"
              "    cFlags = $cFlags -I$$LEGATO_ROOT/framework/daemons/rtos/microSupervisor";

    if (envVars::GetConfigBool("LE_CONFIG_FILEID"))
    {
        script << " -D__FILEID__=" << 2;
    }

    script << "\n\n";

    if (envVars::GetConfigBool("LE_CONFIG_RPC"))
    {
        // Build rpc services file
        script << "build " << rpcServicesOutputFile << ":"
                  "  CompileC " << path::Combine(buildParams.workingDir, "src/rpcServices.c");

        // Create a set of header files that need to be generated for all IPC API interfaces.
        std::list<std::string> interfaceHeaders;

        // Traverse all External Server Interfaces and add them into the
        // interface headers list
        for (auto &serverEntry : systemPtr->externServerInterfaces)
        {
            auto componentPtr = serverEntry.second->ifPtr->componentPtr;

            if (componentPtr->HasCOrCppCode())
            {
                componentGeneratorPtr->GetCInterfaceHeaders(interfaceHeaders, componentPtr);
            }
            else if (componentPtr->HasJavaCode())
            {
                componentGeneratorPtr->GetJavaInterfaceFiles(interfaceHeaders, componentPtr);
            }
        }

        // Traverse all External Client Interfaces and add them into the
        // interface headers list
        for (auto &serverEntry : systemPtr->externClientInterfaces)
        {
            auto componentPtr = serverEntry.second->ifPtr->componentPtr;

            if (componentPtr->HasCOrCppCode())
            {
                componentGeneratorPtr->GetCInterfaceHeaders(interfaceHeaders, componentPtr);
            }
            else if (componentPtr->HasJavaCode())
            {
                componentGeneratorPtr->GetJavaInterfaceFiles(interfaceHeaders, componentPtr);
            }
        }

        if (!interfaceHeaders.empty())
        {
            // Generate a dependency statement for each interface header
            script << " || ";
            std::copy(interfaceHeaders.begin(), interfaceHeaders.end(),
                      std::ostream_iterator<std::string>(script, " "));
        }

        script << "\n";
        script << "    cFlags = $cFlags -I$$LEGATO_ROOT/framework/daemons/rpcProxy/rpcDaemon"
                  " -I$$LEGATO_ROOT/framework/liblegato";

        std::set<const model::ApiFile_t*> useTypesApis;
        for (auto &serverEntry : systemPtr->externServerInterfaces)
        {
            auto apiFilePtr = serverEntry.second->ifPtr->apiFilePtr;

            script << " -I$builddir/" << apiFilePtr->codeGenDir;

            apiFilePtr->GetUsetypesApis(useTypesApis);
        }
        for (auto &clientEntry : systemPtr->externClientInterfaces)
        {
            auto apiFilePtr = clientEntry.second->ifPtr->apiFilePtr;

            script << " -I$builddir/" << apiFilePtr->codeGenDir;

            apiFilePtr->GetUsetypesApis(useTypesApis);
        }
        for (auto apiFilePtr : useTypesApis)
        {
            script << " -I$builddir/" << apiFilePtr->codeGenDir;
        }

        script << "\n"
            "\n";

    } // End of LE_CONFIG_RPC

    // And link everything together into a system file.  This system file exports only one
    // symbol -- the entry point of the microSupervisor
    script << "build " << outputFile << ": PartialLink "
           << path::Combine(envVars::Get("LEGATO_ROOT"),
                            "build/$target/framework/lib/microSupervisor.o")
           << " "
           << tasksOutputFile;

    // Link the rpcServicesOutputFile into the system if RPC is enabled
    if (envVars::GetConfigBool("LE_CONFIG_RPC"))
    {
        script << " " << rpcServicesOutputFile;
        archiveStr += spaceStr + rpcServicesOutputFile;
    }

    // For each app built by the mk tools for this system,
    for (auto& appEntry : systemPtr->apps)
    {
        auto appPtr = appEntry.second;

        // Add the app's executables' files.
        for (auto& exeEntry : appPtr->executables)
        {
            auto exePtr = exeEntry.second;

            script << " $builddir/" << exePtr->path;

            archiveStr += " $builddir/" + exePtr->path;
        }
    }

    // Track common API files
    std::set<std::string> commonClientObjects;

    // For each component in the system.
    for (auto& componentEntry : model::Component_t::GetComponentMap())
    {
        auto componentPtr = componentEntry.second;

        // Add all the component's partially linked object file.
        if (componentPtr->HasCOrCppCode())
        {
            script << " " << componentPtr->GetTargetInfo<target::RtosComponentInfo_t>()->staticlib;
            archiveStr += spaceStr + componentPtr->GetTargetInfo<target::RtosComponentInfo_t>()->staticlib;
        }

        // Also includes all the object files for the auto-generated IPC API client
        // code for the component's required APIs, if file is not already included
        // in the link
        componentGeneratorPtr->GetCommonApiFiles(componentPtr, commonClientObjects);
    }

    for (auto const &commonClientObject : commonClientObjects)
    {
        script << " $builddir/" << commonClientObject;
        archiveStr += " $builddir/" + commonClientObject;
    }

    script << " | $builddir/src/legato.ld";

    GenerateLdFlags();

    // Output the rule for archiving legato system objective files
    script << "build " << outputArFile << ": ArchiveOBJ " << archiveStr << "\n\n";

    // Pack each app in the system into the RFS image.  On RTOS RFS only includes
    // bundled files, not libraries or executables
    script <<
        "build $builddir/.staging.tag: StageLegato";
    for (auto& appEntry : systemPtr->apps)
    {
        auto appPtr = appEntry.second;

        script << " " << path::Combine("$builddir",
                                       path::Combine(appPtr->workingDir, ".staging.tag"));
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate build script for the system.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRtos
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    RtosSystemBuildScriptGenerator_t systemGenerator(filePath, buildParams);

    systemGenerator.Generate(systemPtr);
}

} // namespace ninja
