#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if ($ENV{TARGET} MATCHES "localhost")
    configure_file(cm.test ${EXECUTABLE_OUTPUT_PATH}/cm.test COPYONLY)
    add_test(cm.test ${EXECUTABLE_OUTPUT_PATH}/cm.test)
endif()
