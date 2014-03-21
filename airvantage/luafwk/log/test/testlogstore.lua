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

--test RAM log,
--data written should be equal to data read
--signal coming from lowlevel is used to get data before any overwrite
function testramlog1()
    local str = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"

    local l= require"log.store"
    l.lograminit(2048)
    local n=0;
    local MAX = 150
    local res,err
    local writebuf = ""
    local readbuf = ""
    --[[print("testlog: signal from logramstore"); --]]
    local hk = sched.sigonce("logramstore", "erasedata", function(...)  readbuf=readbuf..(l.logramget() or "nothing!\n") end)


    while n < MAX do
        local nl = str..n.."\n"
        writebuf = writebuf .. nl
        --p("adding... ", nl)
        res,err = l.logramstore(nl)
        assert(res, err)
        n = n+1
        if n%4 == 0 then collectgarbage("collect") end
    end

    print(string.format("testlog #writebuf = %d", #writebuf))

    readbuf=readbuf..(l.logramget() or "nothing!\n");
    if #readbuf ~= #writebuf then
      print(string.format("error: #readbuf = %d, #writebuf = %d", #readbuf, #writebuf))
      print(string.format("readbuf = \n[%s]\n", readbuf))
      print(string.format("error: #readbuf = %d, #writebuf = %d", #readbuf, #writebuf))
    elseif readbuf ~= writebuf then
      print("error: writebuf != readbuf")
      print(string.format("readbuf = \n[%s]\n", readbuf))
      print(string.format("writebuf = \n[%s]\n", writebuf))
      print("error: writebuf != readbuf")
    else
      print("success: writebuf = readbuf")
    end

    sched.kill(hk)
end
--test RAM log,
-- data overwrite occur
-- success test is based upon str content : only one "\n" per log...
--test keep written data (its size must be over lograminit param)
-- at the end, the result of logramget must be
function testramlog2()
    local str = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"

    local l= require"log.store"
    l.lograminit(2048)
    local n=0;
    local MAX = 150
    local res,err
    local writebuf = ""

    while n < MAX do
        local nl = str..n.."\n"
        writebuf = writebuf .. nl
        --p("adding... ", nl)
        res,err = l.logramstore(nl)
        assert(res, err)
        n = n+1
        if n%4 == 0 then collectgarbage("collect") end
    end

    local partialbuf = string.sub(writebuf, -2048)
    local sub =string.find(partialbuf, "\n")
    partialbuf = string.sub(partialbuf, sub+1)

    local readbuf =  l.logramget()

    if #readbuf ~= #partialbuf then
      print(string.format("error: #readbuf = %d, #partialbuf = %d", #readbuf, #partialbuf))
      print(string.format("readbuf = \n[%s]\n", readbuf))
      print(string.format("error: #readbuf = %d, #partialbuf = %d", #readbuf, #partialbuf))
    elseif readbuf ~= partialbuf then
      print("error: partialbuf != readbuf")
      print(string.format("readbuf = \n[%s]\n", readbuf))
      print(string.format("partialbuf = \n[%s]\n", partialbuf))
      print("error: partialbuf != readbuf")
    else
      print("success: partialbuf = readbuf")
    end

end




function testflashsourceoat()
    local res,err

    -- init
    local l = require"log.store"
    assert(l.logflashinit(30*1024))

    -- empty flash memory
    local src =  l.logflashgetsource()
    repeat res = src() until not res

    -- fill flash and save content
    local writebuf = ""
    local str = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
    -- As max number of flash object used to store logs is 255
    -- 256 IDs minus (at least) 1 ID not used
    for i=1,255 do
        local nl = str..i
        assert(l.logflashstore(nl))
        writebuf = writebuf..nl
        if i%4 == 0 then collectgarbage("collect") end
    end


    -- read source
    local readbuf = {}
    local mysinktable = ltn12.sink.table(readbuf)
    ltn12.pump.all(l.logflashgetsource(), mysinktable)
    readbuf = table.concat(readbuf)

    -- control read content
    if #readbuf ~= #writebuf then
      print(string.format("error: #readbuf = %d, #writebuf = %d", #readbuf, #writebuf))
      print(string.format("readbuf = \n[%s]\n", readbuf))
      print(string.format("error: #readbuf = %d, #writebuf = %d", #readbuf, #writebuf))
    elseif readbuf ~= writebuf then
      print("error: writebuf != readbuf")
      print(string.format("readbuf = \n[%s]\n", readbuf))
      print(string.format("writebuf = \n[%s]\n", writebuf))
      print("error: writebuf != readbuf")
    else
      print("success: writebuf = readbuf")
    end

 end


--[[
function to test concurrency on flash space
--]]
function lwrite ()
    local l = require"log.store"
    assert(l.logflashinit(30*1024))
    assert(l.logflashstore("DAMNED"))
    print("log stored")
    return "again"
end
 sched.sighook(timer.new(-4), "run",  lwrite)
