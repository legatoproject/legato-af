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

--------------------------------------------------------------------------------
--
-- Provides interfaces with the M2M Cloud Services
-- Platform.
--
-- The Racon Application Services functionality (managing transfer and
-- remote storage of application data and commands) is exposed in the
-- @{racon.asset} module.
--
--
-- @module racon
--


local common    = require 'racon.common'
local asset     = require 'racon.asset'
local checks    = require 'checks'

local M = { initialized=false }

--------------------------------------------------------------------------------
-- Initializes the module.
--
-- @function [parent=#racon] init
-- @return non-`nil` upon success;
-- @return `nil` + error message upon failure.
--

function M.init()
    if M.initialized then return "already initialized" end
    local a, b = common.init()
    if not a then return a, b end
    a, b = asset.init()
    if not a then return a, b end
    M.initialized=true
    return M
end

--------------------------------------------------------------------------------
-- Forces the tables and unstructured data attached to a given policy to be sent
-- or consolidated immediately.
--
-- A connection to the server is done only if data needs to be sent as the result
-- to this trigger operation. Put another way, if no data is attached to the
-- triggered policy(ies), then no connection to the server is done.
-- See @{#racon.connectToServer} for complementary function.
--
-- For a description of how policies allow to manage data reporting from the
-- assets to the server, see the _Racon technical article_.
--
-- @function [parent=#racon] triggerPolicy
-- @param #string policy name of the policy queue to be flushed.
--   Flush all policies if policy=='*';
--   only flush the `default` policy if policy is omitted.
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--

function M.triggerPolicy(policy)
    checks("?string")
    if not M.initialized then error "Module not initialized" end
    return common.sendcmd("PFlush", { policy=policy })
end

--------------------------------------------------------------------------------
-- Forces a connection to the server.
--
-- This connection will not flush outgoing data handled through policies,
-- but it will poll the server for new messages addressed to assets on this
-- gateway device.
--
-- If using nil as latency, the connection is synchronous, i.e. once this function returns, the requested connection to the server is closed.
-- Otherwise the connection will happen after this call returns.
--
-- Note:
--  - 0 value means the connection will be asynchronous, but will be done as soon as possible.
--
-- @function [parent=#racon] connectToServer
-- @param latency optional positive integer, latency in seconds before initiating the connection to the server,
-- use nil value to specify synchronous connection.
-- @return non-`nil` upon success;
-- @return `nil` + error message upon failure.
--

function M.connectToServer(latency)
    checks('?number')
    if not M.initialized then error "Module not initialized" end
    --latency = nil means synchronous connection, allow it.
    return common.sendcmd("ConnectToServer", latency)
end

--------------------------------------------------------------------------------
-- Acknowledges a server message received with an acknowledgment ticket id.
--
-- In most realistic cases, the appropriate way to acknowledge a message is
-- to have the handler function return a status code, `0` for success or a
-- non-null number to signal failure. Most applications should therefore not
-- call this function explicitly.
--
-- @function [parent=#racon] acknowledge
-- @param ackid the id to acknowledge
-- @param status a boolean, true for success, false for failure
-- @param errmsg an optional error message string
-- @param policy optional triggering policy to send the acknowledgment,
--  defaults to `"now"`.
-- @param persisted if true, the ACK message will be persisted in flash
--  by the agent, and kept even if a reboot occurs before the policy
--  is triggered.
--

function M.acknowledge(ackid, status, errmsg, policy, persisted)
    checks('?number', '?', '?string', '?string', '?')
    if not ackid then return "no need to acknowledge" end
    status = tonumber(status) or (status and 0 or -1)
    local content = {
        ticket    = ackid,
        status    = status,
        message   = errmsg,
        policy    = policy,
        persisted = persisted }
    return common.sendcmd("PAcknowledge", content)
end

--------------------------------------------------------------------------------
-- Creates and returns a new @{racon.asset#asset} instance.
--
-- The newly created @{racon.asset#asset} is not started when returned, it can therefore
-- neither send nor receive messages at this point.
--
-- This intermediate, unstarted set allows to configure message handlers before
-- any message is actually transferred to the #asset.
--
-- See @{racon.asset#asset.start} to start the newly created instance.
--
-- @function [parent=#racon] newAsset
-- @param id string defining the assetid identifying the instance of this new asset.
-- @return racon.asset#asset instance on success.
-- @return `nil` followed by an error message otherwise.
--
M.newAsset = asset.newasset

require 'utils.loweralias' (M)

return M
