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

local table = {
  native = { Host = "localhost", RPCPort = 2012, ConfigModule = "native", path="./native", compil={toolchain = "default"}, install={}},
  linux_x86 = { Host = "10.0.0.1", RPCPort = 2012, ConfigModule = "linux_x86", path="./linux_x86", compil={toolchain = "qemu_i386"}, install={srcpath="/opt/qemu_i386/vm"} },
  linux_amd64 = { Host = "10.0.0.1", RPCPort = 2012, ConfigModule = "linux_x64", path="./linux_x64", compil={toolchain = "qemu_x86_64"}, install={srcpath="/opt/qemu_x86_64/vm"} },
  local_x86 = { Host = "localhost", RPCPort = 2012, ConfigModule = "local_x86", path="./local_x86", compil={toolchain = "default"}, install={} }
}

return table