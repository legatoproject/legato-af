# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(parser
                parser.cpp
                lexer.cpp
                cdefParser.cpp
                adefParser.cpp
                sdefParser.cpp
                apiParser.cpp
           )

add_dependencies(parser
                    PrecompiledHeader
                )

