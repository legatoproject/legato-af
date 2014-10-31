//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the "mk" tool, which actually implements both 'mkexe' and 'mkapp'.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdexcept>

#include "LegatoObjectModel.h"
//#include "mkif.h"
#include "mkcomp.h"
#include "mkexe.h"
#include "mkapp.h"
#include "mksys.h"

using namespace legato;

//--------------------------------------------------------------------------------------------------
/**
 * Program entry point.
 */
//--------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    try
    {
        std::string fileName = GetLastPathNode(argv[0]);

        if (fileName == "mkexe")
        {
            MakeExecutable(argc, argv);
        }
        else if (fileName == "mkcomp")
        {
            MakeComponent(argc, argv);
        }
        else if (fileName == "mkapp")
        {
            MakeApp(argc, argv);
        }
        else if (fileName == "mksys")
        {
            MakeSystem(argc, argv);
        }
        else
        {
            std::cerr << "*** ERROR: unknown command name '" << fileName << "'." << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (std::runtime_error& e)
    {
        std::cerr << "** ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
