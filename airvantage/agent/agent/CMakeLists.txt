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

### Lua modules of the agent

# minimal feature/internal files set
ADD_LUA_LIBRARY(agent_modules DESTINATION agent
    init.lua  config.lua  migration.lua
    boot.lua  system.lua )

ADD_LUA_LIBRARY(agent_modem DESTINATION agent
    modem.lua)

ADD_LUA_LIBRARY(agent_time DESTINATION agent
    time.lua)

ADD_LUA_LIBRARY(agent_mediation DESTINATION agent
    mediation.lua)

ADD_LUA_LIBRARY(agent_appcon DESTINATION agent
    appcon.lua)

ADD_LUA_LIBRARY(agent_rest DESTINATION agent
    rest.lua)

ADD_LUA_LIBRARY(agent_autoexec DESTINATION agent
    autoexec.lua)

ADD_LUA_LIBRARY(agent_networkmanager DESTINATION agent
    netman.lua bearer.lua)

ADD_LUA_LIBRARY(agent_srvcon DESTINATION agent
    srvcon.lua)

ADD_DEPENDENCIES(agent_srvcon
    m3da_transport
    m3da_session)

ADD_SUBDIRECTORY(update)
ADD_SUBDIRECTORY(devman)
ADD_SUBDIRECTORY(monitoring)
ADD_SUBDIRECTORY(treemgr)
ADD_SUBDIRECTORY(asscon)

# optional provisioning module, to easily write crypto keys locally
ADD_LUA_LIBRARY(agent_provisioning DESTINATION agent EXCLUDE_FROM_ALL provisioning.lua)
ADD_DEPENDENCIES(agent_provisioning crypto_keystore)

ADD_DEPENDENCIES(agent_modules

    # Internal dependencies: minimal feature set
    agent_devman agent_treemgr agent_srvcon
    agent_linux agent_asscon   
    agent_versions  

    # External dependencies
    mihini_misc # strict, print etc.
    checks coxpcall lfs log pack
    mihini_linux #posixsignal, fdwrapper etc.
    
    # Target-specific config
    agent_platform
    )


### Test modules ###

ADD_LUA_LIBRARY(test_agent DESTINATION tests EXCLUDE_FROM_ALL
    tests/config.lua
    tests/monitoring.lua
    tests/aleosstub.lua tests/time.lua
    tests/extvars.lua
    tests/mediation.lua tests/mediationtestserver.lua tests/treemgr.lua
    tests/appcon.lua tests/treemgr/treemgr_table1.lua
    tests/treemgr/treemgr_table2.lua
    tests/update/update.lua
    tests/update/updateDaily.lua
    tests/data_policy.lua
)

ADD_DEPENDENCIES(test_agent test_agent_treemgr_maps treehdlsample agent)

ADD_CUSTOM_TARGET(test_update_pkgs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/update/test_packages ${CMAKE_INSTALL_PREFIX}/update/tmp/test_packages
)
ADD_DEPENDENCIES(test_agent test_update_pkgs agent_update_tools)

INSTALL(FILES rest.lua config.lua modem.lua migration.lua boot.lua srvcon.lua netman.lua system.lua time.lua mediation.lua bearer.lua init.lua autoexec.lua appcon.lua provisioning.lua DESTINATION lua/agent)