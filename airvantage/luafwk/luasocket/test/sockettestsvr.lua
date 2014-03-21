--test server to run before using socket unit test suite.
-- this must be run using standard Lua VM and standard luasocket lib, not the luasocket lib provided by Sierra
-- luafmk folder (because this version depends on sched, and this file is not compatible yet with sched)
-- to get standard Lua VM on your system, install those packages:
-- - lua5.1
-- - liblua5.1-socket2
-- start this file doing: $ lua sockettestsvr.lua
--
socket = require("socket");
host = host or "localhost";
port = port or "8383";
server = assert(socket.bind(host, port));
ack = "\n";

function runserver()
    while 1 do
        print("server: waiting for client connection...");
        control = assert(server:accept());
        while 1 do
            command = assert(control:receive());
            assert(control:send(ack));
            print(command);
            (loadstring(command))();
        end
    end
end

while(1) do
    local res, err = pcall(runserver)
    print(string.format("Error while running server: res=[%s], err=[%s]", tostring(res), tostring(err)))
end
