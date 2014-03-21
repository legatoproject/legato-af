#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This building script is made available under the terms
# of the Eclipse Public License v1.0 which accompanies this distribution, and
# is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

PROJECT(MIHINI_COXPCALL)

ADD_LUA_LIBRARY(coxpcall coxpcall.lua)
ADD_DEPENDENCIES(coxpcall utils)
INSTALL(FILES coxpcall.lua DESTINATION lua)
