#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Don't try to connect to the Log Control Daemon.
add_definitions(-DNO_LOG_CONTROL)

set_legato_component(logCtrlDaemon)
add_legato_internal_executable(logCtrlDaemon logDaemon.c)
