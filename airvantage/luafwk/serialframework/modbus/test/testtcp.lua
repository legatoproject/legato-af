-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

function slog(buff)
  if not buff then return end
  local s
  if type(buff) == "table" then
    s = table.concat(buff, ",")
  else
    s = tostring(buff)
  end
  print(s)
end

require 'modbustcp'
m1,err=modbustcp.new({timeout=1,maxsocket=2}) slog(err)
host='10.41.51.36'
slave = 1

data,err=m1:request('ReadInputRegisters',host,nil,slave,0,2);if err then slog("ReadInputRegisters init - "..err) end
if data then
  res={string.unpack(data,"<H2")}
  res={string.unpack(data,"{H2")}
  res={string.unpack(data,">H2")}
  res={string.unpack(data,"}H2")}
  res={string.unpack(data,"<I")}
  res={string.unpack(data,"{I")}
  res={string.unpack(data,">I")}
  res={string.unpack(data,"}I")}
  res={string.unpack(data,"b4")}
  table.remove(res,1) slog(res)
end


------------------------------------
--Infinite test:
--(Copy paste this in Lua shell) 
------------------------------------



function slog(buff)
  if not buff then return end
  local s
  if type(buff) == "table" then
    s = table.concat(buff, ",")
  else
    s = tostring(buff)
  end
  print(s)
end

require 'modbustcp'
m1,err=modbustcp.new({timeout=1,maxsocket=2}) slog(err)
host='10.41.51.36'
slave = 1

function poll2(n)
 local choice = 0
 while true do
  local data,err,res,_
  if choice == 0 then
     slog(n.." - read holding registers")
     data,err=m1:readHoldingRegisters(host,nil,slave,2000,40);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"<H40")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 1 then
     slog(n.." - read input registers")
     data,err=m1:readInputRegisters(host,nil,slave,2000,10);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"<H10")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 2 then
     slog(n.." - read coils")
     data,err=m1:readCoils(host,nil,slave,2000,10);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"b2")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 3 then
     slog(n.." - read discrete inputs")
     data,err=m1:readDiscreteInputs(host,nil,slave,0,10);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"b2")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 4 then
     slog(n.." - write single register")
     _,err=m1:writeSingleRegister(host,nil,slave,2000,1255);if err then slog(n.." - "..err) end

  elseif choice == 5 then
     slog(n.." - write single coil")
     _,err=m1:writeSingleCoil(host,nil,slave,2000,true);if err then slog(n.." - "..err) end

  elseif choice == 6 then
     slog(n.." - write multiple coils")
     data = string.pack("x8",true,false,false,true,false,true,false,true)
     _,err=m1:writeMultipleCoils(host,nil,slave,2000,8,data);if err then slog(n.." - "..err) end

    elseif choice == 7 then
     slog(n.." - write multiple registers")
     data = string.pack("<H10",456,258,753,951,159,357,654,852,0,1)
     _,err=m1:writeMultipleRegisters(host,nil,slave,2000,data);if err then slog(n.." - "..err) end
    end
    --slog("collect:"..collectgarbage("collect"))
    slog(n.." - count:"..collectgarbage("count"))
    choice = (choice + 1)%8
    sched.wait(1)
 end
end

function poll1(n)
 local choice = 0
 while true do
  local data,err,res,_
  if choice == 0 then
     slog(n.." - read holding registers")
     data,err=m1:request('ReadHoldingRegisters',host,nil,slave,2000,40);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"<H40")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 1 then
     slog(n.." - read input registers")
     data,err=m1:request('ReadInputRegisters',host,nil,slave,2000,10);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"<H10")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 2 then
     slog(n.." - read coils")
     data,err=m1:request('ReadCoils',host,nil,slave,2000,10);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"b2")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 3 then
     slog(n.." - read discrete inputs")
     data,err=m1:request('ReadDiscreteInputs',host,nil,slave,0,10);if err then slog(n.." - "..err) end
     if data~=nil then
       res={string.unpack(data,"b2")}
       table.remove(res,1) slog(res)
     end

  elseif choice == 4 then
     slog(n.." - write single register")
     _,err=m1:request('WriteSingleRegister',host,nil,slave,2000,1255);if err then slog(n.." - "..err) end

  elseif choice == 5 then
     slog(n.." - write single coil")
     _,err=m1:request('WriteSingleCoil',host,nil,slave,2000,true);if err then slog(n.." - "..err) end

  elseif choice == 6 then
     slog(n.." - write multiple coils")
     data = string.pack("x8",true,false,false,true,false,true,false,true)
     _,err=m1:request('WriteMultipleCoils',host,nil,slave,2000,8,data);if err then slog(n.." - "..err) end

    elseif choice == 7 then
     slog(n.." - write multiple registers")
     data = string.pack("<H10",456,258,753,951,159,357,654,852,0,1)
     _,err=m1:request('WriteMultipleRegisters',host,nil,slave,2000,data);if err then slog(n.." - "..err) end
    end
    --slog("collect:"..collectgarbage("collect"))
    slog(n.." - count:"..collectgarbage("count"))
    choice = (choice + 1)%8
    sched.wait(1)
 end
end
--t1=sched.run(poll1,1)
t2=sched.run(poll2,1)
t3=sched.run(poll1,3)