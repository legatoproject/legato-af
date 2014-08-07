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
#     Romain Perier for Sierra Wireless - initial API and implementation
#     Benjamin Cabé for Sierra Wireless - bug 407919
#*******************************************************************************

ADD_CUSTOM_TARGET(mdoc
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/pandocize.sh ${DOC_OUTPUR_DIR}
)
