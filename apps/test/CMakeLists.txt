#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Tests
enable_testing()

# Output
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/lib)
set(APP_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/apps)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
file(MAKE_DIRECTORY ${APP_OUTPUT_PATH})

# Custom targets for different language tests
add_custom_target(tests_c
  COMMENT "Generated C tests in ${EXECUTABLE_OUTPUT_PATH}"
)

add_custom_target(tests_java
  COMMENT "Generated Java tests in ${EXECUTABLE_OUTPUT_PATH}"
)

add_custom_target(tests
    DEPENDS tests_c tests_java
    COMMENT "Generated all tests in ${EXECUTABLE_OUTPUT_PATH}"
)

# Framework
add_subdirectory(framework)

# Components

## Modem Services
add_subdirectory(modemServices/sms/smsIntegrationTest)
add_subdirectory(modemServices/sms/smsUnitTest)
add_subdirectory(modemServices/mcc/mccIntegrationTest)
add_subdirectory(modemServices/mcc/mccCallWaitingTest)
add_subdirectory(modemServices/mcc/mccUnitTest)
add_subdirectory(modemServices/mdc/mdcIntegrationTest)
add_subdirectory(modemServices/mdc/mdcUnitTest)
add_subdirectory(modemServices/mdc/mdcMultiPdpTest)
add_subdirectory(modemServices/mrc/mrcIntegrationTest)
add_subdirectory(modemServices/mrc/mrcUnitTest)
add_subdirectory(modemServices/sim/simIntegrationTest)
add_subdirectory(modemServices/sim/simUnitTest)
add_subdirectory(modemServices/sim/simProfileSwap)
add_subdirectory(modemServices/sim/simToolkit)
add_subdirectory(modemServices/info/infoIntegrationTest)
add_subdirectory(modemServices/info/infoUnitTest)
add_subdirectory(modemServices/ips/ipsIntegrationTest)
add_subdirectory(modemServices/ips/ipsUnitTest)
add_subdirectory(modemServices/temp/tempIntegrationTest)
add_subdirectory(modemServices/temp/tempUnitTest)
add_subdirectory(modemServices/antenna/antennaIntegrationTest)
add_subdirectory(modemServices/antenna/antennaUnitTest)
add_subdirectory(modemServices/adc/adcUnitTest)
add_subdirectory(modemServices/riPin/riPinIntegrationTest)
add_subdirectory(modemServices/riPin/riPinUnitTest)
add_subdirectory(modemServices/ecall/ecallUnitTest)
add_subdirectory(modemServices/rtc/rtcUnitTest)
add_subdirectory(modemServices/rsim/rsimIntegrationTest)
add_subdirectory(modemServices/rsim/rsimUnitTest)
add_subdirectory(modemServices/lpt/lptIntegrationTest)
add_subdirectory(modemServices/lpt/lptUnitTest)
add_subdirectory(modemServices/lpt/lptUserTest)

if(    (INCLUDE_ECALL EQUAL 1)
   AND (   (LEGATO_TARGET MATCHES "ar7")
        OR (LEGATO_TARGET MATCHES "ar86")
        OR (LEGATO_TARGET MATCHES "wp85")
       ))
    add_subdirectory(modemServices/ecall/ecallIntegrationTest)
    add_subdirectory(modemServices/ecall/ecallWPrompt)
    add_subdirectory(modemServices/ecall/ecallWVoice)
    add_subdirectory(modemServices/ecall/ecallStress)
endif()

## Positioning Services
add_subdirectory(positioning/gnssTest)
add_subdirectory(positioning/gnssUnitTest)
add_subdirectory(positioning/gnssXtraTest)
# To be implemented add_subdirectory(positioning/posDaemonTest)
add_subdirectory(positioning/positioningTest)
add_subdirectory(positioning/positioningUnitTest)

## Audio Services
add_subdirectory(audio/service/audioTest)
add_subdirectory(audio/service/memoryTest)
add_subdirectory(audio/audioMcc)
add_subdirectory(audio/audioCallPbRec)
add_subdirectory(audio/dtmf)
add_subdirectory(audio/audioCfg)
add_subdirectory(audio/audioPlaybackRec)
add_subdirectory(audio/voicePromptMcc)
add_subdirectory(audio/voicePromptMcc2)
add_subdirectory(audio/audioUnitTest)

## Cellular Network Service
add_subdirectory(cellNetService/cellNetServiceTest)
add_subdirectory(cellNetService/cellNetUnitTest)

## Data Connection Service
add_subdirectory(dataConnectionService/dataConnectionServiceTest)
add_subdirectory(dataConnectionService/dataConnectionUnitTest)

## Other Services ...
add_subdirectory(voiceCallService/voiceCallServiceIntegrationTest)
add_subdirectory(voiceCallService/voiceCallServiceUnitTest)
add_subdirectory(smsInboxService/smsInboxServiceIntegrationTest)
add_subdirectory(smsInboxService/smsInboxServiceUnitTest)

# AirVantage Service
add_subdirectory(avcService)

# UTs for AirVantageConnector
if(EXISTS ${LEGATO_ROOT}/apps/platformServices/airVantageConnector/apps/test)
    add_subdirectory(${LEGATO_ROOT}/apps/platformServices/airVantageConnector/apps/test
                     ${CMAKE_BINARY_DIR}/apps/test/platformServices/airVantageConnector/)

    # Build avc tests when building tests
    add_dependencies(tests_c avc_tests_c)
endif()

# IfGen Tool
add_subdirectory(ifgen/test2)

# IfGen created IPC
add_subdirectory(ipc)

# AT Services
add_subdirectory(atServices/atClientTest)
add_subdirectory(atServices/atServerIntegrationTest)
add_subdirectory(atServices/atServerMultipleAppsTest)
add_subdirectory(atServices/atServerUnitTest)
add_subdirectory(atServices/atClientUnitTest)

# CM tool
add_subdirectory(cm)

# Fw update
add_subdirectory(fwupdate/fwupdateUnitTest)
add_subdirectory(fwupdate/fwupdateIntegrationTest)

# UTs for fwupdateDualsys
if(EXISTS ${LEGATO_ROOT}/platformAdaptor/fwupdate/apps/test)
    add_subdirectory(${LEGATO_ROOT}/platformAdaptor/fwupdate/apps/test
                     ${CMAKE_BINARY_DIR}/apps/test/fwupdate/platformAdaptor/fwupdate)
endif()

# UTs for WiFi
if(EXISTS ${LEGATO_ROOT}/modules/WiFi/apps/test)
    add_subdirectory(${LEGATO_ROOT}/modules/WiFi/apps/test
                     ${CMAKE_BINARY_DIR}/apps/test/WiFi/)
endif()

# Power Manager
add_subdirectory(powerMgr/powerMgrTest)

# Port Service
add_subdirectory(portService/portServiceUnitTest)
add_subdirectory(portService/portServiceIntegrationTest)
