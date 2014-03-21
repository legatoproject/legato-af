local sched = require "sched"
local dt = require "devicetree"

function runApp()
    dt.init()
    dt.set("config.appstarted", "yes")

    --while (true) do
    --    sched.wait(1)
    --end
end

runApp()

