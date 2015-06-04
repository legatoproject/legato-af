//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the "mk" tool, which implements all of "mkcomp", "mkexe", "mkapp", and "mksys".
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
        else
        {
            std::cerr << "*** ERROR: unknown command name '" << fileName << "'." << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (std::runtime_error& e)
    {
        std::cerr << "** ERROR:" << std::endl << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
