#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from helloWorld.adef
mkapp(javaHelloWorld.adef
      -s "${LEGATO_ROOT}/components")

# This is a sample application
add_dependencies(samples_java javaHelloWorld)
