#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application that uses File Stream Service
mkapp(fileStreamDemo.adef
    -i ${LEGATO_ROOT}/interfaces/fileStream
)

# This is a sample application
add_dependencies(samples_c fileStreamDemo)
