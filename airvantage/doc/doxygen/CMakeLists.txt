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

ADD_CUSTOM_TARGET(doxygen_gen
    #doxy.sh generates config files for doxygen (custom html header, etc)
    #those files needs to be kept until doc generation is done
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ${CMAKE_CURRENT_SOURCE_DIR}/doxy.sh -g \"\${GA_TRACKER}\"
    #actually call doxygen to generate doc.
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && doxygen ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    # Cleaning output dir: remove any existing doc
    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/C_User_API_doc
    # Cleaning files generated in doxy.sh
    COMMAND rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/header ${CMAKE_CURRENT_SOURCE_DIR}/footer ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.css
    # Move doc to output dir
    COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/C_User_API_doc ${CMAKE_CURRENT_BINARY_DIR}
)
