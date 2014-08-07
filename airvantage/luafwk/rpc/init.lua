--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
--
-- Contributors:
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

-- Select an implementation of `rpc`, depending on whether `sched` is enabled.

if global then global 'sched'; global 'rpc' end -- Avoid 'strict' errors
rpc = require (sched and 'rpc.sched' or 'rpc.nosched')

rpc.signature = require 'rpc.proxy' .signature

require 'rpc.builtinsignatures' (rpc)

return rpc