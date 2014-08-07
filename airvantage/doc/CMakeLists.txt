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

SET(DOC_OUTPUR_DIR "${CMAKE_CURRENT_BINARY_DIR}/output")

ADD_SUBDIRECTORY (doxygen)
ADD_SUBDIRECTORY (ldoc)
ADD_SUBDIRECTORY (md)

# Add a general to make documentation. This one copies everything in a single doc subdirectoy
# The doc taget depends on each specific doc target
ADD_CUSTOM_TARGET(doc
                  DEPENDS cdoc ldoc mdoc)

