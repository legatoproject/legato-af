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
              << "// Include the default legato system and the platform services." << "\n"
              << "#include $LEGATO_ROOT/default.sdef" << "\n";
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
                 "buildVars:\n"
                 "{\n"
                 "    // Declare custom variables in the build tool's process environment at build"
                 " time\n"
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
                 "interfaceSearch:\n"
                 "{\n"
                 "    // Directories where interfaces are searched\n"
                 "    ${MY_SYSTEM_DIR}/interfaces\n"
                 "    ${LEGATO_ROOT}/interfaces \n"
                 "}\n";

    defStream << "\n"
                 "appSearch:\n"
                 "{\n"
                 "    // Directories where apps are searched\n"
                 "    ${MY_SYSTEM_DIR}/apps\n"
                 "}\n";

    defStream << "\n"
                 "componentSearch:\n"
                 "{\n"
                 "    // Directories where components are searched\n"
                 "    ${MY_SYSTEM_DIR}/components\n"
                 "}\n";

    defStream << "\n"
                 "moduleSearch:\n"
                 "{\n"
                 "    // Directories where kernel modules are searched\n"
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
                 "apps:\n"
                 "{\n"
                 "    // Applications\n"
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
                 "kernelModules:\n"
                 "{\n"
                 "    // Kernel modules\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header for sdef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateSdefTemplate
(
    ArgHandler_t handler
)
{
    std::string filePath = path::MakeAbsolute(handler.sdefFilePath);

    std::ofstream defStream(filePath, std::ofstream::trunc);

    defStream << "// " << path::GetLastNode(handler.sdefFilePath) << "\n"
                 "// This is a system definition file which defines and interconnects a system of"
                 " one or more applications\n"
                 "// with the target's run-time environment." << "\n"
                 "// For more details on the system definition (.sdef) file format see:" << "\n"
                 "// https://docs.legato.io/latest/defFilesSdef.html\n";

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
                 "components:\n"
                 "{\n"
                 "    // Components for this application\n"
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
    ArgHandler_t handler
)
{
    std::string compName;
    std::string compPath;

    compPath = handler.absCdefFilePath;
    compName = path::GetLastNode(compPath);

    // If componentSearch section is present, list the component name (no need to specify full path)
    if (handler.isCompSearchPath)
    {
        compPath = compName;
    }
    else
    {
        std::string adefContainDir = path::GetContainingDir(handler.absAdefFilePath);

        std::size_t foundAdefContainDir = compPath.find(adefContainDir);
        if (foundAdefContainDir != std::string::npos)
        {
            compPath.erase(foundAdefContainDir, adefContainDir.length()+1);
        }
    }

    std::string exeName = compName + "Exe";

    defStream << "\n"
                 "processes:\n"
                 "{\n"
                 "    // Processes to run when the app is started\n"
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
    ArgHandler_t handler
)
{
    std::string compName;
    std::string compPath;

    compPath = handler.absCdefFilePath;
    compName = path::GetLastNode(compPath);

    // If componentSearch section is present, list the component name (no need to specify full path)
    if (handler.isCompSearchPath)
    {
        compPath = compName;
    }
    else
    {
        std::string adefContainDir = path::GetContainingDir(handler.absAdefFilePath);

        std::size_t foundAdefContainDir = compPath.find(adefContainDir);
        if (foundAdefContainDir != std::string::npos)
        {
            compPath.erase(foundAdefContainDir, adefContainDir.length()+1);
        }
    }

    defStream << "\n"
                 "executables:\n"
                 "{\n"
                 "    // Executables which later is added to the bin directory of the app\n";
    defStream << "    " << compName
              <<"Exe = ( " << compPath << " )\n";

    defStream << "}\n";
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
                 "bindings:\n"
                 "{\n"
                 "    // Bindings that allow client side API interfaces to be bound to the server"
                 " side interfaces\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header and other sections for adef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateAdefTemplate
(
    ArgHandler_t handler
)
{
    if (file::FileExists(handler.absAdefFilePath))
    {
        throw mk::Exception_t(
               mk::format(LE_I18N("Application definition file already exists: '%s'"),
                          handler.absAdefFilePath)
        );
    }

    file::MakeDir(path::GetContainingDir(handler.absAdefFilePath));

    std::ofstream defStream(handler.absAdefFilePath, std::ofstream::trunc);

    defStream << "// " << handler.adefFilePath << "\n"
                 "// This is an application definition file that specifies the internal content of"
                 " application and external interfaces\n"
                 "// For more details on the application definition (.adef) file format see:\n"
                 "// https://docs.legato.io/latest/defFilesAdef.html\n";

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
                 "requires:\n"
                 "{\n"
                 "    api:\n"
                 "    {\n"
                 "        // IPC APIs used by this component\n"
                 "    }\n"
                 "    \n"
                 "    file:\n"
                 "    {\n"
                 "        // File paths\n"
                 "    }\n"
                 "    \n"
                 "    device:\n"
                 "    {\n"
                 "        // Device paths\n"
                 "    }\n"
                 "    \n"
                 "    dir:\n"
                 "    {\n"
                 "        // Directories on target device to make accessible to the app\n"
                 "    }\n"
                 "    \n"
                 "    lib:\n"
                 "    {\n"
                 "        // Shared libraries required by any component\n"
                 "    }\n"
                 "}\n";

}


//--------------------------------------------------------------------------------------------------
/**
 * Print header for cdef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateCdefTemplate
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
    }
    else
    {
        sourceFileName = path::GetLastNode(handler.absCdefFilePath) + ".c";
        sourceFilePath = handler.absCdefFilePath + "/"+ sourceFileName;
        compFilePath = handler.absCdefFilePath + "/" + COMP_CDEF;
    }

    file::MakeDir(path::GetContainingDir(sourceFilePath));

    std::ofstream sourceDefStream(sourceFilePath, std::ofstream::trunc);

    sourceDefStream << "/* " << sourceFileName  << " */\n\n"
                       "#include \"legato.h\"\n"
                       "#include \"interfaces.h\"\n\n"
                       "COMPONENT_INIT\n"
                       "{\n"
                       "    LE_INFO(\"Component " << path::RemoveSuffix(sourceFileName, ".c")
                                                  << " started.\");\n"
                       "}";

    file::MakeDir(path::GetContainingDir(compFilePath));

    std::ofstream defStream(compFilePath, std::ofstream::trunc);

    defStream << "// \n"
                 "// This is component definition file that specifies the internal content and"
                 " external interfaces\n"
                 "// of reusable software components." << "\n"
                 "// For more details on the component definition (.cdef) file format see:" << "\n"
                 "// https://docs.legato.io/latest/defFilesCdef.html\n";
    defStream << "\n"
                 "sources:\n"
                 "{\n"
                 "    // Source code files\n"
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
                 "load: auto\n";
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
                 "sources:\n"
                 "{\n"
                 "    // Path of the source code files to build the kernel module\n"
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
                 "scripts:\n"
                 "{\n"
                 "    // Provide path to the installation and removal scripts\n"
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
                 "requires:\n"
                 "{\n"
                 "    kernelModules:\n"
                 "    {\n"
                 "        // Kernel modules which this module is dependent on\n"
                 "    }\n"
                 "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print header for mdef
 **/
//--------------------------------------------------------------------------------------------------
void GenerateMdefTemplate
(
    ArgHandler_t handler
)
{
    std::string filePath = handler.absMdefFilePath;

    std::ofstream defStream(filePath, std::ofstream::trunc);

    defStream << "// " << path::GetLastNode(handler.mdefFilePath) << "\n"
                 "// This is a module definition file that declares kernel modules to be bundled"
                 " with Legato.\n"
                 "// For more details on the module definition (.mdef) file format see:" << "\n"
                 "// https://docs.legato.io/latest/defFilesMdef.html\n";

    GenerateLoadSection(defStream);
    GenerateSourcesModuleSection(defStream);
    GenerateScriptsSection(defStream);
    GenerateRequiresModuleSection(defStream);
}



} // namespace defs

} // namespace cli
