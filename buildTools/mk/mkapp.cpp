//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkapp" functionality of the "mk" tool.
 *
 *  Run 'mkapp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
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
#include "mkapp.h"
#include "ComponentBuilder.h"
#include "ExecutableBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Component Builder and the Executable Builder objects
/// when they are created.
static legato::BuildParams_t BuildParams;

/// Path to the directory into which the final, built application file should be placed.
static std::string OutputDir;

/// The root object for this application's object model.
static legato::App App;

/// Map of process names to debug port numbers.
static std::map<std::string, uint16_t> DebugProcMap;

/// Set of debug port numbers that are in use.  This is used to check for duplicates,
/// since multiple processes can't share the same port number.
static std::set<uint16_t> DebugPortSet;


#if 0
//--------------------------------------------------------------------------------------------------
/**
 *  .
 */
//--------------------------------------------------------------------------------------------------
static std::string GetIfHash
(
    const std::list<std::string>& searchDirs,  ///
    const std::string& name,                   ///
    const bool isVerbose
)
{
    std::string path = FindFile(name, searchDirs);

    if (path.empty())
    {
        throw std::runtime_error("Could not find interface file, '" + name + "'.");
    }

    std::stringstream commandLine;

    static const size_t bufferSize = 256;
    char buffer[bufferSize] = { 0 };


    commandLine << "ifgen --hash " << path;

    if (isVerbose)
    {
        std::cout << commandLine << std::endl;
    }

    FILE* output = popen(commandLine.str().c_str(), "r");

    if (output == NULL)
    {
        throw std::runtime_error("Could not exec ifgen to generate an interface hash.");
    }

    if (fgets(buffer, bufferSize, output) != buffer)
    {
        throw std::runtime_error("Failed to receive the interface hash from ifgen.");
    }
    pclose(output);

    std::string hash = buffer;

    if (hash.back() == '\n')
    {
        hash.erase(hash.length() - 1);
    }

    if (isVerbose)
    {
        std::cout << "Interface '" << path << "' has hash " << hash << std::endl;
    }

    return hash;
}


#endif


//--------------------------------------------------------------------------------------------------
/**
 * Process a --debug (-g) command-line argument.
 **/
//--------------------------------------------------------------------------------------------------
static void HandleDebugArg
(
    void*       contextPtr, ///< Not used.
    const char* procAndPort ///< [in] Optional string containing a process name and a port number,
                            ///       separated by a colon (e.g., "foo:1234").
)
//--------------------------------------------------------------------------------------------------
{
    // Put the app into debug mode.
    App.SetDebug();

    // If the argument is not empty, then parse out the process name and the port number and
    // store them in the DebugProcessMap.
    std::string arg(procAndPort);
    if (!arg.empty())
    {
        size_t colonPos = arg.find(':');
        if ((colonPos == arg.npos) || (colonPos == 0))
        {
            std::cerr << "**ERROR: Invalid content for --debug (-g) argument.  Expected process"
                         " name and port number, separated by a colon (:)." << std::endl;
            exit(EXIT_FAILURE);
        }

        std::string procName = arg.substr(0, colonPos);

        std::string portNumStr(arg.substr(colonPos + 1));
        std::istringstream portNumStream(portNumStr);
        long int portNum;
        portNumStream >> portNum;   // Convert the string into a number.
        // If the conversion failed or there are unconverted bytes left at the end, then
        // it's an error.
        if (portNumStream.fail() || (portNumStream.eof() == false))
        {
            std::cerr << "**ERROR: Invalid port number '" << portNumStr
                      << "' for --debug (-g) argument." << std::endl;
            exit(EXIT_FAILURE);
        }
        // Port numbers must be greater than zero and no larger than 64K.
        if ((portNum <= 0) || (portNum > 65535))
        {
            std::cerr << "**ERROR: Port number " << portNum
                      << " out of range in --debug (-g) argument." << std::endl;
            exit(EXIT_FAILURE);
        }
        // Port numbers lower than 1024 are reserved for special services, like HTTP, NFS, etc.
        if (portNum < 1024)
        {
            std::cerr << "WARNING: Port number (" << portNum
                      << ") less than 1024 in --debug (-g) argument may not be permitted"
                         " by the target OS." << std::endl;
        }

        DebugProcMap[procName] = portNum;

        // Check for duplicate port numbers.
        if (DebugPortSet.insert(portNum).second == false)
        {
            std::cerr << "**ERROR: Debug port number " << portNum << " is used more than once."
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};


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

    // Lambda function that gets called once for each occurence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path) { BuildParams.AddInterfaceDir(path); };

    // Lambda function that gets called once for each occurence of the source/component search path
    // argument on the command line.
    auto compPathPush = [&](const char* path) { BuildParams.AddComponentDir(path); };

    // Lambda function that gets called once for each occurence of a .adef file name on the
    // command line.
    auto adefFileNameSet = [&](const char* param)
            {
                static bool matched = false;
                if (matched)
                {
                    throw legato::Exception("Only one app definition (.adef) file allowed.");
                }
                matched = true;
                App.DefFilePath(param);
            };

    // TODO: Change the Args module to display a "USAGE: ..." line and optionally, a SYNOPSIS.
    // TODO: le_arg_AddHelpSynopsis("Blah blah blah...");

    le_arg_AddOptionalString(&OutputDir,
                             ".",
                             'o',
                             "output-dir",
                             "Specify the directory into which the final, built application file"
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
                             "Add a directory to the component search path.",
                             compPathPush);

    le_arg_AddMultipleString('g',
                             "debug",
                             "Start the app in debug mode and specify a process to debug and a port"
                             " number for gdbserver to use.  e.g., '--debug=myProc:1234'.",
                             HandleDebugArg,
                             NULL);

    le_arg_AddOptionalString(&target,
                             "localhost",
                             't',
                             "target",
                             "Set the compile target (localhost|ar7).");

    le_arg_AddOptionalFlag(&isVerbose,
                           'v',
                           "verbose",
                           "Set into verbose mode for extra diagnostic information.");

    le_arg_AddOptionalString(&cFlags,
                             "",
                             'C',
                             "cflags",
                             "Specify extra flags to be passed to the C compiler.");

    le_arg_AddOptionalString(&ldFlags,
                             "",
                             'L',
                             "ldflags",
                             "Specify extra flags to be passed to the linker when linking "
                             "executables.");

    // Any remaining parameters on the command-line are treated as the .adef file path.
    // Note: there should only be one parameter not prefixed by an argument identifier.
    le_arg_SetLooseParamHandler(adefFileNameSet);

    le_arg_Scan(argc, argv);

    // Were we given an application definition?
    if (App.DefFilePath() == "")
    {
        throw std::runtime_error("An application definition must be supplied.");
    }

    // If we were not given an object file directory (intermediate build output directory) path,
    // use a subdirectory of the current working directory.
    if (objectFilesDir == "")
    {
        objectFilesDir = "./_build_" + App.Name() + "/" + target;
    }
    BuildParams.ObjOutputDir(objectFilesDir);

    // Add the directory containing the .adef file to the list of component search directories
    // and the list of interface search directories.
    std::string appDefFileDir = legato::GetContainingDir(App.DefFilePath());
    BuildParams.AddComponentDir(appDefFileDir);
    BuildParams.AddInterfaceDir(appDefFileDir);

    // Add the Legato framework include directory to the include path so people don't have
    // to keep doing it themselves.
    BuildParams.AddInterfaceDir("$LEGATO_ROOT/framework/c/inc");

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
    // Parse the .adef file and any Component.cdef files that it refers to.
    // This constructs the object model under the App object that we give it.
    legato::parser::ParseApp(App, BuildParams);

    // For every process in every process environment in the app, check if debugging has been
    // enabled (via command-line arguments), and if so, store the port number in the Process
    // object and flag the Executable object to be built for debugging.
    for (auto& procEnv : App.ProcEnvironments())
    {
        for (auto& process : procEnv.ProcessList())
        {
            const auto debugMapIter = DebugProcMap.find(process.Name());
            if (debugMapIter != DebugProcMap.end())
            {
                if (BuildParams.IsVerbose())
                {
                    std::cout << "Process '" << process.Name() << "' will be started in debug mode"
                              " using port number " << debugMapIter->second << "." << std::endl;
                }

                process.EnableDebugging(debugMapIter->second);

                auto exeMapIter = App.Executables().find(process.ExePath());
                if (exeMapIter != App.Executables().end())
                {
                    if (BuildParams.IsVerbose())
                    {
                        std::cout << "Executable '" << process.ExePath() << "' will be built for"
                                  " debugging (debug symbols included and optimization turned off)."
                                  << std::endl;
                    }

                    exeMapIter->second.EnableDebugging();
                }
                else
                {
                    std::cerr << "WARNING: executable '" << process.ExePath()
                              << "' is not built by this tool.  Please ensure that it has been"
                              " built with debug symbols." << std::endl;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy a file from the build host's file system to the application's staging directory.
 **/
//--------------------------------------------------------------------------------------------------
static void CopyToStaging
(
    const std::string& sourcePath,      ///< Build host file system path to source file.
    const std::string& stagingDirPath,  ///< Build host file system path to staging directory.
    const std::string& sandboxPath      ///< Must be an absolute path in the app's runtime sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    // Generate the destination path in the build host's file system.
    std::string destPath = stagingDirPath + sandboxPath;

    // First we have to make sure that the directory exists in the staging area before trying to
    // copy the file into it.
    auto pos = destPath.rfind('/');
    legato::MakeDir(destPath.substr(0, pos));

    // Now construct the copy command line and do the copy.
    std::string copyCommand = "cp \"";
    copyCommand += sourcePath;
    copyCommand += "\" \"";
    copyCommand += stagingDirPath + sandboxPath;
    copyCommand += "\"";
    if (BuildParams.IsVerbose())
    {
        std::cout << "Including file '" << sourcePath
                  << "' in the application bundle." << std::endl;
        std::cout << copyCommand << std::endl;
    }
    mk::ExecuteCommandLine(copyCommand);
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
                                "/usr/local/lib/liblegato.so"
                            };
    legato::FileMapping mapping({ legato::PERMISSION_READABLE, "", "/lib/" });
    for (size_t i = 0; i < (sizeof(libList) / sizeof(libList[0])); i++)
    {
        mapping.m_SourcePath = libList[i];
        GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
    }

    // Import the files specified in the .adef file.
    for (const auto& mapping : App.ImportedFiles())
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

    // Import all the files specified for all the components.
    for (const auto& pair : App.ComponentMap())
    {
        for (const auto& mapping : pair.second.ImportedFiles())
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
        }

        // Included files also need to be imported into the application sandbox.
        // But, the import is from a point relative to the app's install directory.
        // TODO: Allow the destination file name to be different than the source file name?
        for (const auto& mapping : pair.second.IncludedFiles())
        {
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
    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    // Create an Executable Builder object and a Component Builder object for later use.
    ExecutableBuilder_t exeBuilder(BuildParams);
    ComponentBuilder_t componentBuilder(BuildParams);

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


    // For each executable,
    auto& exeList = App.Executables();
    for (auto i = exeList.begin(); i != exeList.end(); i++)
    {
        legato::Executable& exe = (*i).second;

        // Auto-generate the source code file containing main() and add it to the default component.
        exeBuilder.GenerateMain(exe);
    }

    // For each component in the application.
    auto& map = App.ComponentMap();
    for (auto& i : map)
    {
        // Generate the IPC import/export code.
        componentBuilder.GenerateInterfaceCode(i.second);

        // Build the component.
        componentBuilder.Build(i.second);

        // Copy all included files into the staging area.
        auto& includedFilesList = i.second.IncludedFiles();
        for (auto fileMapping : includedFilesList)
        {
            CopyToStaging(fileMapping.m_SourcePath, stagingDirPath, fileMapping.m_DestPath);
        }
    }

    // Do the final build step for all the executables.
    // Note: All the components need to be built before this.
    for (auto i = exeList.begin(); i != exeList.end(); i++)
    {
        legato::Executable& exe = (*i).second;

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
        std::cout << tarCommandLine << std::endl;
    }
    mk::ExecuteCommandLine(tarCommandLine);
}



//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkapp functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeApp
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    GetCommandLineArgs(argc, argv);

    ConstructObjectModel();

    Build();
}
