//--------------------------------------------------------------------------------------------------
/**
 *  @file: generateDefTemplate.cpp
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <stdexcept>
#include <iostream>
#include <set>
#include <list>
#include <string>
#include <cstring>

#include "mkTools.h"
#include "commandLineInterpreter.h"

namespace cli
{

namespace defs
{

//--------------------------------------------------------------------------------------------------
/**
 * Print include defaultSdef
 **/
//--------------------------------------------------------------------------------------------------
static void IncludeDefaultSdef
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Directs mksys to build the default Legato system and all the default platform "
                 "services along\n"
                 "// with your apps and customizations.\n"
                 "#include $LEGATO_ROOT/default.sdef\n"
                 "\n"
                 "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print buildVars section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBuildVarsSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Declare custom variables available at build time for all of your project's "
                 "apps, components and\n"
                 "// modules.\n"
                 "//\n"
                 "// Variables are defined as <name> = <value>.  Values can come from previously "
                 "defined variables or\n"
                 "// from environment variables.\n"
                 "// These variables are available to use throughout the definition file. "
                 "buildVars section is \n"
                 "// evaluated before processing any other sections.\n"
                 "buildVars:\n"
                 "{\n"
                 "    // Define a base directory for your project.\n"
                 "    MY_SYSTEM_DIR = $CURDIR\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print interfaceSearch, appSearch, componentSearch, moduleSearch section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSearchSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Interfaces are searched for in the following directories. Each "
                 "directory/sub-directory needs to\n"
                 "// be identified here for the system to find and use .api files.\n"
                 "interfaceSearch:\n"
                 "{\n"
                 "    // Your project's search paths.\n"
                 "    ${MY_SYSTEM_DIR}/interfaces\n"
                 "\n"
                 "    // Legato API search paths.\n"
                 "    ${LEGATO_ROOT}/interfaces\n"
                 "    ${LEGATO_ROOT}/interfaces/airVantage\n"
                 "    ${LEGATO_ROOT}/interfaces/atServices\n"
                 "    ${LEGATO_ROOT}/interfaces/logDaemon\n"
                 "    ${LEGATO_ROOT}/interfaces/modemServices\n"
                 "    ${LEGATO_ROOT}/interfaces/portService\n"
                 "    ${LEGATO_ROOT}/interfaces/positioning\n"
                 "    ${LEGATO_ROOT}/interfaces/secureStorage\n"
                 "    ${LEGATO_ROOT}/interfaces/wifi\n"
                 "}\n";

    defStream << "\n"
                 "// Apps are searched for in the following directories. Each "
                 "directory/sub-directory needs to be\n"
                 "// identified here for the system to find and use the .adef files.\n"
                 "appSearch:\n"
                 "{\n"
                 "    ${MY_SYSTEM_DIR}/apps\n"
                 "}\n";

    defStream << "\n"
                 "// Directories where components are searched.\n"
                 "componentSearch:\n"
                 "{\n"
                 "    ${MY_SYSTEM_DIR}/components\n"
                 "}\n";

    defStream << "\n"
                 "// Kernel Modules are searched for in the following directories. Each "
                 "directory/sub-directory\n"
                 "// needs to be identified here for the system to find and use the .mdef files.\n"
                 "moduleSearch:\n"
                 "{\n"
                 "    ${MY_SYSTEM_DIR}/modules\n"
                 "}\n";

}


//--------------------------------------------------------------------------------------------------
/**
 * Print apps section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppsSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Add your project/system specific apps here. You just need to add the name as "
                 "long as the\n"
                 "// directory path is specified in the search path in the appSearch: section.\n"
                 "apps:\n"
                 "{\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print kernelModules section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateKernelModulesSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Add your project/system specific kernel modules here. You just need to add "
                 "the name as long as\n"
                 "// the directory path is specified in the search path in the moduleSearch: "
                 "section.\n"
                 "kernelModules:\n"
                 "{\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header for sdef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateSystemTemplate
(
    ArgHandler_t& handler
)
{
    std::string filePath = path::MakeAbsolute(handler.sdefFilePath);

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nGenerating SDEF file '%s'."), filePath);
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    std::ofstream defStream(filePath, std::ofstream::trunc);

    defStream << "\n// " << path::GetLastNode(handler.sdefFilePath) << "\n"
                 "//\n"
                 "// This is a system definition file which defines and interconnects a system of"
                 " one or more \n"
                 "// applications with the target's run-time environment.\n"
                 "//\n"
                 "// For more details on the system definition (.sdef) file format see:\n"
                 "// https://docs.legato.io/latest/defFilesSdef.html\n\n";

    IncludeDefaultSdef(defStream);
    GenerateBuildVarsSection(defStream);
    GenerateSearchSection(defStream);
    GenerateAppsSection(defStream);
    GenerateKernelModulesSection(defStream);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print components section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateComponentsSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Specify the bundled components for your application here. This section needs "
                 "to contain a list\n"
                 "// of system paths to your component directory.\n"
                 "components:\n"
                 "{\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print processes section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateProcessesSection
(
    std::ostream& defStream,
    ArgHandler_t& handler
)
{
    std::string compName;
    std::string exeName;

    compName = path::GetLastNode(handler.absCdefFilePath);
    exeName = compName + "Exe";

    defStream << "\n"
                 "// The processes section specifies processes to run when the app is started "
                 "including environment\n"
                 "// variables, command-line arguments, limits, and fault handling actions.\n"
                 "processes:\n"
                 "{\n"
                 "    run:\n"
                 "    {\n";
    defStream << "        ( " << exeName  << " )\n"
              << "    }\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print executables section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateExecutablesSection
(
    std::ostream& defStream,
    ArgHandler_t& handler
)
{
    std::string compName;
    std::string compPath;

    compName = path::GetLastNode(handler.absCdefFilePath);

    // If componentSearch section is present, list just the relative component name and no need to
    // specify full absolute path.
    for (const auto& it : handler.compSearchPath)
    {
        compPath = path::EraseCommonBasePath(handler.absCdefFilePath, it, false);
        if (!compPath.empty())
        {
            break;
        }
    }

    if (compPath.empty())
    {
        compPath = path::EraseCommonBasePath(handler.absCdefFilePath, handler.absAdefFilePath,
                                             true);
    }

    defStream << "\n"
                 "// Add the list of executables to be constructed and moved to the /bin "
                 "directory of the app. The\n"
                 "// executable content is a list of the components inside the executable.\n"
                 "//\n"
                 "// Example binding format:\n"
                 "// clientExe.clientComponent.clientInterface -> "
                 "serverExe.serverComponent.serverInterface\n"
                 "executables:\n"
                 "{\n"
                 "    " << compName
              << "Exe = ( " << compPath << " )\n"
              << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print bindings section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBindingsSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Bindings that allow client side API interfaces to be bound to servers found "
                 "within other\n"
                 "// applications in the system.\n"
                 "bindings:\n"
                 "{\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header and other sections for adef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateApplicationTemplate
(
    ArgHandler_t& handler
)
{
    if (file::FileExists(handler.absAdefFilePath))
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("Application definition file already exists: '%s'"),
                          handler.absAdefFilePath)
        );
    }

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nGenerating ADEF file '%s'."), handler.absAdefFilePath);
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }


    file::MakeDir(path::GetContainingDir(handler.absAdefFilePath));

    std::ofstream defStream(handler.absAdefFilePath, std::ofstream::trunc);

    defStream << "\n// " << handler.adefFilePath << "\n"
                 "//\n"
                 "// This is an application definition file that specifies the internal content of"
                 " application and\n"
                 "// external interfaces.\n"
                 "//\n"
                 "// For more details on the application definition (.adef) file format see:\n"
                 "// https://docs.legato.io/latest/defFilesAdef.html\n"
                 "\n";

    GenerateComponentsSection(defStream);
    GenerateExecutablesSection(defStream, handler);
    GenerateProcessesSection(defStream, handler);
    GenerateBindingsSection(defStream);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print requires section of cdef
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateRequiresSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Bind resources from the target module into your application.\n"
                 "requires:\n"
                 "{\n"
                 "    // IPC APIs used by this component.\n"
                 "    api:\n"
                 "    {\n"
                 "    }\n"
                 "    \n"
                 "    // File paths local to your target module.\n"
                 "    file:\n"
                 "    {\n"
                 "    }\n"
                 "    \n"
                 "    // Directories on your target module to make accessible to the app.\n"
                 "    dir:\n"
                 "    {\n"
                 "    }\n"
                 "    \n"
                 "    // Linux filesystem device paths.\n"
                 "    device:\n"
                 "    {\n"
                 "    }\n"
                 "    \n"
                 "    // Shared libraries pre-installed on the module.\n"
                 "    lib:\n"
                 "    {\n"
                 "    }\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header for cdef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateComponentTemplate
(
    ArgHandler_t& handler
)
{
    std::string adefFileName;
    std::string sourceFilePath;
    std::string compFilePath;
    std::string sourceFileName;

    adefFileName = path::GetLastNode(handler.adefFilePath);

    if (path::HasSuffix(adefFileName, ADEF_EXT))
    {
        adefFileName = path::RemoveSuffix(adefFileName, ADEF_EXT);
    }

    if (handler.absCdefFilePath.empty())
    {
        sourceFileName = adefFileName + "Component.c";
        // meaning the component name is not passed via the command line argument
        sourceFilePath = path::GetContainingDir(handler.absAdefFilePath) + "/" + adefFileName +
                         "Component/" + sourceFileName;
        compFilePath =  path::GetContainingDir(handler.absAdefFilePath) + "/" + adefFileName +
                        "Component/" + COMP_CDEF;
        handler.absCdefFilePath =  path::GetContainingDir(handler.absAdefFilePath) + "/" +
                                   adefFileName + "Component";
    }
    else
    {
        sourceFileName = path::GetLastNode(handler.absCdefFilePath) + ".c";
        sourceFilePath = handler.absCdefFilePath + "/"+ sourceFileName;
        compFilePath = handler.absCdefFilePath + "/" + COMP_CDEF;
    }

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("\nCreating component template files:"
                                        "\nSource file: '%s'."
                                        "\nCDEF file: '%s'."), sourceFilePath, compFilePath
                     );
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }

    file::MakeDir(path::GetContainingDir(sourceFilePath));

    std::ofstream sourceDefStream(sourceFilePath, std::ofstream::trunc);

    sourceDefStream << "\n// " << sourceFileName << "\n"
                       "//\n"
                       "// Specifies the main source file of the component. Add initialization "
                       "and event registrations to\n"
                       "// the files COMPONENT_INIT functions.\n"
                       "\n"
                       "\n"
                       "// Include the core framework C APIs.\n"
                       "#include \"legato.h\"\n\n"
                       "// Include your component's API interfaces.\n"
                       "#include \"interfaces.h\"\n\n\n"
                       "// This function is called only once on startup.  Place your initialization"
                       " and event registration\n"
                       "// here.\n"
                       "COMPONENT_INIT\n"
                       "{\n"
                       "    // Write in the log that this component has started.\n"
                       "    LE_INFO(\"Component " << path::RemoveSuffix(sourceFileName, ".c")
                                                  << " started.\");\n"
                       "}\n";

    file::MakeDir(path::GetContainingDir(compFilePath));

    std::ofstream defStream(compFilePath, std::ofstream::trunc);

    defStream << "\n// " << path::GetLastNode(path::GetContainingDir(compFilePath)) << "\n"
                 "//\n"
                 "// This is component definition file that specifies the internal content and"
                 " external interfaces\n"
                 "// of reusable software components." << "\n"
                 "//\n"
                 "// For more details on the component definition (.cdef) file format see:" << "\n"
                 "// https://docs.legato.io/latest/defFilesCdef.html\n\n"
                 "\n"
                 "// Source code files.\n"
                 "sources:\n"
                 "{\n"
                 "    " << sourceFileName << "\n"
                 "}\n";

    GenerateRequiresSection(defStream);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print load section of mdef
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateLoadSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Specifies the module is loaded automatically at system startup.\n"
                 "// Replace 'auto' with 'manual' to manually load the module.\n"
                 "load: auto\n"
                 "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print sources section of mdef
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSourcesModuleSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Path of the source code files to build the kernel module.\n"
                 "sources:\n"
                 "{\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print scripts section of mdef
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateScriptsSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Provide path to the installation and removal scripts.\n"
                 "scripts:\n"
                 "{\n"
                 "    //install:\n"
                 "    //remove:\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print requires kernelModules section
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateRequiresModuleSection
(
    std::ostream& defStream
)
{
    defStream << "\n"
                 "// Kernel modules which this module is dependent on.\n"
                 "requires:\n"
                 "{\n"
                 "    kernelModules:\n"
                 "    {\n"
                 "    }\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header for mdef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateModuleTemplate
(
    ArgHandler_t& handler
)
{
    if (file::FileExists(handler.absMdefFilePath))
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("\nModule definition file already exists: '%s'"),
                          handler.absMdefFilePath)
        );
    }

    if (handler.isPrintLogging())
    {
        std::cout << mk::format(LE_I18N("Generating MDEF file '%s'."), handler.absMdefFilePath);
    }

    if (handler.buildParams.isDryRun)
    {
        return;
    }


    file::MakeDir(path::GetContainingDir(handler.absMdefFilePath));

    std::ofstream defStream(handler.absMdefFilePath, std::ofstream::trunc);

    defStream << "\n// " << path::GetLastNode(handler.mdefFilePath) << "\n"
                 "//\n"
                 "// This is a module definition file that declares kernel modules to be bundled"
                 " with Legato.\n"
                 "//\n"
                 "// For more details on the module definition (.mdef) file format see:\n"
                 "// https://docs.legato.io/latest/defFilesMdef.html\n"
                 "\n";

    GenerateLoadSection(defStream);
    GenerateSourcesModuleSection(defStream);
    GenerateScriptsSection(defStream);
    GenerateRequiresModuleSection(defStream);
}


} // namespace defs

} // namespace cli
