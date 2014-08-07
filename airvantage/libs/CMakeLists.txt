 #*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
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

PROJECT(CLIBS)



ADD_SUBDIRECTORY (common)
ADD_SUBDIRECTORY (luasignal)
ADD_SUBDIRECTORY (bysant)
ADD_SUBDIRECTORY (SerialFramework)
ADD_SUBDIRECTORY (hmac-md5)
ADD_SUBDIRECTORY (sha1)
ADD_SUBDIRECTORY (cdb)
ADD_SUBDIRECTORY (yajl)
ADD_SUBDIRECTORY (${SECURITY_LIB})
ADD_SUBDIRECTORY (keystore)
