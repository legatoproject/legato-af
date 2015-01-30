# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------


find_program(FLEX flex DOC "Path to the Flex scanner generator." COMMENT "Finding flex.")
find_program(BISON bison DOC "Path to the Bison parser generator." COMMENT "Finding bison.")


message("-- Flex found at ${FLEX}.")
message("-- Bison found at ${BISON}.")

remove_definitions(-Wall -Werror)

# Generate .cdef parser

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ComponentParser.tab.c
                   DEPENDS ComponentParser.y
                   COMMENT "Generating .cdef file parser."
                   COMMAND ${BISON} -pcyy_ -d -o ${CMAKE_CURRENT_BINARY_DIR}/ComponentParser.tab.c ${CMAKE_CURRENT_SOURCE_DIR}/ComponentParser.y
                   VERBATIM)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lex.cyy.c
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ComponentParser.l
                   COMMENT "Generating .cdef lexical scanner."
                   COMMAND ${FLEX} -Pcyy_ --header-file=${CMAKE_CURRENT_BINARY_DIR}/lex.cyy.h -o ${CMAKE_CURRENT_BINARY_DIR}/lex.cyy.c ${CMAKE_CURRENT_SOURCE_DIR}/ComponentParser.l
                   VERBATIM)

# Generate .adef parser

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ApplicationParser.tab.c
                   DEPENDS ApplicationParser.y
                   COMMENT "Generating .adef file parser."
                   COMMAND ${BISON} -payy_ -d -o ${CMAKE_CURRENT_BINARY_DIR}/ApplicationParser.tab.c ${CMAKE_CURRENT_SOURCE_DIR}/ApplicationParser.y
                   VERBATIM)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lex.ayy.c
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ApplicationParser.l
                   COMMENT "Generating .adef lexical scanner."
                   COMMAND ${FLEX} -Payy_ --header-file=${CMAKE_CURRENT_BINARY_DIR}/lex.ayy.h -o ${CMAKE_CURRENT_BINARY_DIR}/lex.ayy.c ${CMAKE_CURRENT_SOURCE_DIR}/ApplicationParser.l
                   VERBATIM)

# Generate .sdef parser

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SystemParser.tab.c
                   DEPENDS SystemParser.y
                   COMMENT "Generating .adef file parser."
                   COMMAND ${BISON} -psyy_
                                    -d
                                    -o ${CMAKE_CURRENT_BINARY_DIR}/SystemParser.tab.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/SystemParser.y
                   VERBATIM)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lex.syy.c
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/SystemParser.l
                   COMMENT "Generating .adef lexical scanner."
                   COMMAND ${FLEX}  -Psyy_
                                    --header-file=${CMAKE_CURRENT_BINARY_DIR}/lex.syy.h
                                    -o ${CMAKE_CURRENT_BINARY_DIR}/lex.syy.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/SystemParser.l
                   VERBATIM)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ComponentModel)

add_library(Parser
        ParserCommon.cpp
        # .cdef
        ComponentParser.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/lex.cyy.c
        ${CMAKE_CURRENT_BINARY_DIR}/ComponentParser.tab.c
        # .adef
        ApplicationParser.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/lex.ayy.c
        ${CMAKE_CURRENT_BINARY_DIR}/ApplicationParser.tab.c
        # .sdef
        SystemParser.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/lex.syy.c
        ${CMAKE_CURRENT_BINARY_DIR}/SystemParser.tab.c
        )

