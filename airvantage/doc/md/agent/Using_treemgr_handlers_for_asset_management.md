Using treemgr handlers for asset management
===========================================

**[WORK IN PROGRESS]**

This is a proposal to reuse treemgr as a means for asset to be monitored
by the server.

Rationale

The current system as shown some serious shortcoming in real-life use:

-   it's primarily optimized to let servers write data into nested Lua
    tables, but that's not such a frequent use case. At least, most
    applications want to be notified about the writing, maybe in
    addition to storing the values in a table
-   it proved trickier than anticipated to store server-sent data in
    persisted tables
-   it's confusing to have two distinct tree APIs, with slitghtly
    different concepts underneath
-   it's purely passive, there's no way to notify the server except
    through a completely distinct mechanism based on :pushdata().

So we consider the possibility to let users manage server exchanges
through treemgr handlers, which they can either write themselves or
derive from handler factories provided by us. These handlers would be
mounted in a standard place in the logical tree, but would execute
through Racon RPC, so that the user code runs in a user process. We
would also like to extend the registration process, so that it supports
typical monitoring filters (on deadband, threshold etc.), and can be
configured to report to the server.

#### General design

the Agent has a data tree; its variables can be get/set/monitored
by 3rd party applications, and get/set by the server. We want to let 3rd
party applications extend the tree with custom variables, which will
thus also become accessible to the server. 3rd party applications
interact with the server through assets, which can either reflect an
actual physical object (a thermometer, a compressor...) or be a software
abstraction. Applications can create assets with
"racon.newAsset(assetId)", then use them to push data to the
server, and configure them to react properly to server requests.

To the existing asset API, we add:

asset:mount([path,] handler)

-   path: optional path relative to the asset's root node, defaults to
    the root path "".
-   handler: object which provides method :get(), and optionally methods
    :set(), :register() and :unregister()
-   returns "ok" or nil + error msg\
     the asset tree is accessible in treemgr under node
    "assets.<assetid\>". This means that a node of path "x.y", which in
    principle belongs to the device asset, should also be accessible as
    "assets.@sys.x.y". Whether this is important and worth implementing
    remains to be seen.

Whenever a ReadNode command or a data write is received from the server,
it is translated into a :get() or :set() operation on path
"assets.<asset_id>.<path>".

asset:register(active_vars, callback, [passive_vars],
[sampling_period], [filter_name], [filter_args...])

-   active_vars: variable or list of variables whose value changes must
    be monitored
-   callback: function to be run every time a value change is detected
    and accepted. The function receives as parameter a table of
    path/value pairs, where every active var whose value changed, plus
    every passive var whether its value changed or not, is stored.
-   passive_vars: variables whose value must be passed to the callback
    every time it's called, even if the passive var's value didn't
    change.
-   sampling_period: if present, the variable's value is sampled at
    this period (in seconds). This allows to monitor variables who can't
    notify changes by themselves. If absent, the system relies on the
    handlers' ability to notify changes. If a sampling period is given,
    the variables are monitored by sampling only, even if they would
    have been able to notify their changes by themselves.
-   filter_name: if present, filter out some irrelevant changes,
    according to policies such as "min_threshold", "max_threshold",
    "deadband" etc. Most filters take additional parameters, passed
    after the filter name.<
-   returns "ok" or nil + error msg\
     It can be useful to register a variable for report to the server.
    This can be done by passing the appropriate callback:

~~~~{.lua}
function report_to_server(policy)
    return function(asset, map)
               asset:pushdata(map, policy)
               return 'ok'
           end
end
~~~~

It is also desirable to subscribe to such data monitoring from the
server. A standard function inpired by ReadNode should do the trick:
MonitorNode(active_vars, [passive_vars], policy, [sampling_period],
[filter], [filter_args...])

TODO: how to unregister? We have to define a primary key, or to send
some token to the server.

Note that get/set operations on asset nodes must not be performed
directly: user code must run on the user-side of the Racon connector, so
these operations must happen through Racon's RPC system. Similarly,
notification by the user must go through Racon. This might mean that a
function distinct from treemgr.notify() will have to be used, which goes
through Racon; an asset:notify() API would probably make sense.

#### Mounting multiple handlers

Users can mount several handlers on several nodes. It allows to reuse
handler factories provided with the agent, for common needs (e.g.
storing values in RAM or flash, handling commands...); they can still
create their own handlers for more specialized needs. In effect, instead
of providing one kind (__default) of handling functions inside a
nested tables structure, user associate get/set/register handling
functions with paths were they are used. Writing in a nested tables
structure remains possible, but given that it is much less frequently
needed than we anticipated, the system isn't optimized for this use case
anymore.

TODO: Examples.

