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

ADD_LUA_LIBRARY(testsuites DESTINATION tests EXCLUDE_FROM_ALL
  tsagent.lua
  tsluafwk.lua
  tsintegration.lua
  integration/crypto.lua
  integration/rest.lua
  integration/restdigest.lua
  integration/srvcon.lua
  integration/updatedwl.lua
)

ADD_LUA_LIBRARY(test_fwkemb DESTINATION tests EXCLUDE_FROM_ALL
  luafwktests.lua
  agenttests.lua
)
