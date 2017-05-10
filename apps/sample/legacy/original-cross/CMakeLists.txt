#
# Original program, force to cross-compile
#

include(CMakeForceCompiler)

CMAKE_FORCE_C_COMPILER(${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc LegatoCrossCompiler)

add_executable(legacy-cross main.c)

# This is a C sample app
add_dependencies(samples_c legacy-cross)
