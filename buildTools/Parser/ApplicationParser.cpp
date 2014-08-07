//--------------------------------------------------------------------------------------------------
/**
 * Main file for the Application Parser.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "../ComponentModel/LegatoObjectModel.h"
#include "Parser.h"

extern "C"
{
    #include "ApplicationParser.tab.h"
    #include "ParserCommonInternals.h"
    #include "ApplicationParserInternals.h"
    #include <string.h>
    #include <stdio.h>
    #include "lex.ayy.h"
}


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
int ayy_IsVerbose = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Build parameters received via the command line.
 */
//--------------------------------------------------------------------------------------------------
static const legato::BuildParams_t* BuildParamsPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the Application object for the application that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::App* AppPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Executable object for the executable currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::Executable* ExePtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Process Environment object for the processes: section currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::ProcessEnvironment* ProcEnvPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Process object for the run: subsection currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::Process* ProcessPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Set the application-wide unique name ("exe.componentInstance.interface") for an interface.
 **/
//--------------------------------------------------------------------------------------------------
static void SetInterfaceAppUniqueName
(
    legato::Interface& interface,
    const legato::Executable& exe,
    const legato::ComponentInstance& componentInstance
)
//--------------------------------------------------------------------------------------------------
{
    interface.AppUniqueName(exe.CName()
                            + '.'
                            + componentInstance.Name()
                            + '.'
                            + interface.ServiceInstanceName() );
}


//--------------------------------------------------------------------------------------------------
/**
 * Tries to apply a framework API auto-binding on a given interface.
 *
 * @return true if the binding was applied.
 **/
//--------------------------------------------------------------------------------------------------
static bool TryFrameworkApiAutoBind
(
    legato::ImportedInterface& interface,
    const std::string& frameworkServiceName
)
//--------------------------------------------------------------------------------------------------
{
    if (interface.Api().Name() == frameworkServiceName)
    {
        interface.MakeExternal(frameworkServiceName);

        if (BuildParamsPtr->IsVerbose())
        {
            std::cout << "    Auto-binding required API '" << frameworkServiceName
                      << "' to framework service '<root>." << frameworkServiceName << "'."
                      << std::endl;
        }

        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply an automatic binding if this is one of the framework APIs, such as the Watchdog API
 * or the Config API.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyFrameworkApiAutoBind
(
    legato::ImportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    // Don't apply this if the API has already been declared an external interface.
    // We don't want to prevent people from doing something different than the default auto-bind.
    if (!interface.IsExternal())
    {
        if (TryFrameworkApiAutoBind(interface, "le_wdog"))
        {
            AppPtr->SetUsesWatchdog();
        }
        else if (TryFrameworkApiAutoBind(interface, "le_cfg"))
        {
            AppPtr->SetUsesConfig();
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply an internal bind by changing the service name of the client-side interface to match
 * the service name that the server-side interface will advertise itself as.
 *
 * @throw legato::Exception if there is an internal binding for this interface, but this interface
 *        has already been bound to something.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyInternalBind
(
    legato::ImportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    const auto& internalApiBindMap = AppPtr->InternalApiBinds();

    const auto bindIter = internalApiBindMap.find(interface.AppUniqueName());
    if (bindIter != internalApiBindMap.cend())
    {
        const auto& bind = bindIter->second;

        if (interface.IsBound())
        {
            throw legato::Exception("Client-side interface '" + interface.AppUniqueName() + "'"
                                    " has been bound more than once.");
        }
        // If the interface is supposed to be one of the app's external interfaces,
        // but it has also been bound to something inside this app, report an error.
        else if (interface.IsExternal())
        {
            std::string errorMsg = "Client-side (required) interface '"
                                   + interface.AppUniqueName() + "' has been declared an"
                                   " external (inter-app) required interface"
                                   " (in a \"requires: api:\" section in the .adef)"
                                   " but has also been explicitly bound to a"
                                   " server-side interface inside the app.";
            ayy_error(errorMsg.c_str());
        }

        const auto& server = AppPtr->FindServerInterface(bind.ServerInterface());

        interface.ServiceInstanceName(server.ServiceInstanceName());
        interface.MarkBound();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply an external bind by changing the service name of the client-side interface to match
 * the service name that the server-side interface will advertise itself as.
 *
 * @throw legato::Exception if there is an external binding for this interface, but this interface
 *        has already been bound to something.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyExternalBind
(
    legato::ImportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    const auto& explicitBindMap = AppPtr->ExternalApiBinds();

    const auto bindIter = explicitBindMap.find(interface.AppUniqueName());
    if (bindIter != explicitBindMap.cend())
    {
        const auto& bind = bindIter->second;

        if (interface.IsBound())
        {
            throw legato::Exception("Client-side interface '" + interface.AppUniqueName() + "'"
                                    " has been bound more than once.");
        }

        interface.ServiceInstanceName(bind.ServerServiceName());
        interface.MarkBound();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the binding lists to see if a given client-side interface has been explicitly bound
 * to something, and if so, apply the effects of that binding on the interface.
 *
 * @throw legato::Exception if an error is detected.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyExplicitBind
(
    legato::ImportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    ApplyInternalBind(interface);
    ApplyExternalBind(interface);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do final processing of the application's object model.
 *
 * @note ayy_error() will be called if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
static void FinalizeApp
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Go through all the IPC API interfaces of all the executables and
    //
    // 1. Prefix their service names with the exe name and component instance name to prevent
    //    naming conflicts and accidental mis-bindings.
    //
    // 2. Update their service names according to any bindings, so that generated IPC code will
    //    use the correct service names when opening services.
    //
    // 3. Check that all client-side (required) interfaces have either been bound to something
    //    or declared an inter-app interface.
    //
    // 4. Make sure each "required" external client-side interface is not also explicitly
    //    bound internally.
    //

    try
    {
        // For each executable,
        for (auto& exeMapEntry : AppPtr->Executables())
        {
            auto& exe = exeMapEntry.second;

            // For each component instance in the executable,
            for (auto& componentInstance : exe.ComponentInstances())
            {
                // For each provided (server-side) interface,
                for (auto& interfaceMapEntry : componentInstance.ProvidedApis())
                {
                    auto& interface = interfaceMapEntry.second;

                    // Add the "exe.component." prefix to the interface's service name.
                    SetInterfaceAppUniqueName(interface, exe, componentInstance);
                }

                // For each required (client-side) interface in the component instance,
                for (auto& interfaceMapEntry : componentInstance.RequiredApis())
                {
                    auto& interface = interfaceMapEntry.second;

                    // If this is an auto-bound framework API, such as the Watchdog API
                    // or the Config API, then do the auto-binding now.
                    ApplyFrameworkApiAutoBind(interface);

                    // Add the "exe.component." prefix to the interface's service name.
                    SetInterfaceAppUniqueName(interface, exe, componentInstance);

                    // Apply any binding that may exist for this interface so that the generated
                    // code will try to use the correct service name when it opens the service.
                    ApplyExplicitBind(interface);

                    // If the interface is not satisfied (either bound to something or declared
                    // an external interface that needs to be bound in a .sdef), generate an error.
                    // Note: we don't need to worry about APIs that we only use the typedefs from.
                    if (!interface.IsSatisfied() && !interface.TypesOnly())
                    {
                        throw legato::Exception("Client-side (required) interface '"
                                                + interface.AppUniqueName() + "' is unsatisfied. "
                                                " It has not been declared an external (inter-app)"
                                                " required interface (in a \"requires: api:\""
                                                " section in the .adef) and has not been bound to"
                                                " any service (in the \"bindings:\" section of the"
                                                " .adef).");
                    }
                }
            }
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


namespace legato
{

namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Parses an Application Definition (.adef) and populates an App object with the information
 * garnered.
 *
 * @note    Expects the Application's definition (.adef) file path to be set.
 */
//--------------------------------------------------------------------------------------------------
void ParseApp
(
    App* appPtr,                        ///< The object to be populated.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
)
//--------------------------------------------------------------------------------------------------
{
    AppPtr = appPtr;
    BuildParamsPtr = &buildParams;

    const std::string& path = appPtr->DefFilePath();

    // Open the .adef file for reading.
    FILE* file = fopen(path.c_str(), "r");
    if (file == NULL)
    {
        int error = errno;
        std::stringstream errorMessage;
        errorMessage << "Failed to open file '" << path << "'." <<
                        " Errno = " << error << "(" << strerror(error) << ").";
        throw Exception(errorMessage.str());
    }

    if (buildParams.IsVerbose())
    {
        std::cout << "Parsing '" << path << "'\n";
    }

    // Tell the parser to reset itself and connect to the new file stream for future parsing.
    ayy_FileName = path.c_str();

    ayy_IsVerbose = (BuildParamsPtr->IsVerbose() ? 1 : 0);
    ayy_EndOfFile = 0;
    ayy_ErrorCount = 0;
    ayy_set_lineno(1);
    ayy_restart(file);

    // Until the parsing is done,
    int parsingResult;
    do
    {
        // Start parsing.
        parsingResult = ayy_parse();
    }
    while ((parsingResult != 0) && (!ayy_EndOfFile));

    // Close the file
    fclose(file);

    // Do final processing.
    FinalizeApp();

    // Halt if there were errors.
    if (ayy_ErrorCount > 0)
    {
        throw Exception("Errors encountered while parsing '" + path + "'.");
    }

    AppPtr = NULL;
    BuildParamsPtr = NULL;

    if (buildParams.IsVerbose())
    {
        std::cout << "Finished parsing '" << path << "'\n";
    }
}


}

}

//--------------------------------------------------------------------------------------------------
// NOTE: The following functions are called from C code inside the bison-generated parser code.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Set the application version.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetVersion
(
    const char* version         ///< The version string.
)
//--------------------------------------------------------------------------------------------------
{
    AppPtr->Version(version);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the application sandboxed or unsandboxed ("true" if sandboxed or "false" if unsandboxed).
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetSandboxed
(
    const char* mode    ///< The mode string.
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(mode, "false") == 0)
    {
        AppPtr->IsSandboxed(false);
    }
    else if (strcmp(mode, "true") == 0)
    {
        AppPtr->IsSandboxed(true);
    }
    else
    {
        std::string msg = "Unrecognized content in 'sandboxed:' section: '";
        msg += mode;
        msg += "'.  Expected 'true' or 'false'.";
        ayy_error(msg.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the application start-up mode ("manual" or "auto"; default is "auto").
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetStartMode
(
    const char* mode    ///< The mode string.
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(mode, "auto") == 0)
    {
        AppPtr->StartMode(legato::App::AUTO);
    }
    else if (strcmp(mode, "manual") == 0)
    {
        AppPtr->StartMode(legato::App::MANUAL);
    }
    else
    {
        std::string msg = "Unrecognized start mode: '";
        msg += mode;
        msg += "'";
        ayy_error(msg.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a component to the list of components used by this application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddComponent
(
    const char* name,  ///< Name of the component (or "" if the name should be derived
                        ///   from the file system path).
    const char* path   ///< File system path of the component.
)
//--------------------------------------------------------------------------------------------------
{
    ayy_error("components: section is not yet implemented.  Use component path directly in"
              " executables section for now.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a group name to the list of groups that this application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddGroup
(
    const char* name    ///< Name of the group.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        AppPtr->AddGroup(name);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a component to the list of components used by this application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_FinishProcessesSection
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if ((ayy_IsVerbose) && (ProcEnvPtr != NULL))
    {
        std::cout << "-- end of processes section --" << std::endl;
    }

    ProcEnvPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the Process Environment object for the "processes:" subsection currently
 * being parsed.
 **/
//--------------------------------------------------------------------------------------------------
static legato::ProcessEnvironment& GetProcessEnvironment
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (ProcEnvPtr == NULL)
    {
        if (ayy_IsVerbose)
        {
            std::cout << "-- start of processes section --" << std::endl;
        }

        ProcEnvPtr = &(AppPtr->CreateProcEnvironment());
    }
    return *ProcEnvPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test if we are currently inside a 'processes' section
 *  @return true if we are
 **/
//--------------------------------------------------------------------------------------------------
static bool InProcessEnvironment
(
    void
)
{
    return ProcEnvPtr != NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a file or directory from the build host file system to the application.
 *
 * If the sourcePath ends in a '/', then it is treated as a directory.
 * Otherwise, it is treated as a file.
 *
 * @warning This function is deprecated.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host's file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    size_t sourcePathLen = strlen(sourcePath);

    if ((sourcePathLen > 0) && (sourcePath[sourcePathLen - 1] == '/'))
    {
        if (strcmp(permissions, "[r]") != 0)
        {
            ayy_error("Permissions not allowed for bundled directories."
                      "  They are always read-only at run time.");
        }
        else
        {
            ayy_AddBundledDir(sourcePath, destPath);
        }
    }
    else
    {
        ayy_AddBundledFile(permissions, sourcePath, destPath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a file from the build host file system to the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBundledFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host's file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        AppPtr->AddBundledFile(yy_CreateBundledFileMapping(permissions,
                                                           sourcePath,
                                                           destPath,
                                                           *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a directory from the build host file system to the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBundledDir
(
    const char* sourcePath, ///< The path in the build host's file system.
    const char* destPath    ///< The path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        AppPtr->AddBundledDir(yy_CreateBundledDirMapping(sourcePath, destPath, *BuildParamsPtr));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Find the Executable object for a given executable name.
 *
 * @return Reference to the Executable object.
 *
 * @throw legato::Exception if not found.
 */
//--------------------------------------------------------------------------------------------------
static legato::Executable& FindExecutable
(
    const std::string& name     ///< The name of the executable.
)
//--------------------------------------------------------------------------------------------------
{
    auto& map = AppPtr->Executables();

    auto i = map.find(name);
    if (i != map.end())
    {
        return map[name];
    }
    else
    {
        throw legato::Exception("Unknown executable '" + name + "'.");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a new executable to the app.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddExecutable
(
    const char* exePath     ///< Exe file path (relative to executables output directory).
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Creating executable '" << exePath << "'." << std::endl;
    }

    legato::Executable& exe = AppPtr->CreateExecutable(exePath);

    if (ayy_IsVerbose)
    {
        legato::Component& defaultComponent = exe.DefaultComponent();

        std::cout << "    Default component for '" << exePath << "' is '";
        std::cout << defaultComponent.Name() << "'." << std::endl;
    }

    ExePtr = &exe;
}



//--------------------------------------------------------------------------------------------------
/**
 * Add an item of content to the executable that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddExeContent
(
    const char* contentName
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const char* contentType;
        const legato::ComponentInstance* componentInstancePtr = NULL;

        std::string contentSpec = legato::DoEnvVarSubstitution(contentName);

        // If env var substitution happened.
        if (contentSpec != contentName)
        {
            if (ayy_IsVerbose)
            {
                std::cout << "Environment variable substitution of '" << contentName
                          << "' resulted in '" << contentSpec << "'." << std::endl;
            }

            // If the result was an empty string, ignore it.
            if (contentSpec == "")
            {
                return;
            }
        }

        if (legato::IsCSource(contentSpec))
        {
            contentType = "C source code";

            // Add the source code file to the default component.
            ExePtr->AddCSourceFile(contentSpec);
        }
        else if (legato::IsCppSource(contentSpec))
        {
            contentType = "C++ source code";

            // Add the source code file to the default component.
            ExePtr->AddCSourceFile(contentSpec);
        }
        else if (legato::IsLibrary(contentSpec))
        {
            contentType = "library";

            // Add the library file to the list of libraries to be linked with the default
            // component.
            ExePtr->AddLibrary(contentSpec);
        }
        else if (legato::IsComponent(contentSpec, BuildParamsPtr->ComponentDirs()))
        {
            contentType = "component";

            // Find the component and add it to the executable's list of component instances.
            // NOTE: For now, we only support one instance of a component per executable.
            legato::parser::AddComponentToExe(AppPtr, ExePtr, contentSpec, *BuildParamsPtr);
        }
        else
        {
            contentType = "** unknown **";

            std::string msg = "Executable '";
            msg += ExePtr->OutputPath();
            msg += "': Unable to identify content item '";
            msg += contentSpec;
            msg += "'.";

            throw legato::Exception(msg);
        }

        if (ayy_IsVerbose)
        {
            std::cout << "    Added '" << contentSpec << "' (" << contentType << ")"
                      << " to executable '" << ExePtr->OutputPath() << "'." << std::endl;
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Called when parsing of an executable specification finishes.
 */
//--------------------------------------------------------------------------------------------------
void ayy_FinalizeExecutable
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    ExePtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the Process object for the "run:" subsection currently being parsed.
 **/
//--------------------------------------------------------------------------------------------------
static legato::Process& GetProcess
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (ProcessPtr == NULL)
    {
        ProcessPtr = &(GetProcessEnvironment().CreateProcess());
    }
    return *ProcessPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Wrap-up the processing of a (non-empty) "run:" subsection in the "processes:" section.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_FinalizeProcess
(
    const char* name    ///< The process name, or NULL if the exe name should be used.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        if (name == NULL)
        {
            ProcessPtr->Name(legato::GetLastPathNode(ProcessPtr->ExePath()));
        }
        else
        {
            ProcessPtr->Name(name);
        }

        if (ayy_IsVerbose)
        {
            std::cout << "    Will start process '" << ProcessPtr->Name() << "' using command line: "
                      << "\"" << ProcessPtr->ExePath() << "\"";
            for (const auto& arg : ProcessPtr->CommandLineArgs())
            {
                std::cout << " \"" << arg << "\"";
            }
            std::cout << std::endl;
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }

    ProcessPtr = NULL;
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the path to the executable that is to be used to start the process.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_SetProcessExe
(
    const char* path
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        std::string exePath = legato::DoEnvVarSubstitution(yy_StripQuotes(path));

        // If env var substitution happened.
        if (exePath != path)
        {
            if (ayy_IsVerbose)
            {
                std::cout << "Environment variable substitution of '" << path
                          << "' resulted in '" << exePath << "'." << std::endl;
            }

            // If the result was an empty string,
            if (exePath == "")
            {
                throw legato::Exception("Environment variable substitution of '" + std::string(path)
                                        + "' resulted in an empty string.");
            }
        }

        if (!legato::IsAbsolutePath(exePath))
        {
            exePath = exePath;
        }

        legato::Process& process = GetProcess();

        process.ExePath(exePath);

        // If the executable path is the name of one of the executables built in this app,
        // then record that association in the Process object.
        auto exeIter = AppPtr->Executables().find(exePath);
        if (exeIter != AppPtr->Executables().end())
        {
            process.ExePtr(&(exeIter->second));
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a command-line argument to a process.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddProcessArg
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        GetProcess().AddCommandLineArg(yy_StripQuotes(arg));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds an environment variable to the Process Environment object associated with the processes:
 * section that is currently being parsed.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddEnvVar
(
    const char* name,
    const char* value
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        GetProcessEnvironment().AddEnvVar(name, yy_StripQuotes(value));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a file import mapping, which is the mapping of a non-directory object from the file system
 * somewhere outside the application sandbox to somewhere in the file system inside the application
 * sandbox.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddRequiredFile
(
    const char* permissions,///< String representing the permissions. (e.g., "[r]")
    const char* sourcePath, ///< The path in the target file system., outside the sandbox.
    const char* destPath    ///< The path in the target file system, inside the sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        AppPtr->AddRequiredFile(yy_CreateRequiredFileMapping(permissions,
                                                             sourcePath,
                                                             destPath,
                                                             *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a directory import mapping, which is the mapping of a directory object from the file system
 * somewhere outside the application sandbox to somewhere in the file system inside the application
 * sandbox.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddRequiredDir
(
    const char* permissions,///< String representing the permissions. (e.g., "[r]")
    const char* sourcePath, ///< The path in the target file system., outside the sandbox.
    const char* destPath    ///< The path in the target file system, inside the sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        AppPtr->AddRequiredDir(yy_CreateRequiredDirMapping(permissions,
                                                           sourcePath,
                                                           destPath,
                                                           *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of processes that this application is allowed to have running at any
 * given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetNumProcsLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum number of processes: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("Limit on number of processes must be a non-negative number.");
        }

        AppPtr->NumProcs(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of bytes that can be allocated for POSIX MQueues for all processes
 * in this application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMqueueSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum number of bytes for POSIX MQueues: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("POSIX MQueue size limit must be a non-negative number.");
        }

        AppPtr->MqueueSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of real-time signals that are allowed to be queued-up waiting for
 * processes in this application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetRTSignalQueueSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum number of real-time signals: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("Real-time signal queue size limit must be a non-negative number.");
        }

        AppPtr->RtSignalQueueSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of memory (in kilobytes) the application is allowed to use for all of it
 * processes.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMemoryLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Memory limit: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("Memory limit must be a non-negative number.");
        }

        AppPtr->MemLimit(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the cpu share for all processes in the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetCpuShare
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  CPU share: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("CPU share must be a non-negative number.");
        }

        AppPtr->CpuShare(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of RAM (in bytes) that the application is allowed to consume through
 * usage of its temporary sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetFileSystemSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum size of sandbox temporary file system: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("File system size limit must be a non-negative number of bytes.");
        }

        AppPtr->FileSystemSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the starting (and maximum) priority level of processes in the current processes section.
 *
 * Allowable values are:
 *   "idle" - intended for very low priority processes that will only get CPU time if there are
 *            no other processes waiting for the CPU.
 *
 *   "low",
 *   "medium",
 *   "high" - intended for normal processes that contend for the CPU.  Processes with these
 *            priorities do not preempt each other but their priorities affect how they are
 *            inserted into the scheduling queue. ie. "high" will get higher priority than
 *            "medium" when inserted into the queue.
 *
 *   "rt1"
 *    ...
 *   "rt32" - intended for (soft) realtime processes. A higher realtime priority will pre-empt
 *            a lower realtime priority (ie. "rt2" would pre-empt "rt1"). Processes with any
 *            realtime priority will pre-empt processes with "high", "medium", "low" and "idle"
 *            (non-real-time) priorities.  Also, note that processes with these realtime priorities
 *            will pre-empt the Legato framework processes so take care to design realtime
 *            processes that relinguishes the CPU appropriately.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetPriority
(
    const char* priority
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Starting (and max) process priority: " << priority << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (   (strcmp(priority, "idle") == 0)
            || (strcmp(priority, "low") == 0)
            || (strcmp(priority, "medium") == 0)
            || (strcmp(priority, "high") == 0)
           )
        {
            env.Priority(priority);
        }
        else if ((priority[0] == 'r') && (priority[1] == 't'))
        {
            int number = yy_GetNumber(priority + 2);
            if ((number < 1) || (number > 32))
            {
                throw legato::Exception("Real-time priority level must be between rt1 and rt32, inclusive.");
            }
            env.Priority(priority);
        }
        else
        {
            throw legato::Exception(std::string("Unrecognized priority level '") + priority + "'.");
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) of the core dump file that a process in the current processes
 * section can generate.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetCoreFileSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum size of core dump files: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Maximum core dump file size limit must be a non-negative number of bytes.");
        }

        env.CoreFileSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in the current processes section can make files.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMaxFileSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum file size: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Maximum file size limit must be a non-negative number of bytes.");
        }

        env.MaxFileSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in this processes section is allowed to lock
 * into physical memory.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMemLockSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
                ///  Will be rounded down to the nearest system memory page size.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum amount of locked physical memory: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Locked memory size limit must be a non-negative number of bytes.");
        }

        env.MemLockSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of file descriptor that each process in the processes section are
 * allowed to have open at one time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetNumFdsLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum number of file descriptors: " << limit << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Number of file descriptors must be positive.");
        }

        env.NumFds(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the action that should be taken if a process in the process group currently being parsed
 * terminates with a non-zero exit code (i.e., any error code other than EXIT_SUCCESS).
 *
 * Accepted actions are:
 *  "ignore"        - Leave the process dead.
 *  "restart"       - Restart the process.
 *  "restartApp"    - Terminate and restart the whole application.
 *  "stopApp"       - Terminate the application and leave it stopped.
 *  "reboot"        - Reboot the device.
 *  "pauseApp"      - Send a SIGSTOP to all processes in the application, halting them in their
 *                    tracks, but not killing them.  This allows the processes to be inspected
 *                    for debugging purposes.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetFaultAction
(
    const char* action
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Fault action: " << action << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (   (strcmp(action, "ignore") == 0)
            || (strcmp(action, "restart") == 0)
            || (strcmp(action, "restartApp") == 0)
            || (strcmp(action, "stopApp") == 0)
            || (strcmp(action, "reboot") == 0)
            || (strcmp(action, "pauseApp") == 0)
           )
        {
            env.FaultAction(action);
        }
        else
        {
            throw legato::Exception(std::string("Unknown fault action '") + action + "'.");
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the action that should be taken if a process terminates due to a watchdog time-out.
 *
 * @todo Implement watchdog actions.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogAction
(
    const char* action  ///< Accepted actions are TBD.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Watchdog action: " << action << std::endl;
    }
    try
    {
        {
            AppPtr->UsesWatchdog();

            if (InProcessEnvironment())
            {
                legato::ProcessEnvironment& env = GetProcessEnvironment();
                env.WatchdogAction().Set(action);
            }
            else
            {
                AppPtr->WatchdogAction().Set(action);
            }
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the timeout for a watchdog.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogTimeout
(
    const int timeout  ///< The time in milliseconds after which the watchdog expires if not
                       ///< kicked again before then. Only positive values are allowed.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Watchdog timeout: " << timeout << std::endl;
    }
    try
    {
        AppPtr->UsesWatchdog();

        if (InProcessEnvironment())
        {
            legato::ProcessEnvironment& env = GetProcessEnvironment();
            env.WatchdogTimeout().Set(static_cast<int32_t>(timeout));
        }
        else
        {
            AppPtr->WatchdogTimeout().Set(static_cast<int32_t>(timeout));
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the timeout for the watchdogs be disabled in the case that a timeout of "never" is specified
 *
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogDisabled
(
    const char* timeout  ///< if the string "never" is here the watchdog will be disabled
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Watchdog timeout: " << timeout << std::endl;
    }
    try
    {
        AppPtr->UsesWatchdog();

        if (InProcessEnvironment())
        {
            legato::ProcessEnvironment& env = GetProcessEnvironment();
            env.WatchdogTimeout().Set(timeout);
        }
        else
        {
            AppPtr->WatchdogTimeout().Set(timeout);
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the size of a pool.
 *
 * The pool name is expected to be of the form "process.component.pool".
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetPoolSize
(
    const char* poolName,
    int numBlocks
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Pool '" << poolName << " set to " << numBlocks << " blocks" << std::endl;
    }
    std::cerr << "**WARNING: Pool size configuration not yet implemented." << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check that there's no illegal characters in an interface specification.
 *
 * @note This is necessary because the interface specifications are tokenized as FILE_PATH
 * tokens, which can have some characters that are not valid as parts of an interface
 * specification.
 *
 * @throw legato::Exception if there's a bad character.
 */
//--------------------------------------------------------------------------------------------------
static void CheckForBadCharsInInterfaceSpec
(
    const char* interfaceSpec
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: The parser won't accept whitespace in this stuff, so we don't have to check that.

    const char illegalChars[] = "?-/+";
    const char* badCharPtr;

    badCharPtr = strpbrk(interfaceSpec, illegalChars);

    if (badCharPtr != NULL)
    {
        std::string msg;

        msg += "Illegal character '";
        msg += *badCharPtr;
        msg += "' in interface specification '";
        msg += interfaceSpec;
        msg += "'.";

        throw legato::Exception(msg);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given interface specifier is well formed.
 *
 * @throw legato::Exception if not.
 *
 * @return The number of parts it has (2 = "app.service", 3 = "exe.comp.interface").
 **/
//--------------------------------------------------------------------------------------------------
static size_t CheckInterfaceSpec
(
    const std::string& interfaceSpec    ///< Specifier of the form "exe.component.interface"
)
//--------------------------------------------------------------------------------------------------
{
    CheckForBadCharsInInterfaceSpec(interfaceSpec.c_str());

    // Split the interface specifier into its component parts.
    size_t firstPeriodPos = interfaceSpec.find('.');
    if (firstPeriodPos == std::string::npos)
    {
        throw legato::Exception("Interface specifier '" + interfaceSpec + "'"
                                " is missing its '.' separators.");
    }
    if (firstPeriodPos == 0)
    {
        throw legato::Exception("Nothing before '.' separator in"
                                " interface specifier '" + interfaceSpec + "'.");
    }

    size_t secondPeriodPos = interfaceSpec.find('.', firstPeriodPos + 1);
    if (secondPeriodPos == std::string::npos)
    {
        // This is an "app.service" external interface specifier.

        // Make sure there's something after the separator.
        if (interfaceSpec.substr(firstPeriodPos + 1).empty())
        {
            throw legato::Exception("Service name missing after '.' separator in external"
                                    " interface specifier '" + interfaceSpec + "'.");
        }

        return 2;
    }
    else
    {
        // This is an "exe.component.interface" internal interface specifier.

        // Make sure there's something between the '.' separators.
        if (secondPeriodPos == firstPeriodPos + 1)
        {
            throw legato::Exception("Interface component name missing between '.' separators in"
                                    " internal interface specifier '" + interfaceSpec + "'.");
        }

        // Make sure there's only two separators.
        if (interfaceSpec.find('.', secondPeriodPos + 1) != std::string::npos)
        {
            throw legato::Exception("Interface specifier '" + interfaceSpec + "' contains too"
                                    " many '.' separators.");
        }

        // Make sure there's something after the second separator.
        if (interfaceSpec.substr(secondPeriodPos + 1).empty())
        {
            throw legato::Exception("Interface instance name missing after second '.' separator in"
                                    " internal interface specifier '" + interfaceSpec + "'.");
        }

        return 3;
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Mark a client-side IPC API interface as an external interface that can be bound to other
 * apps or users using a given interface name.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddRequiredApi
(
    const char* externalAlias, ///< [in] Name to use outside app (NULL = use client interface name).
    const char* clientInterfaceSpec  ///< [in] Client-side interface (exe.component.interface).
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        if (CheckInterfaceSpec(clientInterfaceSpec) != 3)
        {
            throw legato::Exception("Second '.' separator missing in internal interface"
                                    " specification '" + std::string(clientInterfaceSpec) + "'."
                                    " Should be of the form \"exe.component.interface\".");
        }

        auto& interface = AppPtr->FindClientInterface(clientInterfaceSpec);

        std::string interfaceName;

        if (externalAlias == NULL)
        {
            interfaceName = interface.ServiceInstanceName();
        }
        else
        {
            interfaceName = externalAlias;
        }

        if (ayy_IsVerbose)
        {
            std::cout << "  Making client-side interface '" << clientInterfaceSpec
                      << "' into an external interface called '" << interfaceName
                      << "' that must be bound to a service.'" << std::endl;
        }

        // Set the interface's inter-app name.
        interface.MakeExternal(interfaceName);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark a server-side IPC API interface as an external interface that other apps or users
 * can bind to using a given interface name.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddProvidedApi
(
    const char* externalAlias, ///< [in] Name to use outside app (NULL = use server interface name).
    const char* serverInterfaceSpec  ///< [in] Client-side interface (exe.component.interface).
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        if (CheckInterfaceSpec(serverInterfaceSpec) != 3)
        {
            throw legato::Exception("Second '.' separator missing in internal interface"
                                    " specification '" + std::string(serverInterfaceSpec) + "'."
                                    " Should be of the form \"exe.component.interface\".");
        }

        auto& interface = AppPtr->FindServerInterface(serverInterfaceSpec);

        std::string serviceName;

        if (externalAlias == NULL)
        {
            serviceName = interface.ServiceInstanceName();
        }
        else
        {
            serviceName = externalAlias;
        }

        if (ayy_IsVerbose)
        {
            std::cout << "  Making server-side interface '" << serverInterfaceSpec
                      << "' into an external interface called '" << serviceName
                      << "' available for other apps to bind to.'" << std::endl;
        }

        // Set the interface's inter-app name.
        interface.MakeExternal(serviceName);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding.
 *
 * The client interface specification is always expected to be of the form
 * "exe.component.interface".
 *
 * The server interface specification can be one of the following:
 * - "exe.component.interface" = an internal bind (within the app).
 * - "app.service" = an external bind to a service provided by an application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBind
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverInterfaceSpec     ///< [in] Server-side interface.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const std::string clientSpec(clientInterfaceSpec);
        const std::string serverSpec(serverInterfaceSpec);

        // Check that the client and server interface specifications are valid.
        size_t numParts = CheckInterfaceSpec(clientSpec);
        if (numParts != 3)
        {
            throw legato::Exception("Too few parts in client interface specification. "
                                    " Interface name missing from end of '"
                                    + clientSpec + "'.");
        }
        numParts = CheckInterfaceSpec(serverSpec);

        // If there are three parts to the server interface specifier, then it must be an
        // internal binding.
        if (numParts == 3)
        {
            if (ayy_IsVerbose)
            {
                std::cout << "  Binding '" << clientInterfaceSpec << "' to '" << serverInterfaceSpec
                          << "'." << std::endl;
            }

            AppPtr->AddInternalApiBind(clientInterfaceSpec, serverInterfaceSpec);
        }

        // If there are only two parts to the server interface specifier, then this must be an
        // external binding, and the server interface specification must be of the form
        // "app.service".
        else
        {
            // Extract the app name.
            size_t periodPos = serverSpec.find('.');
            std::string serverAppName = serverSpec.substr(0, periodPos);

            // Extract the service name.
            std::string serverServiceName = serverSpec.substr(periodPos + 1);

            if (ayy_IsVerbose)
            {
                std::cout << "  Binding '" << clientSpec << "' to service '"
                          << serverServiceName << "' provided by application '" << serverAppName
                          << "'." << std::endl;
            }

            legato::ExternalApiBind& binding = AppPtr->AddExternalApiBind(clientSpec);
            binding.ServerAppName(serverAppName);
            binding.ServerServiceName(serverServiceName);
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding to a service offered by a user.
 *
 * The client interface specification is expected to be of the form
 * "exe.component.interface".
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBindOutToUser
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverUserName,         ///< [in] Server-side user name.
    const char* serverServiceName       ///< [in] Server-side service name.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const std::string clientSpec(clientInterfaceSpec);
        const std::string userName(serverUserName);
        std::string serviceName(serverServiceName);

        // First check that the interface specifications are valid.
        size_t numParts = CheckInterfaceSpec(clientSpec);
        if (numParts != 3)
        {
            throw legato::Exception("Too few parts in client interface specification. "
                                    " Interface name missing from end of '"
                                    + clientSpec + "'.");
        }
        CheckForBadCharsInInterfaceSpec(serverUserName);
        CheckForBadCharsInInterfaceSpec(serverServiceName);

        // Check that the server service name has exactly one '.' character at the beginning of it.
        if (serviceName.front() != '.')
        {
            throw legato::Exception("Missing '.' separator in server external interface"
                                    " specification '<" + userName + ">" + serviceName + "'.");
        }
        if (serviceName.find('.', 1) != std::string::npos)
        {
            throw legato::Exception("Too many '.' separators in server external interface"
                                    " specification '<" + userName + ">" + serviceName + "'.");
        }

        // Extract the service name and make sure it's not empty.
        serviceName = serviceName.substr(1);
        if (serviceName.empty())
        {
            throw legato::Exception("Service name missing after '.' separator in"
                                    " server external interface specification '<"
                                    + userName + ">" + serviceName + "'.");
        }

        if (ayy_IsVerbose)
        {
            std::cout << "  Binding '" << clientSpec << "' to service '"
                      << serviceName << "' provided by user '" << userName
                      << "'." << std::endl;
        }

        // Application user names are the application name with "app" added to the front.
        // E.g., the user name for "MyApp" is "appMyApp".
        legato::ExternalApiBind& binding = AppPtr->AddExternalApiBind(clientSpec);
        binding.ServerUserName(userName);
        binding.ServerServiceName(serviceName);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add read access permission for a particular configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddConfigTreeAccess
(
    const char* permissions,    ///< [in] String representing the permissions (e.g., "[r]", "[w]").
    const char* treeName        ///< [in] The name of the configuration tree.
)
//--------------------------------------------------------------------------------------------------
{
    const char readable[] = "read";
    const char writeable[] = "read and write";
    const char* accessMode;

    try
    {
        int flags = yy_GetPermissionFlags(permissions);

        if (flags & legato::PERMISSION_EXECUTABLE)
        {
            throw legato::Exception("Executable permission 'x' invalid for configuration trees.");
        }

        // For configuration trees, writeable implies readable.
        if (flags & legato::PERMISSION_WRITEABLE)
        {
            flags |= legato::PERMISSION_READABLE;
            accessMode = writeable;
        }
        else
        {
            accessMode = readable;
        }

        if (ayy_IsVerbose)
        {
            std::cout << "  Granting " << accessMode << " access to configuration tree '"
                      << treeName << "'." << std::endl;
        }

        AppPtr->AddConfigTreeAccess(treeName, flags);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}
