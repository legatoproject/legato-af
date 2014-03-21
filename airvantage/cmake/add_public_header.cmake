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

function(add_public_header libraryTarget headerFile)

add_custom_command(
  TARGET     ${libraryTarget}
  COMMAND    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${headerFile} ${CMAKE_PUBLIC_HEADERS_OUTPUT_DIRECTORY}/${headerFile}
  DEPENDS    ${CMAKE_CURRENT_SOURCE_DIR}/${headerFile}
)

endfunction(add_public_header)