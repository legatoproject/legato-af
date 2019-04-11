//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the "mk" tool, which implements all of "mkcomp", "mkexe", "mkapp", and "mksys".
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdexcept>

#include "mkTools.h"
#include "commandLineInterpreter.h"


//--------------------------------------------------------------------------------------------------
/**
 * Program entry point.
 */
//--------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    try
    {
        std::string fileName = path::GetLastNode(argv[0]);

        if (fileName == "mkexe")
        {
            cli::MakeExecutable(argc, argv);
        }
        else if (fileName == "mkcomp")
        {
            cli::MakeComponent(argc, argv);
        }
        else if (fileName == "mkapp")
        {
            cli::MakeApp(argc, argv);
        }
        else if (fileName == "mksys")
        {
            cli::MakeSystem(argc, argv);
        }
        else if (fileName == "mkedit")
        {
            cli::MakeEdit(argc, argv);
        }
	else if (fileName == "mkparse")
        {
            cli::MakeParsedModel(argc, argv);
        }
        else
        {
            std::cerr << mk::format(LE_I18N("** ERROR: unknown command name '%s'."), fileName)
                      << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (mk::Exception_t &e)
    {
        std::cerr << LE_I18N("** ERROR:") << std::endl
                  << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception& e)
    {
        std::cerr << LE_I18N("** ERROR:") << std::endl
                  << mk::format(LE_I18N("Internal error: %s"), e.what()) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
