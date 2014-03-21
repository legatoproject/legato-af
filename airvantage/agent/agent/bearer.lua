-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require "sched"
local systemutils = require "utils.system"
local config = require "agent.config"
local socket = require "socket"
local tableutils = require "utils.table"
local setmetatable = setmetatable
local pairs = pairs
local table = table
local io = io
local os = os
local tostring = tostring
local string = string
local next = next
local select = select
local type = type
local assert = assert
local tonumber = tonumber

local BIN_PATH = (LUA_AF_RO_PATH or "./").."bin/"


module(...)

local BEARER = {}
BEARER.__index = BEARER

local function initialize(name)
    local argstable = {"init"}

    -- concat all args into a single string
    if config.network.bearer[name] then
        for k,v in pairs(config.network.bearer[name]) do
            table.insert(argstable, k .. "=" .. tostring(v))
        end
    end
    --add specific configurations
    if name == "GPRS" then
        assert(config.modem.pppport and type(config.modem.pppport) == "string", "When mounting GPRS device, config.modem.pppport need to be defined (as a string)")
        table.insert(argstable, "pppport="..config.modem.pppport)
    end

    -- execute the init script
    local status, read = systemutils.pexec(BIN_PATH.. name .. " " .. table.concat(argstable, " "))
    -- check exit status
    if status ~=0 then return nil, string.format("Initiliazation script failed with status [%s] and output [%s]", status, read or "unknown") end
    return read:match("%w+")
end





function new(name, automount)
    local itr, err = initialize(name)
    if itr then
        local ber = setmetatable({name = name or "unknown", interface = itr, connected = false, mounting = false, automount = automount, infos  = {}}, BEARER)
        return ber
    else
        return nil, err
    end
end

function BEARER:unmount()
    os.execute(BIN_PATH .. self.name .. " stop " .. self.interface)
    self.connected = false
end

function BEARER:mount()
    if self.connected then return true end
    self.mounting = true

    self:unmount()
    -- execute the start script
    -- copy args
    -- concat all args into a single string
    local argstable = {"start "..self.interface}
    local defaultconfig = config.network.bearer[self.name]
    -- get current config
    if defaultconfig then
        for k,v in pairs(defaultconfig) do
            table.insert(argstable, k .. "=" .. tostring(v))
        end
    end
    local status, read = systemutils.pexec(BIN_PATH .. self.name .. " " .. table.concat(argstable, " "))

    if status==0 and read then
        --parse returned values
        --self.infos["ipaddr"], self.infos["hw_addr"], self.infos["netmask"], self.infos["gw"] =read:match()
        local infos_to_get = { "ipaddr", "hwaddr", "netmask", "gw"}
        local i = 1
        local new_info
        for new_value in string.gmatch(read, "%S+") do
            new_info = infos_to_get[i] or "nameserver"..(i-4)
            i = i+1
            self.infos[new_info] = new_value~="NULL" and new_value or nil
        end

    else
        os.execute(BIN_PATH .. self.name .. " stop " .. self.interface)
        self.mounting = false
        self.connected = false
        sched.signal("NETMAN-BEARER", "MOUNT_FAILED", self)
        return nil, string.format("Bearer:mount for bearer [%s] Failed: script status = [%s], output = [%s]", self.name, status, read or "nothing")
    end

    self.start = os.time()
    self.mounting = false
    self.connected = true
    sched.signal("NETMAN-BEARER", "MOUNTED", self)
    return true
end


function BEARER:setdefault()
    local gateway = self.infos["gw"] or self.infos["ipaddr"]
    local dns
    for k,v in pairs(self.infos) do
        if string.match(k,"nameserver") then dns = (dns or "")..v.." " end
    end
    if gateway and dns then
        local cmd = BIN_PATH .. self.name .. " default " .. self.interface .. " " .. gateway.." "..dns
        local status = os.execute(cmd)
        if status == 0 then
            return "ok"
        end
    end
    return nil, "Cannot set '"..self.name.."' as gateway"
end
