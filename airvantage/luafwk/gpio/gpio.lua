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

local core = require"gpio.core"
local checks = require"checks"
local sched = require"sched"
local schedfd = require"sched.fd"
local lfs = require"lfs"

local M ={}


local reggpio={}

-- function to be set as hook for sched 
-- notification about interrupt on registered GPIOs
--
--must return true value to keep registration alive
local function common_interrupt_hook(io)
    assert(io)
    local id = assert(io:getid())
    local t = assert(reggpio[id])
    local hook = assert(t.userhook)
    --need to use specific core read:
    -- must use the fd used to register the 
    -- GPIO in sched.fd so that interrupt is
    -- cleared
    local value = core.readinterrupt(io)
    -- call the usercall hook with the GPIO id and the value
    sched.run(hook, io:getid(), value)
    return "again"
end


local function register(id, userhook)
    checks("number", "?function")
    if not userhook then
        -- it is unregister request
        if not reggpio[id] then return nil, "gpio is not registered" end
        local io = reggpio[id].gpio
        --unregister from sched
        schedfd.when_exception(io, nil)
        --clean local state
        reggpio[id]=nil
    else
        -- it is register request
        local io, err
        if not reggpio[id] then
            --need to call core API to get userdata embedding low level fd.
            io, err = core.newgpio(id)
            if not io then return nil, err or "register: can't create gpio userdata" end
            -- register the GPIO in sched.fd so that interrupt (i.e. changes) is monitored.
            schedfd.when_exception(io, common_interrupt_hook)
        else
            --retrieve previous registration  GPIO userdata
            io = reggpio[id].gpio
        end

        --update user hook : keep GPIO userdata over sequential regsitrations.
        reggpio[io:getid()]= { gpio=io, userhook=userhook}
    end
    return "ok"
end


--helper function to read GPIO files
local function readfile(filename)
    local res,err
    local f, err = io.open(filename)
    if not f then return nil, err or "can't open file:"..filename end
    res,err = f:read("*a")
    f:close()
    return res,err
end

--read capabilities of all gpiochip* in /sys/class/gpio/ 
local function availablelist()
    local res = {}
    local gpiodir = "/sys/class/gpio/"
    for file in lfs.dir(gpiodir) do
        if file:match("gpiochip") then
            --`base` contains the first GPIO id provided by this gpiochip
            local base, err = readfile(gpiodir.."/"..file.."/base")
            if not base then return nil, err end
            --`ngpio` contains the number of GPIO provided by this gpiochip
            local number, err = readfile(gpiodir.."/"..file.."/ngpio")
            if not number then return nil, err end
            for i = base,number-1 do table.insert(res, i) end
        end
    end
    return res
end

--get all enabled gpio, using 
-- /sys/class/gpio/gpioN folder existence as discrimination.
local function enabledlist()
    local res = {}
    local gpiodir = "/sys/class/gpio/"
    for file in lfs.dir(gpiodir) do
        local id = file:match("gpio(%d+)") 
        id = tonumber(id)
        if id then
            table.insert(res, id)
        end
    end
    return res
end


--Public API:
M.read = core.read
M.write = core.write
M.configure = core.configure
M.getconfig = core.getconfig
M.availablelist = availablelist
M.enabledlist = enabledlist
M.register = register

return M