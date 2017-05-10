#
# Program modified to use a legato API
#

include(CMakeForceCompiler)

CMAKE_FORCE_C_COMPILER(${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc LegatoCrossCompiler)

# Build legacy binary
include_directories(${LEGATO_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
generate_client(${LEGATO_ROOT}/interfaces/modemServices/le_info.api)
add_executable(legacy-LegatoApi-bin main.c ${CMAKE_CURRENT_BINARY_DIR}/le_info_client.c)
target_link_libraries(legacy-LegatoApi-bin ${LEGATO_LIBRARY_PATH} pthread rt)

# Build legato app using that binary
mkapp(legacy-LegatoApi.adef)
add_dependencies(legacy-LegatoApi legacy-LegatoApi-bin)

add_dependencies(samples_c legacy-LegatoApi)
