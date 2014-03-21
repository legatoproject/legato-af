local sched = require "sched"
local dt = require "devicetree"

function run()
    dt.init()
    dt.set("config.appstarted", "no-auto")

    while (true) do
    sched.wait(1)
    end
end

run()

