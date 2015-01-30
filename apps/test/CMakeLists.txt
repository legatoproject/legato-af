
#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
add_subdirectory(framework)

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
add_subdirectory(modemServices/ipsTest)
add_subdirectory(modemServices/tempTest)

if((INCLUDE_ECALL EQUAL 1) AND ((LEGATO_TARGET MATCHES "ar7") OR (LEGATO_TARGET MATCHES "ar86")))
    add_subdirectory(modemServices/ecall)
    add_subdirectory(modemServices/ecallWPrompt)
endif()

if(LEGATO_COMPONENTS_MODEM MATCHES "QMI")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modem/qmi)
        add_subdirectory(modem/qmi/sim)
        add_subdirectory(modem/qmi/voice)
        add_subdirectory(modem/qmi/sms)
        add_subdirectory(modem/qmi/mdc)
        add_subdirectory(modem/qmi/mrc)
        if((INCLUDE_ECALL EQUAL 1) AND ((LEGATO_TARGET MATCHES "ar7") OR (LEGATO_TARGET MATCHES "ar86")))
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
add_subdirectory(positioning/gnssTest)
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
add_subdirectory(audio/audioMcc)
add_subdirectory(audio/audioMccExe)
add_subdirectory(audio/audioCallPbRec)
add_subdirectory(audio/dtmf)
add_subdirectory(audio/configuration)
add_subdirectory(audio/audioPlaybackRec)
add_subdirectory(audio/audioPlaybackRecExe)

## Data Connection Service
add_subdirectory(dataConnectionService)

## Monitoring Service
add_subdirectory(monitoring/antennaTest)

## Other Services ...
add_subdirectory(cellNetService)
add_subdirectory(voiceCallService)

# IfGen Tool
add_subdirectory(ifgen/test2)
