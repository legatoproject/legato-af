//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mksys" functionality of the "mk" tool.
 *
 *  Run 'mksys --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <sys/sendfile.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

#include "Args.h"
#include "LegatoObjectModel.h"
#include "Parser.h"
#include "mksys.h"
#include "InterfaceBuilder.h"
#include "ComponentBuilder.h"
#include "ExecutableBuilder.h"
#include "ApplicationBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Builder objects when they are created.
static legato::BuildParams_t BuildParams;

/// Path to the directory into which the final, built system file should be placed.
static std::string OutputDir;

/// The root object for this system's object model.
static legato::System System;



//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments and update the static operating parameters variables.
 *
 * Throws a std::runtime_error exception on failure.
 **/
//--------------------------------------------------------------------------------------------------
static void GetCommandLineArgs
(
    int argc,
    const char** argv
)
//--------------------------------------------------------------------------------------------------
{
    std::string target;
    int isVerbose = 0;  // 0 = not verbose, non-zero = verbose.

    // Path to the directory where intermediate build output files (such as generated
    // source code and object code files) should be put.
    std::string objectFilesDir;

    std::string cFlags;  // C compiler flags.
    std::string ldFlags; // Linker flags.

    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [&](const char* arg) { cFlags += " ";  cFlags += arg; };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [&](const char* arg) { ldFlags += " ";  ldFlags += arg; };

    // Lambda function that gets called once for each occurence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path)
        {
            BuildParams.AddInterfaceDir(legato::DoEnvVarSubstitution(path));
        };

    // Lambda function that gets called once for each occurence of the source/component search path
    // argument on the command line.
    auto compPathPush = [&](const char* path)
        {
            BuildParams.AddComponentDir(legato::DoEnvVarSubstitution(path));
        };

    // Lambda function that gets called once for each occurence of a .sdef file name on the
    // command line.
    auto sdefFileNameSet = [&](const char* param)
            {
                static bool matched = false;
                if (matched)
                {
                    throw legato::Exception("Only one system definition (.sdef) file allowed.");
                }
                matched = true;
                System.DefFilePath(legato::DoEnvVarSubstitution(param));
            };

    le_arg_AddOptionalString(&OutputDir,
                             ".",
                             'o',
                             "output-dir",
                             "Specify the directory into which the final, built system file"
                             "(ready to be installed on the target) should be put.");

    le_arg_AddOptionalString(&objectFilesDir,
                             "",
                             'w',
                             "object-dir",
                             "Specify the directory into which any intermediate build artifacts"
                             " (such as .o files and generated source code files) should be put.");

    le_arg_AddMultipleString('i',
                             "interface-search",
                             "Add a directory to the interface search path.",
                             ifPathPush);

    le_arg_AddMultipleString('c',
                             "component-search",
                             "Add a directory to the component search path (same as -s).",
                             compPathPush);

    le_arg_AddMultipleString('s',
                             "source-search",
                             "Add a directory to the source search path (same as -c).",
                             compPathPush);

    le_arg_AddOptionalString(&target,
                             "localhost",
                             't',
                             "target",
                             "Set the compile target (localhost|ar7).");

    le_arg_AddOptionalFlag(&isVerbose,
                           'v',
                           "verbose",
                           "Set into verbose mode for extra diagnostic information.");

    le_arg_AddMultipleString('C',
                             "cflags",
                             "Specify extra flags to be passed to the C compiler.",
                             cFlagsPush);

    le_arg_AddMultipleString('L',
                             "ldflags",
                             "Specify extra flags to be passed to the linker when linking "
                             "executables.",
                             ldFlagsPush);

    // Any remaining parameters on the command-line are treated as the .sdef file path.
    // Note: there should only be one parameter not prefixed by an argument identifier.
    le_arg_SetLooseParamHandler(sdefFileNameSet);

    le_arg_Scan(argc, argv);

    // Were we given an system definition?
    if (System.DefFilePath() == "")
    {
        throw std::runtime_error("A system definition must be supplied.");
    }

    // If we were not given an object file directory (intermediate build output directory) path,
    // use a subdirectory of the current working directory.
    if (objectFilesDir == "")
    {
        objectFilesDir = "./_build_" + System.Name() + "/" + target;
    }
    BuildParams.ObjOutputDir(objectFilesDir);

    // Add the directory containing the .sdef file to the list of component search directories
    // and the list of interface search directories.
    std::string systemDefFileDir = legato::GetContainingDir(System.DefFilePath());
    BuildParams.AddComponentDir(systemDefFileDir);
    BuildParams.AddInterfaceDir(systemDefFileDir);

    // Store other build params specified on the command-line.
    if (isVerbose)
    {
        BuildParams.SetVerbose();
    }
    BuildParams.SetTarget(target);
    BuildParams.CCompilerFlags(cFlags);
    BuildParams.LinkerFlags(ldFlags);
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct the object model.
 */
//--------------------------------------------------------------------------------------------------
static void ConstructObjectModel
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Parse the .sdef file.


    // This constructs the object model under the System object that we give it.
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for the application-wide limits (including the start-up modes).
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppLimitsConfig
(
    std::ofstream& cfgStream
)
//--------------------------------------------------------------------------------------------------
{
    if (App.IsSandboxed() == false)
    {
        cfgStream << "  \"sandboxed\" !f" << std::endl;
    }

    if (App.Debug() == true)
    {
        cfgStream << "  \"debug\" !t" << std::endl;
    }

    if (App.StartMode() == legato::App::MANUAL)
    {
        cfgStream << "  \"deferLaunch\" !t" << std::endl;
    }

    if (App.NumProcs() != SIZE_MAX)
    {
        cfgStream << "  \"numProcessesLimit\" [" << App.NumProcs() << "]" << std::endl;
    }

    if (App.MqueueSize() != SIZE_MAX)
    {
        cfgStream << "  \"totalPosixMsgQueueSizeLimit\" [" << App.MqueueSize() << "]" << std::endl;
    }

    if (App.RtSignalQueueSize() != SIZE_MAX)
    {
        cfgStream << "  \"rtSignalQueueSizeLimit\" [" << App.RtSignalQueueSize() << "]" << std::endl;
    }

    if (App.MemLimit() != SIZE_MAX)
    {
        cfgStream << "\"memLimit\" [" << App.MemLimit() << "]" << std::endl;
    }

    if (App.CpuShare() != SIZE_MAX)
    {
        cfgStream << "\"cpuShare\" [" << App.CpuShare() << "]" << std::endl;
    }

    if (App.FileSystemSize() != SIZE_MAX)
    {
        // This is not supported for unsandboxed apps.
        if (App.IsSandboxed() == false)
        {
            std::cerr << "**** Warning: File system size limit being ignored for unsandboxed"
                      << " application '" << App.Name() << "'." << std::endl;
        }
        else
        {
            cfgStream << "  \"fileSystemSizeLimit\" [" << App.FileSystemSize() << "]" << std::endl;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for the list of groups that the application's user should be a
 * member of.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateGroupsConfig
(
    std::ofstream& cfgStream
)
//--------------------------------------------------------------------------------------------------
{
    const auto& groupsList = App.Groups();

    // If the groups list is empty, nothing needs to be done.
    if (groupsList.empty())
    {
        return;
    }

    // If the application is sandboxed, warn if there's group membership configuration,
    // and ignore it.
    if (App.IsSandboxed())
    {
        std::cerr << "**** Warning: Groups list being ignored for sandboxed application '"
                  << App.Name() << "'." << std::endl;
        return;
    }

    // Group names are specified by inserting empty leaf nodes under the "groups" branch
    // of the application's configuration tree.
    cfgStream << "  \"groups\"" << std::endl;
    cfgStream << "  {" << std::endl;

    for (auto groupName : groupsList)
    {
        cfgStream << "  \"" << groupName << "\" \"\"" << std::endl;
    }

    cfgStream << "  }" << std::endl << std::endl;
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for a single file mapping.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSingleFileMappingConfig
(
    std::ofstream& cfgStream,   ///< Stream to send the configuration to.
    size_t          index,      ///< The index of the file in the files list in the configuration.
    const legato::FileMapping& mapping  ///< The file mapping.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "    \"" << index++ << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"src\" \"" << mapping.m_SourcePath << "\"" << std::endl;
    cfgStream << "      \"dest\" \"" << mapping.m_DestPath << "\"" << std::endl;
    cfgStream << "    }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all file mappings from outside the application sandbox to inside
 * the sandbox.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateFileMappingConfig
(
    std::ofstream& cfgStream
)
//--------------------------------------------------------------------------------------------------
{
    size_t index = 0;

    // Create nodes under "files", where each node is named with an index, starting a 0,
    // and contains a "src" node and a "dest" node.
    cfgStream << "  \"files\"" << std::endl;
    cfgStream << "  {" << std::endl;

    // Import the standard libraries that everyone needs.
    const char* libList[] = {   "/lib/ld-linux.so.3",
                                "/lib/libc.so.6",
                                "/lib/libpthread.so.0",
                                "/lib/librt.so.1",
                                "/lib/libgcc_s.so.1",
                                "/usr/lib/libstdc++.so.6",
                                "/lib/libm.so.6",
                                "/usr/local/lib/liblegato.so"
                            };
    legato::FileMapping mapping({ legato::PERMISSION_READABLE, "", "/lib/" });
    for (size_t i = 0; i < (sizeof(libList) / sizeof(libList[0])); i++)
    {
        mapping.m_SourcePath = libList[i];
        GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
    }

    // Import the files specified in the .adef file.
    for (const auto& mapping : App.ExternalFiles())
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
    }

    // Included files also need to be imported into the application sandbox.
    // But, the import is from a point relative to the app's install directory.
    // On the target, the source file will be found in the application's install directory,
    // under a directory whose path is the same as the directory in which the file will appear
    // inside the sandbox.  For example, if the app is installed under /opt/legato/apps/myApp/,
    // then the file /opt/legato/apps/myApp/usr/share/beep.wav would appear inside the sandbox
    // under the directory /usr/share/.
    for (const auto& mapping : App.IncludedFiles())
    {
        legato::FileMapping importMapping = mapping;

        // Extract the name of the source file, as it will appear in the application's install
        // directory on target.
        std::string sourceFileName = legato::GetLastPathNode(mapping.m_SourcePath);

        // If the destination is a directory name, the file's name in the application sandbox will
        // be the same as the file's name in the application install directory, and the
        // Supervisor will automatically add the file name onto the end of the destination path.
        if (mapping.m_DestPath.back() == '/')
        {
            // The source path must be a relative path from the application install directory,
            // so skip over the leading '/' in the destination path (which must be absolute)
            // and copy the rest to the source path.
            importMapping.m_SourcePath = importMapping.m_DestPath.substr(1);

            // Append the original source path's file name to the new source path.
            importMapping.m_SourcePath += sourceFileName;

        }
        // But, if the destination is a file name, the file name in the sandbox (dest path) may be
        // different from the file name in the application install directory (source path)
        else
        {
            // Get the directory path that is the same relative to both the root of the sandbox
            // and the application's install directory.  (Skip the leading '/' in the dest path
            // and strip off the file name.)
            std::string dirPath = legato::GetContainingDir(importMapping.m_DestPath.substr(1));

            // Generate the file mapping's source path relative to the application's install
            // directory.
            importMapping.m_SourcePath = dirPath + '/' + sourceFileName;

            // Note: the destination path requires no modification in this case.  It will already
            // be an absolute path to a file inside the sandbox.
        }

        GenerateSingleFileMappingConfig(cfgStream, index++, importMapping);
    }

    // Map into the sandbox all the files for all the components.
    for (const auto& pair : App.ComponentMap())
    {
        // External files...
        for (const auto& mapping : pair.second.ExternalFiles())
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
        }

        // Included (bundled) files...
        // NOTE: Included files also need to be mapped into the application sandbox because the
        // application install directory is outside the sandbox.
        for (const auto& mapping : pair.second.IncludedFiles())
        {
            // The mapping's source path is absolute, but it should be anchored at the root of
            // the app's install directory tree, so we turn the source path into a relative path by
            // removing the leading '/' character.  The Supervisor treats all relative source paths
            // for bind mounts as relative to the root of the application's install directory tree.
            legato::FileMapping importMapping = mapping;
            std::string sourceFileName = legato::GetLastPathNode(mapping.m_SourcePath);
            importMapping.m_SourcePath = importMapping.m_DestPath.substr(1) + sourceFileName;

            GenerateSingleFileMappingConfig(cfgStream, index++, importMapping);
        }
    }

    cfgStream << "  }" << std::endl << std::endl;
}



//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for the environment variable settings for a process.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateProcessEnvVarsConfig
(
    std::ofstream& cfgStream,
    const legato::ProcessEnvironment& procEnv
)
//--------------------------------------------------------------------------------------------------
{
    // The PATH environment variable has to be handled specially.  If no PATH variable is
    // specified in the .adef, we must provide one.
    bool pathSpecified = false;

    // Any environment variables are declared under a node called "envVars".
    // Each env var has its own node, with the name of the node being the name of
    // the environment variable.
    cfgStream << "      \"envVars\"" << std::endl;
    cfgStream << "      {" << std::endl;
    for (const auto& pair : procEnv.EnvVarList())
    {
        if (pair.first == "PATH")
        {
            pathSpecified = true;
        }

        cfgStream << "        \"" << pair.first << "\" \"" << pair.second << "\""
                  << std::endl;
    }

    if (!pathSpecified)
    {
        // The default path depends on whether the application is sandboxed or not.
        std::string path = "/usr/local/bin:/usr/bin:/bin";
        if (App.IsSandboxed() == false)
        {
            path = "/opt/legato/apps/" + App.Name() + "/bin:" + path;
        }
        cfgStream << "        \"PATH\" \"" << path << "\"" << std::endl;
    }

    cfgStream << "      }" << std::endl;
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all the processes that the Supervisor should start when the
 * application is started.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateProcessConfig
(
    std::ofstream& cfgStream
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "procs", where each process has its own node, named after the process.
    cfgStream << "  \"procs\"" << std::endl;
    cfgStream << "  {" << std::endl;

    for (const auto& procEnv : App.ProcEnvironments())
    {
        for (const auto& process : procEnv.ProcessList())
        {
            cfgStream << "    \"" << process.Name() << "\"" << std::endl;
            cfgStream << "    {" << std::endl;

            // If the process has debugging enabled, then add a "debug" configuration
            // node for this process that contains the debug port number to be used.
            if (process.IsDebuggingEnabled())
            {
                cfgStream << "      \"debug\" [" << process.DebugPort() << "]" << std::endl;
            }

            // The command-line argument list is an indexed list of arguments under a node called
            // "args", where the first argument (0) must be the executable to run.
            cfgStream << "      \"args\"" << std::endl;
            cfgStream << "      {" << std::endl;
            cfgStream << "        \"0\" \"" << process.ExePath() << "\"" << std::endl;
            int argIndex = 1;
            for (const auto& arg : process.CommandLineArgs())
            {
                cfgStream << "        \"" << argIndex << "\" \"" << arg << "\"" << std::endl;
                argIndex++;
            }
            cfgStream << "      }" << std::endl;

            GenerateProcessEnvVarsConfig(cfgStream, procEnv);

            // Generate the priority, fault action, and limits configuration.
            if (procEnv.FaultAction() != "")
            {
                cfgStream << "      \"faultAction\" \"" << procEnv.FaultAction() << "\"" << std::endl;
            }
            if (procEnv.Priority() != "")
            {
                cfgStream << "      \"priority\" \"" << procEnv.Priority() << "\"" << std::endl;
            }
            if (procEnv.CoreFileSize() != SIZE_MAX)
            {
                cfgStream << "      \"coreDumpFileSizeLimit\" [" << procEnv.CoreFileSize() << "]" << std::endl;
            }
            if (procEnv.MaxFileSize() != SIZE_MAX)
            {
                cfgStream << "      \"maxFileSizeLimit\" [" << procEnv.MaxFileSize() << "]" << std::endl;
            }
            if (procEnv.MemLockSize() != SIZE_MAX)
            {
                cfgStream << "      \"memLockSizeLimit\" [" << procEnv.MemLockSize() << "]" << std::endl;
            }
            if (procEnv.NumFds() != SIZE_MAX)
            {
                cfgStream << "      \"numFileDescriptorsLimit\" [" << procEnv.NumFds() << "]" << std::endl;
            }

            cfgStream << "    }" << std::endl;
        }
    }

    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for a single IPC binding.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSingleIpcBindingConfig
(
    std::ofstream& cfgStream,   ///< Stream to send the configuration to.
    const legato::IpcBinding& binding  ///< IPC Binding
)
//--------------------------------------------------------------------------------------------------
{
    const legato::ImportedInterface& interface = App.FindClientInterface(
                                                                    binding.ClientInterfaceName());

    cfgStream << "    \"" << binding.ClientInterfaceName() << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"protocol\" \"" << interface.Hash() << "\"" << std::endl;
    cfgStream << "      \"server\"" << std::endl;
    cfgStream << "      {" << std::endl;
    cfgStream << "        \"user\" \"" << binding.ServerUserName() << "\"" << std::endl;
    cfgStream << "        \"interface\" \"" << binding.ServerServiceName() << "\"" << std::endl;
    cfgStream << "      }" << std::endl;
    cfgStream << "    }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all the IPC bindings for this application's client interfaces.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateIpcBindingConfig
(
    std::ofstream& cfgStream
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "bindings", where each binding has its own node, named with the client
    // interface service name.
    cfgStream << "  \"bindings\"" << std::endl;
    cfgStream << "  {" << std::endl;

    for (const auto& mapEntry : App.IpcBindingMap())
    {
        GenerateSingleIpcBindingConfig(cfgStream, mapEntry.second);
    }

    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration that the Supervisor needs for this app.  This is the configuration
 * that will be installed in the root configuration tree by the installer when the app is
 * installed on the target.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSupervisorConfig
(
    const std::string& stagingDirPath   ///< Path to the root of the app's staging directory.
)
//--------------------------------------------------------------------------------------------------
{
    std::string path = stagingDirPath + "/root.cfg";

    if (BuildParams.IsVerbose())
    {
        std::cout << "Generating Supervisor configuration for this app under '"
                  << path << "'." << std::endl;
    }

    std::ofstream cfgStream(path, std::ofstream::trunc);

    cfgStream << "{" << std::endl << std::endl;

    GenerateAppLimitsConfig(cfgStream);

    GenerateGroupsConfig(cfgStream);

    GenerateFileMappingConfig(cfgStream);

    GenerateProcessConfig(cfgStream);

    GenerateIpcBindingConfig(cfgStream);

    cfgStream << "}" << std::endl;
}



//--------------------------------------------------------------------------------------------------
/**
 * Build the application.
 */
//--------------------------------------------------------------------------------------------------
static void Build
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create Builder objects for later use.
    ExecutableBuilder_t exeBuilder(BuildParams);
    ComponentBuilder_t componentBuilder(BuildParams);
    InterfaceBuilder_t interfaceBuilder(BuildParams);

    // Construct the working directory structure, which consists of an "obj" directory and
    // a "staging" directory.  Inside the "staging" directory, there is "lib", "bin", and "config".
    // The "obj" directory is for intermediate build output, like generated .c files and .o files.
    // The "staging" directory will get tar-compressed to become the actual application file.

    if (BuildParams.IsVerbose())
    {
        std::cout << "Creating working directories under '" << BuildParams.ObjOutputDir() << "'." << std::endl;
    }
    std::string stagingDirPath = BuildParams.ObjOutputDir() + "/staging";
    BuildParams.LibOutputDir(stagingDirPath + "/lib");
    BuildParams.ExeOutputDir(stagingDirPath + "/bin");
    BuildParams.ObjOutputDir(BuildParams.ObjOutputDir() + "/work");

    legato::MakeDir(BuildParams.ObjOutputDir());
    legato::MakeDir(BuildParams.LibOutputDir());
    legato::MakeDir(BuildParams.ExeOutputDir());

    // For each component in the application.
    auto& map = App.ComponentMap();
    for (auto& i : map)
    {
        // Build the component.
        componentBuilder.Build(i.second);

        // Copy all included files into the staging area.
        auto& includedFilesList = i.second.IncludedFiles();
        for (auto fileMapping : includedFilesList)
        {
            CopyToStaging(fileMapping.m_SourcePath, stagingDirPath, fileMapping.m_DestPath);
        }
    }

    // For each executable,
    auto& exeList = App.Executables();
    for (auto i = exeList.begin(); i != exeList.end(); i++)
    {
        legato::Executable& exe = (*i).second;

        // Auto-generate the source code file containing main() and add it to the default component.
        exeBuilder.GenerateMain(exe);

        // For each component instance in the executable,
        for (auto& componentInstance : exe.ComponentInstanceList())
        {
            // Build the IPC import/export libs and add them to the list of libraries that need
            // to be linked with the executable and bundled in the application.
            for (auto& mapEntry : componentInstance.ExportedInterfaces())
            {
                auto& interface = mapEntry.second;
                interfaceBuilder.Build(interface);

                // Add the library to the list of files that need to be mapped into the sandbox.
                App.AddExternalFile({ legato::PERMISSION_READABLE,
                                      "lib/lib" + interface.Lib().ShortName() + ".so",
                                      "/lib/" });

            }
            for (auto& mapEntry : componentInstance.ImportedInterfaces())
            {
                auto& interface = mapEntry.second;
                interfaceBuilder.Build(mapEntry.second);

                // Add the library to the list of files that need to be mapped into the sandbox.
                App.AddExternalFile({ legato::PERMISSION_READABLE,
                                      "lib/lib" + interface.Lib().ShortName() + ".so",
                                      "/lib/" });
            }
        }

        // Build the executable.
        exeBuilder.Build(exe);
    }

    // Copy in any files from the "files:" section of the .adef.
    for (auto& fileMapping : App.IncludedFiles())
    {
        CopyToStaging(fileMapping.m_SourcePath, stagingDirPath, fileMapping.m_DestPath);
    }

    // Generate the configuration.
    GenerateSupervisorConfig(stagingDirPath);

    // TODO: Generate the application's configuration tree (containing all its pool sizes,
    //       and anything else listed under the "config:" section of the .adef.)

    // TODO: Copy in the metadata (.adef and Component.cdef) files so they can be retrieved
    //       by Developer Studio.

    // Zip it all up.
    std::string outputPath = legato::CombinePath(OutputDir, App.Name() + "." + BuildParams.Target());
    if (!legato::IsAbsolutePath(outputPath))
    {
        outputPath = legato::GetWorkingDir() + "/" + outputPath;
    }
    std::string tarCommandLine = "tar cjf \"" + outputPath + "\" -C \"" + stagingDirPath + "\" .";
    if (BuildParams.IsVerbose())
    {
        std::cout << "Packaging application into '" << outputPath << "'." << std::endl;
        std::cout << std::endl << "$ "<< tarCommandLine << std::endl << std::endl;
    }
    mk::ExecuteCommandLine(tarCommandLine);
}



//--------------------------------------------------------------------------------------------------
/**
 * Put everything together into a system bundle.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSystemBundle
(
    void
)
//--------------------------------------------------------------------------------------------------
{

    std::string outputPath = legato::CombinePath(OutputDir, App.Name() + "." + BuildParams.Target());
    if (!legato::IsAbsolutePath(outputPath))
    {
        outputPath = legato::GetWorkingDir() + "/" + outputPath;
    }
    std::string tarCommandLine = "tar cjf \"" + outputPath + "\" -C \"" + stagingDirPath + "\" .";
    if (BuildParams.IsVerbose())
    {
        std::cout << "Packaging application into '" << outputPath << "'." << std::endl;
        std::cout << std::endl << "$ " << tarCommandLine << std::endl << std::endl;
    }
    mk::ExecuteCommandLine(tarCommandLine);
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the mksys functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeSystem
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    GetCommandLineArgs(argc, argv);

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    // Parse the .sdef file, populating the System object with the results.
    legato::parser::ParseSystem(System, BuildParams);

    // For each app in the system, make sure the app is built and output into the system
    // staging area.
    for (auto& mapEntry : System.Applications())
    {
        auto& app = mapEntry.second;

        BuildApp(app);
    }

    // Generate the application configuration overrides.
    GenerateAppConfigOverrides();

    // Zip up all the apps together with the configuration overrides and the .sdef file to make
    // the system bundle.
    GenerateSystemBundle();
}
