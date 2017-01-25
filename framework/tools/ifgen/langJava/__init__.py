
# --------------------------------------------------------------------------------------------------
#
#  Initialization for the Java language generator.
#
#  Copyright (C) Sierra Wireless Inc.
#
# --------------------------------------------------------------------------------------------------

import commandLib
import codeTypes
import interfaceParser



# --------------------------------------------------------------------------------------------------
# Entry point for the host script to get our code generation API.
# --------------------------------------------------------------------------------------------------
def GetCommandLib():
    # Specify the codeTypes library for the interface parser to use, and return our implementation
    # of the command lib.
    interfaceParser.SetCodeTypeLibrary(codeTypes)

    return commandLib
