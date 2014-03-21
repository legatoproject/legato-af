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

ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(config)
ADD_SUBDIRECTORY(testsuites)
ADD_SUBDIRECTORY(customfiles)
ADD_SUBDIRECTORY(racontestwrapper)

ADD_LUA_LIBRARY(testsauto DESTINATION testsauto EXCLUDE_FROM_ALL)
ADD_CUSTOM_COMMAND(TARGET testsauto POST_BUILD
    COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/updatepkgs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/453-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/454-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/456-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/457-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/458-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/459-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/474-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/475-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/476-updatepackage.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/resources/normalapp.tar ${CMAKE_INSTALL_PREFIX}/updatepkgs
    
    #COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/453-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/453-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/454-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/454-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/455-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/455-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/456-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/456-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/457-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/457-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/458-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/458-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/459-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/459-updatepackage
    
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/474-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/474-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/475-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/475-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/476-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/476-updatepackage
    COMMAND ${CMAKE_COMMAND} -E copy_directory    ${CMAKE_CURRENT_SOURCE_DIR}/resources/200-updatepackage     ${CMAKE_INSTALL_PREFIX}/updatepkgs/200-updatepackage
)

ADD_DEPENDENCIES(testsauto tester testconfig testsuites testspecific)



