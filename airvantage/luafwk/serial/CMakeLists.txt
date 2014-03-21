#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

PROJECT(MIHINI_SERIAL)

# TODO at could/should be distinguished from basic serial access

ADD_LUA_LIBRARY(serial serial.c serial.lua at.lua)
ADD_DEPENDENCIES(serial sched serial_core fdwrapper)
INSTALL(TARGETS serial LIBRARY DESTINATION lua)
INSTALL(FILES serial.lua DESTINATION lua)
