
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
set(APP_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})


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
add_subdirectory(framework/c++)

# Components

## Modem Services
add_subdirectory(modemServices/sms)
add_subdirectory(modemServices/smsPdu)
add_subdirectory(modemServices/smsCdmaPdu)
add_subdirectory(modemServices/mcc)
add_subdirectory(modemServices/mdc/mdcIntegrationTest)
add_subdirectory(modemServices/mdc/mdcUnitaryTest)
add_subdirectory(modemServices/mrc)
add_subdirectory(modemServices/sim)
add_subdirectory(modemServices/info)
add_subdirectory(modemServices/remoteMgmt)

if(INCLUDE_ECALL EQUAL 1 AND LEGATO_TARGET MATCHES "ar7")
    add_subdirectory(modemServices/ecall)
endif()

if(LEGATO_COMPONENTS_MODEM MATCHES "QMI")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modem/qmi)
        add_subdirectory(modem/qmi/sim)
        add_subdirectory(modem/qmi/voice)
        add_subdirectory(modem/qmi/sms)
        add_subdirectory(modem/qmi/mdc)
        add_subdirectory(modem/qmi/mrc)
        if(INCLUDE_ECALL EQUAL 1 AND LEGATO_TARGET MATCHES "ar7")
           add_subdirectory(modem/qmi/ecall)
        endif()
    endif()
elseif(LEGATO_COMPONENTS_MODEM MATCHES "AT")
    add_subdirectory(components/ATManager)
    add_subdirectory(modem/at/mrc)
    add_subdirectory(modem/at/sim)
    add_subdirectory(modem/at/sms)
endif()

## Positioning Services
add_subdirectory(positioning/posDaemonTest)
add_subdirectory(positioning/posCtrlTest)

if(LEGATO_COMPONENTS_GNSS MATCHES "QMI")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/positioning/qmi)
        add_subdirectory(positioning/qmi/position)
        add_subdirectory(positioning/qmi/xtra)
    endif()
endif()

## Audio Services
add_subdirectory(audio/pa)
add_subdirectory(audio/service/audioTest)
add_subdirectory(audio/service/memoryTest)
add_subdirectory(audio/mccClient)
add_subdirectory(audio/audioCallPbRec)
add_subdirectory(audio/dtmf)
add_subdirectory(audio/configuration)

## Data Connection Service
add_subdirectory(dataConnectionService)

## Other Services ...
add_subdirectory(cellNetService)
add_subdirectory(voiceCallService)


# IfGen Tool
add_subdirectory(ifgen/test2)

