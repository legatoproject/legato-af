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

-- Tries to load and return `persist.qdbm`. If the module isn't available, e.g
-- because of licensing issues, loads the less efficient but EPL licensed
-- `persist.file` instead.

local status
status, persist = pcall(require, 'persist.qdbm')
if not status then persist = require 'persist.file' end
return persist