//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building Executables.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "ExecutableBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Generates a main .c for the executable.
 */
//--------------------------------------------------------------------------------------------------
void ExecutableBuilder_t::GenerateMain
(
    legato::Executable& executable
)
//--------------------------------------------------------------------------------------------------
{
    // Generate the path to the directory in which the generated files will go.
    std::string path = m_Params.ObjOutputDir()
                     + "/"
                     + executable.CName();

    // Make sure the directory exists.
    legato::MakeDir(path);

    // Add "/_main.c" to get the path of the file to generate code into.
    path += "/_main.c";

    if (m_Params.IsVerbose())
    {
        std::cout << "Generating startup code for '" << executable.OutputPath()
                  << "' in '" << path << "'." << std::endl;
    }

    // Open the file as an output stream.
    std::ofstream outputFile(path);

    if (outputFile.is_open() == false)
    {
        throw legato::Exception("Could not open, '" + path + ",' for writing.");
    }

    // The code is broken down into multiple secions.  This is to make writing the code
    // generator a little easier.  We just have to stream the various parts into the
    // required sections, so we only need to loop over our data structures once.
    std::stringstream compInitDeclBuffer,
                      sessionBuffer,
                      filterBuffer,
                      logInitBuffer,
                      compInitBuffer;

    // Make sure that the various sections are commented to make the generated code a little
    // clearer.
    compInitDeclBuffer<< "// Declare all component initializers." << std::endl;
    sessionBuffer     << "// Declare component log session variables." << std::endl;
    filterBuffer      << "// Declare log filter level pointer variables." << std::endl;
    logInitBuffer     << "    // Initialize all log sessions." << std::endl;
    compInitBuffer    << "    // Schedule component initializers for execution by the event loop."
                      << std::endl;

    // Iterate over the list of Component Instances, in reverse order so that initialization
    // happens in the correct order (lower-level stuff gets initialized before the higher-level
    // stuff that uses it).
    // TODO: Do proper sorting of the initialization functions based on dependencies.
    // TODO: Check for dependency loops and halt the build (or find a tricky way to break the loop).
    auto componentList = executable.ComponentInstanceList();
    for (auto i = componentList.crbegin(); i != componentList.crend(); i++)
    {
        const legato::Component& component = i->GetComponent();
        auto name = component.CName();

        // Define the component's Log Session Reference variable.
        sessionBuffer  << "le_log_SessionRef_t " << name << "_LogSession;" << std::endl;

        // Define the component's Log Filter Level variable.
        filterBuffer   << "le_log_Level_t* " << name << "_LogLevelFilterPtr;" << std::endl;

        // Register the component with the Log Control Daemon.
        logInitBuffer  << "    " << name << "_LogSession = log_RegComponent(\""
                       << name << "\", &" << name << "_LogLevelFilterPtr);"
                       << std::endl;

        // Generate forward declaration of the component initializer.
        compInitDeclBuffer << "void " << mk::GetComponentInitName(component) << "(void);"
                           << std::endl;

        // Queue up the component initializer to be called when the Event Loop starts.
        compInitBuffer << "    event_QueueComponentInit(" << mk::GetComponentInitName(component)
                       << ");" << std::endl;
    }

    // Now do the above for the default component.
    const legato::Component& defaultComponent = executable.DefaultComponent();
    {
        auto name = defaultComponent.CName();

        // Define the component's Log Session Reference variable.
        sessionBuffer  << "le_log_SessionRef_t " << name << "_LogSession;" << std::endl;

        // Define the component's Log Filter Level variable.
        filterBuffer   << "le_log_Level_t* " << name << "_LogLevelFilterPtr;" << std::endl;

        // Register the component with the Log Control Daemon.
        logInitBuffer  << "    " << name << "_LogSession = log_RegComponent(\""
                       << name << "\", &" << name << "_LogLevelFilterPtr);"
                       << std::endl;

        // If the Default Component has at least one source file (besides the one we are
        // auto-generating right now),
        if (!(defaultComponent.CSourcesList().empty()))
        {
            std::string componentInitFuncName = mk::GetComponentInitName(defaultComponent);
            // Generate forward declaration of the component initializer.
            compInitDeclBuffer << "void " << componentInitFuncName << "(void);" << std::endl;

            // Queue up the component initializer to be called when the Event Loop starts.
            compInitBuffer << "    event_QueueComponentInit(" << componentInitFuncName << ");"
                           << std::endl;
        }
    }

    // Now that we have all of our subsections filled out, dump all of the generated code
    // and the static template code to the target output file.
    outputFile  << std::endl
                << "// Startup code for the executable '" << executable.OutputPath() << "'."
                << std::endl
                << "// This is a generated file, do not edit." << std::endl
                << std::endl
                << "#include \"legato.h\"" << std::endl
                << "#include \"../src/eventLoop.h\"" << std::endl
                << "#include \"../src/log.h\"" << std::endl
                << "#include \"../src/args.h\"" << std::endl

                << std::endl
                << std::endl

                << compInitDeclBuffer.str() << std::endl

                << sessionBuffer.str() << std::endl

                << filterBuffer.str() << std::endl
                << std::endl
                << std::endl

                << "int main(int argc, char* argv[])" << std::endl
                << "{" << std::endl
                << "    // Gather the program arguments for later processing." << std::endl
                << "    arg_SetArgs((size_t)argc, (char**)argv);" << std::endl
                << std::endl

                << logInitBuffer.str() << std::endl
                << "    // Connect to the log control daemon." << std::endl
                << "    log_ConnectToControlDaemon();" << std::endl
                << std::endl
                << "    LE_DEBUG(\"== Log sessions registered. ==\");" << std::endl
                << std::endl

                << "    // TODO: Load configuration." << std::endl
                << "    // TODO: Create configured memory pools." << std::endl
                << std::endl

                // Queue component initialization functions for execution by the event loop.
                << compInitBuffer.str() << std::endl
                << std::endl

                << "    LE_DEBUG(\"== Starting Event Processing Loop ==\");" << std::endl
                << "    le_event_RunLoop();" << std::endl
                << "    LE_FATAL(\"== SHOULDN'T GET HERE! ==\");" << std::endl
                << "}" << std::endl;

    // Add the generated file to the list of C source code files to be compiled into this
    // executable (as part of its "default" component).
    executable.AddCSourceFile(path);
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds the source files in the executable's default component and links everything together to
 * create the executable file.
 */
//--------------------------------------------------------------------------------------------------
void ExecutableBuilder_t::Build
(
    const legato::Executable& executable
)
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;
    std::string outputPath;
    const legato::Component& defaultComponent = executable.DefaultComponent();

    // If the executable file path is a relative path, then it is relative to the
    // executable output directory.
    if (! legato::IsAbsolutePath(executable.OutputPath()))
    {
        outputPath = m_Params.ExeOutputDir() + "/";
    }
    outputPath += executable.OutputPath();

    if (m_Params.IsVerbose())
    {
        std::cout << "Compiling and linking executable '" << outputPath << "'." << std::endl;
    }

    auto instanceList = executable.ComponentInstanceList();

    // Specify the compiler command and the output file path.
    commandLine << mk::GetCompilerPath(m_Params.Target()) <<" -o " << outputPath;

    // Turn on all warnings and treat them as errors.
    commandLine << " -Wall"
                << " -Werror";

    // If the executable is going to be debugged using gdb, turn off optimizations and turn
    // on debug symbols in the output.
    if (executable.IsDebuggingEnabled())
    {
        commandLine << " -g -O0";
    }

    // Add the CFLAGS to the command-line.
    commandLine << " " << m_Params.CCompilerFlags();

    // Add the include paths specified on the command-line.
    for (auto i : m_Params.InterfaceDirs())
    {
        commandLine << " -I" << i;
    }

    // Add the include paths specific to the default component.
    for (auto i : defaultComponent.IncludePath())
    {
        commandLine << " -I" << i;
    }

    // Define the component name, log session variable, and log filter variable for the default
    // component.
    commandLine << " -DLEGATO_COMPONENT=" << defaultComponent.CName();
    commandLine << " -DLE_LOG_SESSION=" << defaultComponent.Name() << "_LogSession ";
    commandLine << " -DLE_LOG_LEVEL_FILTER_PTR=" << defaultComponent.Name() << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT for the default component.
    commandLine << " \"-DCOMPONENT_INIT=void " << mk::GetComponentInitName(defaultComponent) << "()\"";

    // Add all the default component's source files to the command-line (compile them now).
    // TODO: Change to pre-compile the .c files to .o files and then just include the .o files here.
    for (const auto& sourceFile : defaultComponent.CSourcesList())
    {
        commandLine << " \"" << sourceFile << "\"";
    }

    // Add the library output directory as a library search directory.
    for (auto instance : instanceList)
    {
        commandLine << " -L" << m_Params.LibOutputDir();
    }

    // Link with each component instance's component library.
    for (auto i : instanceList)
    {
        const auto& component = i.GetComponent();
        commandLine << " -l" << component.Name();
    }

    // Link with other libraries that are needed by the default component.
    for (const auto& lib : defaultComponent.LibraryList())
    {
        commandLine << " \"" << lib << "\"";
    }

    // Link with the Legato C runtime library.
    commandLine << " -L$LEGATO_ROOT/build/" << m_Params.Target() << "/bin/lib" << " -llegato";

    // Link with other libraries added to components included in this executable.
    for (auto i : instanceList)
    {
        auto libList = i.GetComponent().LibraryList();
        for (auto lib : libList)
        {
            commandLine << " \"" << lib << "\"";
        }
    }

    // Link with the real-time library and the pthreads library, just in case they're needed too.
    commandLine << " -lpthread -lrt";

    // Insert LDFLAGS on the command-line.
    commandLine << " " << m_Params.LinkerFlags();

    // On the localhost, set the DT_RUNPATH variable inside the executable to include the
    // expected locations of the libraries needed.
    if (m_Params.Target() == "localhost")
    {
        commandLine << " -Wl,--enable-new-dtags"
                    << ",-rpath=\"\\$ORIGIN/../lib:$LEGATO_LOCALHOST_LIB_PATH";
        // TODO: Add filtered (duplicates removed) list of component library search paths?
        commandLine <<"\"";
    }
    // On embedded targets, set the DT_RUNPATH variable inside the executable to include the
    // expected location of libraries bundled in this application (this is needed for unsandboxed
    // applications).
    else
    {
        commandLine << " -Wl,--enable-new-dtags,-rpath=\"\\$ORIGIN/../lib\"";
    }

    // Run the command.
    if (m_Params.IsVerbose())
    {
        std::cout << commandLine.str() << std::endl;
    }
    mk::ExecuteCommandLine(commandLine);
}


