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

PROJECT(MIHINI_UTILS)

ADD_LUA_LIBRARY(utils DESTINATION utils
    loader.lua path.lua table.lua loweralias.lua =ltn12/source.lua system.lua)

INSTALL(FILES loader.lua path.lua table.lua loweralias.lua system.lua DESTINATION lua/utils)
INSTALL(FILES ltn12/source.lua DESTINATION lua/utils/ltn12)
