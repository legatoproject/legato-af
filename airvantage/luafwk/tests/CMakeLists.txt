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

PROJECT(TEST_LUAFWK)

ADD_LUA_LIBRARY(test_luafwk DESTINATION tests EXCLUDE_FROM_ALL
                bysant.lua luatobin.lua  persist.lua rpc.lua sched.lua socket.lua logstore.lua crypto.lua posixsignal.lua timer.lua
                stagedb.lua 
               )

ADD_DEPENDENCIES(test_luafwk agent_provisioning)

ADD_LUA_LIBRARY(test_racon DESTINATION tests EXCLUDE_FROM_ALL
    devicetree.lua sms.lua system.lua)

ADD_UNIT_TEST(asset_tree asset_tree.lua TEST_TYPE non-standalone TEST_DEPENDENCY system_stubs)
ADD_UNIT_TEST(racon racon.lua)
ADD_UNIT_TEST(devicetree devicetree.lua)
ADD_DEPENDENCIES(unittest_devicetree treehdlsample agent_treemgr_treehdlsample)
ADD_UNIT_TEST(sms sms.lua TEST_DEPENDENCY messaging_stubs)
ADD_UNIT_TEST(system system.lua TEST_DEPENDENCY system_stubs)
ADD_UNIT_TEST(emp emp.lua TEST_DEPENDENCY emp_server TEST_OPT 1235)
