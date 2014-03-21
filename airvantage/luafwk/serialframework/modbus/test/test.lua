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

function slog(buff)
    if not buff then return end
    local s
    if type(buff) == "table" then
        s = table.concat(buff, ",")
    else
        s = tostring(buff)
    end
    log('T','INFO', s)
end

function testall(m)
    local id = "test"
    local slave = 1
    local res, err, data
    for value=0, 100, 10 do
        res=assert(m:request('WriteSingleRegister',slave,value+50,value))
        data=assert(m:request('ReadHoldingRegisters',slave,value+50,1))
        res={string.unpack(data,"<H")}; table.remove(res,1)
        assert(value == res[1])

        data=assert(m:request('ReadInputRegisters',slave,value+50,1))
        res={string.unpack(data,"<H")}; table.remove(res,1)
        assert(value == res[1])

        res=assert(m:request('WriteMultipleRegisters',slave,value+50,string.pack("<H8",21,159,357,654,852,65535,65536,0)))
        data=assert(m:request('ReadHoldingRegisters',slave,value+50,8))
        res={string.unpack(data,"<H8")}; table.remove(res,1)
        assert(
        21 == res[1] and 159 == res[2] and 357 == res[3] and
        654 == res[4] and 852 == res[5] and 65535 == res[6] and
        65536 == res[7] and 0 == res[8])

        data,err=assert(m:request('ReadInputRegisters',slave,value+50,8))
        res={string.unpack(data,"<H8")}; table.remove(res,1)
        assert(
        21 == res[1] and 159 == res[2] and 357 == res[3] and
        654 == res[4] and 852 == res[5] and 65535 == res[6] and
        65536 == res[7] and 0 == res[8])


        res=assert(m:request('WriteMultipleRegisters',slave,value+50,string.pack(">H8",21,159,357,654,852,65535,65536,0)))
        data=assert(m:request('ReadHoldingRegisters',slave,value+50,8))
        res={string.unpack(data,">H8")} table.remove(res,1)
        assert(
        21 == res[1] and 159 == res[2] and 357 == res[3] and
        654 == res[4] and 852 == res[5] and 65535 == res[6] and
        65536 == res[7] and 0 == res[8])

        data=assert(m:request('ReadInputRegisters',slave,value+50,8))
        res={string.unpack(data,">H8")}; table.remove(res,1)
        assert(
        21 == res[1] and 159 == res[2] and 357 == res[3] and
        654 == res[4] and 852 == res[5] and 65535 == res[6] and
        65536 == res[7] and 0 == res[8])

    end
    slog("WriteSingleRegister/ReadHoldingRegisters/[ReadInputRegisters]/WriteMultipleRegisters OK")

    for value=0, 100, 10 do
        res=assert(m :request('WriteSingleCoil',slave,value+50,true))
        data=assert(m :request('ReadCoils',slave,value+50,1))
        res={string.unpack(data,"x8")} table.remove(res,1)
        assert(true == res[1])

        data=assert(m :request('ReadDiscreteInputs',slave,value+50,1))
        res={string.unpack(data,"x8")} table.remove(res,1)
        assert(true == res[1])


        res=assert(m :request('WriteSingleCoil',slave,value+50,false))
        data=assert(m :request('ReadCoils',slave,value+50,1))
        res={string.unpack(data,"x8")} table.remove(res,1)
        assert(false == res[1])

        data=assert(m :request('ReadDiscreteInputs',slave,value+50,1))
        res={string.unpack(data,"x8")} table.remove(res,1)
        assert(false == res[1])


        res=assert(m :request('WriteMultipleCoils',slave,value+50,8,string.pack("x8",true,false,false,true,false,true,false,true)))
        data=assert(m :request('ReadCoils',slave,value+50,8))
        res={string.unpack(data,"x8")} table.remove(res,1)
        assert(true == res[1],false == res[2],false == res[3],true == res[4],false == res[5],true == res[6],false == res[7],true == res[8])

        data=assert(m :request('ReadDiscreteInputs',slave,value+50,8))
        res={string.unpack(data,"x8")} table.remove(res,1)
        assert(true == res[1],false == res[2],false == res[3],true == res[4],false == res[5],true == res[6],false == res[7],true == res[8])

    end
    slog("WriteSingleCoil/ReadCoils/[ReadDiscreteInputs]/WriteMultipleCoils OK")
end

function poll(m,id)
    local choice = 0
    local slave = 1
    local data,err,res
    while true do
        res = nil
        if choice == 0 then
            slog(id.."->read holding registers")
            data=assert(m :request('ReadHoldingRegisters',slave,90,40))
            if data~=nil then
                res={string.unpack(data,"<H40")}
                res[1]=id.."->"
            end

            elseif choice == 1 then
            slog(id.."->read input registers")
            data=assert(m :request('ReadInputRegisters',slave,0,10) )
            if data~=nil then
            res={string.unpack(data,"<H10")}
            res[1]=id.."->"
            end

        elseif choice == 2 then
            slog(id.."->read coils")
            data=assert(m :request('ReadCoils',slave,0,10))
            if data~=nil then
                res={string.unpack(data,"b2")}
                res[1]=id.."->"
            end

        elseif choice == 3 then
            slog(id.."->read discrete inputs")
            data=assert(m :request('ReadDiscreteInputs',slave,0,10) )
            if data~=nil then
                res={string.unpack(data,"b2")}
                res[1]=id.."->"
            end

        elseif choice == 4 then
            slog(id.."->write single register")
            res=assert(m :request('WriteSingleRegister',slave,9,1255))
            if res then res=id.."->"..res end

        elseif choice == 5 then
            slog(id.."->write single coil")
            res=assert(m :request('WriteSingleCoil',slave,9,true))
            if res then res=id.."->"..res end

        elseif choice == 6 then
            slog(id.."->write multiple coils")
            data = string.pack("x8",true,false,false,true,false,true,false,true)
            res=assert(m :request('WriteMultipleCoils',slave,0,8,data))
            if res then res=id.."->"..res end

        elseif choice == 7 then
            slog(id.."->write multiple registers")
            data = string.pack("<H8",21,159,357,654,852,357,654,852)
            res=assert(m :request('WriteMultipleRegisters',slave,90,data))
            if res then res=id.."->"..res end
        end
        if err then slog(id.."->"..err) end
        if res then slog(res) end
        --slog("collect:"..collectgarbage("collect"))
        --slog(id.."->count:"..collectgarbage("count"))
        choice = (choice + 1)%8
        sched.wait(1)
    end
end

------------------------------------LINUX---------------------------------------------------------------
--baudrate=19200
--mode='RTU'
--data=8
--parity='odd'
--timeout=1

------------------------------------STOP---------------------------------------------------------------
sched.kill(t1)
t1=nil
sched.kill(t2)
t2=nil
m1 = nil
m2 = nil
at("at+wmfm=0,1,1")
at("at+wmfm=0,1,2")

------------------------------------UART 1---------------------------------------------------------------
at("at+wmfm=0,0,1")
portname = 'UART1'
baudrate=19200
mode='RTU'
data=8
parity='odd'
timeout=2

require 'modbus'
require 'pack'
m1=assert(modbus.new(portname ,{baudrate=baudrate,parity=parity,data=data,timeout=timeout}, mode))

t1=sched.run(poll, m1, "[1]")

------------------------------------UART 2---------------------------------------------------------------
at("at+wmfm=0,0,2")
portname = 'UART2'
--portname = '/dev/ttyS0'
--baudrate=19200
--mode='ASCII'
--data=7
--parity='even'
--timeout=1
baudrate=19200
mode='RTU'
data=8
parity='odd'
timeout=2

require 'modbus'
require 'pack'
m2=assert(modbus.new(portname ,{baudrate=baudrate,parity=parity,data=data,timeout=timeout}, mode))

t2=sched.run(poll, m2, "[2]")

------------------------------------ENDIANESS---------------------------------------------------------------
require 'modbus'
require 'pack'

portname = 'UART2'
baudrate=19200
mode='RTU'
data=8
parity='odd'
timeout=2
m=assert(modbus.new(portname ,{baudrate=baudrate,parity=parity,data=data,timeout=timeout}, mode))

data=string.pack(">H2",0x1234,0xABCD)
m:request('WriteMultipleRegisters',1,10,data)
buff,err=m:request('ReadHoldingRegisters',1,10,2) p(err)
res={string.unpack(buff,">H2")}
p(string.format("%X - %X",res[2],res[3]))

data=string.pack("<H2",0x1234,0xABCD)
m:request('WriteMultipleRegisters',1,10,data)
buff,err=m:request('ReadHoldingRegisters',1,10,2) p(err)
res={string.unpack(buff,"<H2")}
p(string.format("%X - %X",res[2],res[3]))
