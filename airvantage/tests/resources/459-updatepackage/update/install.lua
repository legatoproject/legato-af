local sched = require "sched"
local dt = require "devicetree"

function runApp()
    dt.init()
    dt.set("config.appstarted", "yes")
end

runApp()

