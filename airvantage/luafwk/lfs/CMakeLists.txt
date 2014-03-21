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

PROJECT(LFS)
ADD_LUA_LIBRARY(lfs lfs.c)
INSTALL(TARGETS lfs LIBRARY DESTINATION lua)
