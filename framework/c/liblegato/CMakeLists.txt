#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Flags
set_legato_component(framework)
add_definitions(-fPIC)

# Generate dependencies in the same directory that contains the .o
#  For instance, dependencies for le_clock.c will be located at
#  framework/c/CMakeFiles/legato_dep.dir/src/le_clock.c.d
add_definitions(-MD)

# Sources
aux_source_directory(${LEGATO_C_SOURCE_DIR} LEGATO_C_SRC)

# Builds the legato library
add_library(${LEGATO_FRAMEWORK_TARGET} SHARED ${LEGATO_C_SRC})

# Linking
target_link_libraries(${LEGATO_FRAMEWORK_TARGET}
    pthread
    rt
)
