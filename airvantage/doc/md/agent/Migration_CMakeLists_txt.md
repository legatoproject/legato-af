Migration Script C Example: the CMake build file
=========

~~~~{.cmake}
include_directories(
    ${COMMON_SOURCE_DIR})

add_lua_library(migration DESTINATION agent MigrationScript.c)
target_link_libraries(migration lib_swi_log)
~~~~

