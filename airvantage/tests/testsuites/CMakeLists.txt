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

ADD_LUA_LIBRARY(testsuites DESTINATION tests EXCLUDE_FROM_ALL
  tsagent.lua
  tsluafwk.lua
  tsintegration.lua
  integration/security.lua
  integration/rest.lua
  integration/restdigest.lua
  integration/srvcon.lua
  integration/updatedwl.lua
  integration/raconwrapper.lua
)

ADD_LUA_LIBRARY(test_fwkemb DESTINATION tests EXCLUDE_FROM_ALL
  luafwktests.lua
  agenttests.lua
)
