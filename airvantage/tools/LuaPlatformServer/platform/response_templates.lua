-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local M = { }
platform.response_templates = M

M.readnode = [[{
    __class = 'Message',
    path     = '@sys.commands', -- must contain at least the asset id
    ticketid = 0, -- ticket number for acknowledgement, leave to 0 to disable
    body = {
        ['ReadNode'] = { 'config.server' }
    }
}]]


M.acknowledgement = [[{
    __class = 'Response',
    status = 0,   -- 0 for success, other number for errors
    ticketid = 0, -- ticket to acknowledge
    data = nil    -- Optional extra data, typically error message
}]]

M.data = [[{
    __class = 'Message',
    path     = 'myasset', -- must contain at least the asset id
    ticketid = 0, -- ticket number for acknowledgement, leave to 0 to disable
    body = {
        ['data'] = { },
        ['timestamp'] = { os.time() }
    }
}]]

M.commands = [[{
    __class = 'Message',
    path     = 'myasset.commands', -- must contain at least the asset id
    ticketid = 0, -- ticket number for acknowledgement, leave to 0 to disable
    body = {
        ['command_name'] = { TODO }
    }
}]]

return M