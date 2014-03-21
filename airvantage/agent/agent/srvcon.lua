-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local require     = require
local sched       = require "sched"
local log         = require "log"
local config      = require "agent.config"
local asscon      = require "agent.asscon"
local usource     = require "utils.ltn12.source"
local upath       = require "utils.path"
local timer       = require "timer"
local lock        = require "sched.lock"
local ltn12       = require "ltn12"
local checks      = require "checks"
local errnum      = require "returncodes".tonumber
require 'coxpcall'
require 'socket.url'
require 'print'

local M = { }

global 'agent'; agent.srvcon = M

local m3da_deserialize = require "m3da.bysant".deserializer()


-- hook that is set to nil by default: if non nil this function is called prior to doing the connexion
M.preconnecthook = false

-- will be filled with a `require 'm3da.transport.XXX'` where XXX is
-- a session management protocol. Currently, `security` and `default` are
-- supported. This is set during the `srvcon` module's initialization, based on
-- the agent's config.
M.session = nil

-- Dispatch deserialized server messages to the appropriate assets through EMP.
local function dispatch_message(envelope_payload)

    if not envelope_payload or #envelope_payload <= 0 then return 'ok' end

    --base.global 'last_payload'
    --base.last_payload = payload
    local offset = 1
    while offset <= #envelope_payload do
        local msg
        msg, offset = m3da_deserialize(envelope_payload, offset)
        if msg==nil and tonumber(offset) or msg=='' then
            log('SRVCON', 'DETAIL', 'Empty message from server')
        elseif msg.__class == 'Message' then
            if not msg.body or not next(msg.body) then
                log("SRVCON", "ERROR", "invalid message: message body is nil or empty, message is rejected")
                if msg.ticketid and msg.ticketid ~= 0 then require('racon').acknowledge(msg.ticketid, false, "invalid message: message body is nil or empty", "now", false) end
            else
                local name = upath.split(msg.path, 1)
                local r, errmsg = asscon.sendcmd(name, "SendData", msg)
                if not r then
                    --build and send a NAK to the server through a session+transport
                    log("SRVCON", "ERROR", "Failed to dispatch server message %s", sprint(msg))
                    if msg.ticketid and msg.ticketid ~= 0 then require('racon').acknowledge(msg.ticketid, false, errmsg, "now", false) end
                end
            end
         elseif msg.__class == 'Response' then
            log('SRVCON', 'WARNING', "Received a response %d: %s to ticket %d",
                msg.status, sprint(msg.data), msg.ticketid)
            log('SRVCON', 'WARNING', "No special handling of responses implemented")
         else
            log('SRVCON', 'ERROR', "Received unsupported envelope content: %s",
                sprint(msg))
        end
    end
end

M.sourcefactories = { }

M.pendingcallbacks = { }

local function concat_factories(factories)
    return function()
        local sources = { }
        for k, _ in pairs(factories) do
            table.insert(sources, k())
        end
        return ltn12.source.cat(unpack(sources))
    end
end

local function rollback_session(factories, callbacks)
    for f, _ in pairs(factories) do
        M.sourcefactories[f]=true
    end
    for f, _ in pairs(callbacks) do
        M.pendingcallbacks[f]=true
    end
end


function M.dosession()
    if lock.waiting(M) > 0 then return nil, "connection already in progress" end
    lock.lock(M)
    local pending_factories, pending_callbacks
    M.sourcefactories, M.pendingcallbacks, pending_factories, pending_callbacks =
        { }, { }, M.sourcefactories, M.pendingcallbacks
    local source_factory = concat_factories(pending_factories)
    local status, errmsg = agent.netman.withnetwork(M.session.send, M.session, source_factory)
    if not status then
        log('SRVCON', 'ERROR', "Error while sending data to server: %s", tostring(errmsg))
        rollback_session(pending_factories, pending_callbacks)
        lock.unlock(M)
        return nil, errmsg
    end

	-- Execute the callbacks that where registered when the
	-- session started (meanwhile, other callbacks might have
	-- been stacked, corresponding with sources to be sent in
	-- the next session).
    for callback, _ in pairs(pending_callbacks) do
        callback(status, errmsg)
    end
    lock.unlock(M)

    if status >= 200 and status <= 299 then
        return "ok"
    else
        return nil, "unexpected status code " .. tostring(status)
    end
end

-- Obsolete: former support for data sending through SMS.
function M.parseonewaypackage(message)
    error "SMS support disabled"
end

--- Gives data to the server.
--
--  @param factory an optional function, returning an ltn12 data source. The factory might be called
--    more than once, in case of failure or authentication issue.
--  @param callback an optional user function to be called when the data has been sent to the server.
--
function M.pushtoserver(factory, callback)
    checks('?function', '?function')
    if factory  then M.sourcefactories[factory] = true end
    if callback then M.pendingcallbacks[callback] = true end
end

--- Forces data fed through @{#pushtoserver} to be actually flushed to the server.
--
--  @param factory an optional function, returning an ltn12 data source. The factory might be called
--    more than once, in case of failure or authentication issue.
--  @param callback an optional user function to be called when the data has been sent to the server.
--  @param latency an optional delay after which connection to the server is forced.
--
function M.connect(latency)
    return timer.latencyexec(M.dosession, latency)
end

--- Responds to request to connect to server sent through EMP messages.
local function EMPConnectToServer(assetid, latency)
    local s, err = M.connect(latency)
    if not s then return errnum 'COMMUNICATION_ERROR', err else return 0 end
end

--- Sets up the module according to agent.config settings.
function M.init()

    -- EMP callbacks
    asscon.registercmd("ConnectToServer", EMPConnectToServer)

    -- Choose and load the appropriate session and transport modules, depending on config.
    local cs, session_name = config.server, 'default'
    local transport_name = socket.url.parse(cs.url).scheme :lower()
    log('SRVCON', 'INFO', "Server URL = %q", cs.url)
    if cs.authentication or cs.encryption then session_name = 'security' end

    local transport_mod = require ('m3da.transport.'..transport_name)
    if not transport_mod then return nil, "cannot get transport" end

    local session_mod   = require ('m3da.session.'..session_name)
    if not session_mod then return nil, "cannot get session manager" end

    local transport, err_msg = transport_mod.new(cs.url);
    if not transport then return nil, err_msg end

    M.session, err_msg = session_mod.new{
        transport      = transport,
        msghandler     = dispatch_message,
        localid        = config.agent.deviceId,
        peerid         = cs.serverId,
        authentication = cs.authentication,
        encryption     = cs.encryption
    }
    if not M.session then return nil, err_msg end

    -- Apply agent.config's settings
    if type(config.server.autoconnect) == "table" then
        log("SRVCON", "DETAIL", "Setting up connection policy")
        for policy, p in pairs(config.server.autoconnect) do
            if policy == "period" then
                if tonumber(p) then
                    timer.new(tonumber(p)*(-60), M.connect, 0)
                else
                    log("SRVCON", "ERROR", "Ignoring unknown period: '%s'", tostring(p))
                end
            elseif policy == "cron" then
                timer.new(p, M.connect, 0)
            elseif policy == "onboot" then
                M.connect(tonumber(p) or 30)
            else
                log("SRVCON", "ERROR", "Ignoring unknown policy: '%s'", tostring(policy))
            end
        end
    end

    return "ok"
end

return M

