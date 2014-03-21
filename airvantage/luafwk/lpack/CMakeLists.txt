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

PROJECT(PACK)

ADD_LUA_LIBRARY(pack lpack.c)
INSTALL(TARGETS pack LIBRARY DESTINATION lua)
