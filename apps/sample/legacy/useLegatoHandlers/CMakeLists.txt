#
# Program modified to use a legato API
#

include(CMakeForceCompiler)

# Build legacy binary
include_directories(${LEGATO_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
generate_client(${LEGATO_ROOT}/interfaces/modemServices/le_mdmDefs.api)
generate_client(${LEGATO_ROOT}/interfaces/modemServices/le_sms.api)
add_executable(legacy-LegatoHandlers-bin main.c
  ${CMAKE_CURRENT_BINARY_DIR}/le_sms_client.c
  ${CMAKE_CURRENT_BINARY_DIR}/le_sms_commonclient.c
  ${CMAKE_CURRENT_BINARY_DIR}/le_mdmDefs_interface.h)
target_link_libraries(legacy-LegatoHandlers-bin ${LEGATO_LIBRARY_PATH} pthread rt)

# Build legato app using that binary
mkapp(legacy-LegatoHandlers.adef)
add_dependencies(legacy-LegatoHandlers legacy-LegatoHandlers-bin)

add_dependencies(samples_c legacy-LegatoHandlers)
