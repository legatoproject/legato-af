#*******************************************************************************
# Copyright (c) 2013 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#   http://www.eclipse.org/legal/epl-v10.html
# The Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************
PROJECT(LIB_SHA1)

ADD_LIBRARY(lib_sha1 SHARED sha1.c)
SET_TARGET_PROPERTIES(lib_sha1 PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME sha1)
INSTALL(TARGETS lib_sha1 LIBRARY DESTINATION lib)
