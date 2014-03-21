-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local logstore = require'log.store'
local u = require 'unittest'
local config = require 'agent.config'

local t=u.newtestsuite("logstore")

local cfg = {}
cfg.ramlogger={size = 2048}

if _OSVERSION:match("^Linux") then
    cfg.flashlogger={size=15*1024, path="logs/"}
else
    cfg.flashlogger={size=2*1024 }
end


function t:setup()
    --assert no log policy running
    u.assert_nil(config.log.policy)
    u.assert(logstore.lograminit(cfg.ramlogger))
end

local function fillrambuf()
    local buf="sqddlmsqkldkmlsqkdlmkslmqkdlmkslmqkdlmkslmqk"
    local buftotal=""
    local totalsize=0
    while (totalsize + #buf < cfg.ramlogger.size) do
        --+2: implementation detail: log is stored with its size on 2 bytes
        totalsize = totalsize + #buf + 2
        buftotal = buftotal..buf
        u.assert(logstore.logramstore(buf))
    end
    local getbuf = logstore.logramget()
    u.assert_equal(buftotal, getbuf)
end

function t:test_01_simpleram()
    for i=1,100 do
        fillrambuf()
    end
end

local function fillram_catchsig()
    local buf="llldlsdsdsdpdpp:c,qnqpqmqmdfkfkfkfkfkfk"
    local buftotal=""
    local totalsize=0;
    local getbuf= ""
    --empty buffer
    logstore.logramget()

    local function acclog()
        local tmp,err =  logstore.logramget()
        u.assert(tmp, err)
        getbuf = getbuf .. tmp
    end
    local hook = sched.sighook("logramstore", "erasedata", acclog)

    while (totalsize < cfg.ramlogger.size *10) do
        --+2: implementation detail: log is stored with its size on 2 bytes
        totalsize = totalsize + #buf + 2
        buftotal = buftotal..buf
        u.assert(logstore.logramstore(buf))
    end
    acclog()
    u.assert_equal(buftotal, getbuf)
    sched.kill(hook)

end

function t:test_02_advancedram()
    for i=1,2 do
        fillram_catchsig()
    end
end


function t:test_03_clean_init_flash_logs()
    if _OSVERSION:match("^Linux") then
        u.assert(os.execute("rm logs/*.log"))
        u.assert(logstore.logflashinit(cfg.flashlogger))
    else
         local hdl, err = flash.create("LuaLogPersistHdl")
         if hdl then
             flash.reset(hdl)
         end

         u.assert(logstore.logflashinit(cfg.flashlogger))
    end
end



local function getallflash()
    local source = u.assert(logstore.logflashgetsource())
    local buffer
    local t = {}
    local sink = ltn12.sink.table(t)
    u.assert(ltn12.pump.all(source, sink))
    buffer = table.concat(t)
    return buffer
end


local function fillflash(maxsize)
    local buf="llldlsdsdsdpdpp:c,qnqpqmqmdfkfkfkfkfkfk"
    local buftotal=""
    local totalsize=0;
    local getbuf= ""
    while (totalsize + #buf < maxsize ) do
        totalsize = totalsize + #buf
        buftotal = buftotal..buf
        u.assert(logstore.logflashstore(buf))
    end
    local res  = getallflash()
    u.assert_equal(buftotal, res)
end


function t:test_04_flash()
    if _OSVERSION:match("^Linux") then
        fillflash(cfg.flashlogger.size *2 - 250)
    else
        fillflash(cfg.flashlogger.size )
    end
end




