#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

add_subdirectory(audio)
add_subdirectory(modem)
add_subdirectory(positioning)
add_subdirectory(dataConnection)
add_subdirectory(cellNet)
add_subdirectory(smsInbox)
add_subdirectory(voiceCall)
add_subdirectory(power)
add_subdirectory(secureStorage)
add_subdirectory(airVantage)
add_subdirectory(fwupdate)
if(LEGATO_TARGET MATCHES "wp85")
    add_subdirectory(gpioService)
endif()
