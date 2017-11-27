#
# Original program, force to cross-compile
#

include(CMakeForceCompiler)

add_executable(legacy-cross main.c)

# This is a C sample app
add_dependencies(samples_c legacy-cross)
