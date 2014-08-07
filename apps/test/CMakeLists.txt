#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Tests
enable_testing()

# Output
set(EXECUTABLE_OUTPUT_PATH ${LEGATO_BINARY_DIR}/bin/tests)

# Framework
add_subdirectory(framework/args)
add_subdirectory(framework/eventLoop)
add_subdirectory(framework/path)
add_subdirectory(framework/hashmap)
add_subdirectory(framework/safeRef)
add_subdirectory(framework/semaphore)
add_subdirectory(framework/signalEvents)
add_subdirectory(framework/threads)
add_subdirectory(framework/timers)
add_subdirectory(framework/hex)
add_subdirectory(framework/messaging)
add_subdirectory(framework/user)
add_subdirectory(framework/configTree)

# Components
add_subdirectory(modemServices/sms)
add_subdirectory(modemServices/smsPdu)
add_subdirectory(modemServices/mcc)
add_subdirectory(modemServices/mdc)
add_subdirectory(modemServices/mrc)
add_subdirectory(modemServices/sim)
if(AUTOMOTIVE_TARGET)
    add_subdirectory(modemServices/ecall)
endif()
#add_subdirectory(positioning)
#add_subdirectory(audio/pa)
#add_subdirectory(audio/service)
#add_subdirectory(audio/mccClient)
#add_subdirectory(cellNetService)
#add_subdirectory(audio/audioCallPbRecApp)
add_subdirectory(voiceCallService)

#if(LEGATO_COMPONENTS_MODEM MATCHES "QMI")
#    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modem/qmi)
#        add_subdirectory(modem/qmi/sim)
#        add_subdirectory(modem/qmi/voice)
#        add_subdirectory(modem/qmi/sms)
#        add_subdirectory(modem/qmi/ecall)
#    endif()

#    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/positioning/qmi)
#        add_subdirectory(positioning/qmi/position)
#        add_subdirectory(positioning/qmi/xtra)
#    endif()
#elseif(LEGATO_COMPONENTS_MODEM MATCHES "AT")
#    add_subdirectory(components/ATManager)
#    add_subdirectory(modem/at/mrc)
#    add_subdirectory(modem/at/sim)
#    add_subdirectory(modem/at/sms)
#endif()

# IfGen Tool
add_subdirectory(ifgen/test2)

