-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------




local param = ...
--test parameter given to the script
p("installscript2:", param)

--install script: there must always have a table param
if not param then error("installscript2: No param table") end
--in test Manifest : if install(version != nil) -> no extra parameters
if param.version and param.parameters then error("installscript: parameters inconsistent with Manifest")  end
--in test Manifest : if uninstall(version) -> extra  parameters
if not param.version and (not param.parameters or not param.parameters.reason or param.parameters.reason ~= "bye bye") then error("installscript2: parameters inconsistent with Manifest")  end
log("installscript2", "INFO", "OK")
