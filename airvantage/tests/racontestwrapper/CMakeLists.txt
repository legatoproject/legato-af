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

add_custom_target(start_dev_test
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/racontestwrapper.lua ${CMAKE_INSTALL_PREFIX}/racontestwrapper.lua
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/testwrapperfwk ${CMAKE_INSTALL_PREFIX}/testwrapperfwk
)
ADD_DEPENDENCIES(start_dev_test lua web platformserver)
