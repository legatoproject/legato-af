Racon Lua library
======================


#### 1. Presentation

The Racon Connector library provides access to the M2M
platform through the Agent. On architectures supporting multiple
processes, it is a library which dialogs with the Agent process
through a dedicated IPC channel, most commonly a socket.

Communications with the server include buffering and consolidating data
on the embedded side, flushing them to the server according to
customizable policies, subscribing to server-side or hardware events and
value changes, acknowledge management.

The Agent naturally reasons by assets: connections to it are
associated with a given asset when established, events and notifications
are dispatched by asset, etc. The Agent's logical assets might map
one-to-one with physical assets connected to the physical device, but
they don't have to. Within an asset, data are organized into tree paths,
and it is possible to subscribe to data or event limited to any
arbitrary sub-path.

As a general principle, the Agent takes responsibility for the data
that have been passed to it: it is in charge of optimizing their
encoding and regrouping, of handling acknowledgements, of securing them
in non-volatile memory if asked to, etc.

In addition to offering communication means to the server, the
Racon library also gives access to other Agent features: SMS
communications, hardware I/O control and monitoring, etc.

#### 2. Create a New Asset Instance, Initialize a New Asset

Creating a new asset is as simple as calling `racon.newasset()`
with a mandatory asset id string.\
 It returns an Racon asset object, which acts on behalf of the
asset.\
 An asset does not necessarily represent a physical piece of hardware;
it is merely a representation of the global application architecture.

At this point, the asset is created but is not ready to communicate with
the Agent. To make it active, the `start` method must be called
first, after any required event subscription has been performed.

~~~~{.lua}
local racon = require "racon"
racon.init() -- configure the module

-- create asset
asset = racon.newAsset("newassetid")
-- register asset
local status, err = asset :start()
if not status then print("Error when starting the asset: "..err)
end
~~~~

The asset identifier is the root element of the message path. For
instance, if the variable "foo" is written under path "bar" in the asset
"myAsset", the complete path as seen by the server is "myAsset.bar.foo".

#### 3. Data Sending

##### 3.1. Generalities

Data can be sent with two different APIs:

-   Through pre-declared tables, where the user can specify the table's
    persistence mode, columns, serialization options, etc.
-   Directly, by passing data in a Lua table. This is the most flexible
    way in terms of what can be sent, but it does not allow you to
    customize storage and emission settings.

##### 3.2. Event Sending

Noteworthy events are notified between the assets and the server by
sending data. When designing an application, some conventions are taken
about which paths represent "normal" data, and which ones\
 will be used to represent events. The embedded application will
subscribe to these paths and hook a proper handling function on them;
symmetrically, the server will be configured to react to "event\
 paths" by causing the appropriate notifications.

Both data sending APIs can be used (raw data or predeclared tables).
However, given the generally simple and immediate kind of data exchange
suitable to event notifications, the raw data API will most often be the
best choice.

##### 3.3. Policies

Data is not normally sent to the server as soon as the user requests it.
It is accumulated by the Agent to limit the number of connections,
to optimize their encoding, and optionally to perform some data
consolidation operations on them. Data emission is performed according
to a policy, which can be:

- cron="cron_config": The operation is performed at predefined
  dates, specified using the standard Unix cron syntax.
- latency=n: The operation will be performed in `n` seconds at most.
- manually=true: The connection will be triggered by the application
  through an explicit call to `racon.triggerPolicy()`.
- boot=n: The operation will be performed `n` seconds after the
  `racon` module has been initialized.

There can be multiple policies set up on a single device, and Racon
APIs allow to choose according to which policy when each data is sent.

###### 3.3.1. Policy Examples

If `cron="*"`, the device will connect at the beginning of every hour if
there is something to send. Even if a connection is forced at 8:59, the
cron connection will still be forced at 9:00.

If `latency=60*60`, the device will connect every one hour: the first
connection will be done one hour after the device booted. However, if a
periodic connection is due at 9:00, and a connection is forced at 8:59,
the next connection will occur at 9:59 rather than at 9:00. This policy
guarantees that data will not be buffered for more than 60*60 seconds
before being sent to the server, and that no more than 60*60 seconds
will pass between two connections. (Connections not only allow data to
be sent to the server, but also to receive data sent by it).

`boot=n` policy rules are mostly useful if some unsent, persisted data
might have been left unsent before a reboot. With such a policy, data
unsent because of a shutdown will be sent when the device reboots.

###### 3.3.2. Policy Configurations

To allow the handling of several policies, policy queues can be
declared, in unlimited number, in the agent's configuration. They are
indexed by name in `"data.policy"`, and contain a table with one key and
one value. The key is one of `"cron", "latency", "manually"`. More than
one pair can be given for a single policy.

-   cron policies are triggered at predefined dates, which can be
    described with the same formalism as POSIX' cron task scheduler;
-   latency policies are guaranteed to be triggered before a given delay
    in seconds elapses. The delay, when non-zero, leaves a chance to
    group several triggerings together and to save some bandwidth;
-   manual policies are only triggered when the user application
    explicitly triggers them.

Some policies must exist and have certain properties:

-   `"default"` is the policy chosen when a data sending command is
    given with no explicit policy. Users can change the way this policy
    is triggered.
-   `"never"` cannot be triggered: data associated with this policy are
    never sent.
-   `"now"` is intended for data which are to be sent (almost)
    immediately. It must be configured to be sent with a latency, and in
    normal situations, this latency should be short, no more than a
    couple of seconds.

####### 3.3.2.1. Policy configuration examples

~~~~{.lua}
agent.config.set('data.policy.hourly.latency', 60*60) -- this policy connects at least once per hour
agent.config.set('data.policy.manual.manually', true) -- this policy only connects when the user requests it
agent.config.set('data.policy.midnight.cron', '0 0 * * \*') -- this policy connects at midnight
agent.config.set('data.policy.default.latency',30) -- change default policy step 1
agent.config.set('data.policy.default.1',nil)      -- change default policy step 2 (optional)
~~~~

####### 3.3.2.2. Why policies?

Policies introduce an indirection in the way data are flushed from the
agent to the server. This indirection is extremely precious over the
lifetime of an M2M application: policies can be changed remotely, by
asking the server to update the agent's configuration. If data
triggering were done explicitly by the application, adapting the
reporting policy to changing conditions (bandwidth congestion and
billing, change in usage patterns...) owuld require a full software
update.

There are a couple of legitimate, very advanced cases where an
application needs direct control over data sending policies, and this
can be done by forcing the triggers on a manual-only policy. But it
should be avoided in most reasonable use cases, and represents a
liability for long term maintenance.

##### 3.4. Sending Undeclared Data

~~~~{.lua}
asset :pushData (?path, record, ?policy)
~~~~

This function sends some data according to the specified policy (or the
default policy if unspecified). The record can be a flat key/value set
of values, which will be stored under the specified path (relative to
the asset's root on the server).\
 If the record contains nested sub-tables, it will be interpreted as
several sub-trees (cf. examples).\
 If the path is omitted, data is stored at the asset's root.\
 If the record is not a table, the last segment of the path is used as
only record key.

Notice that for the server, all data is timestamped. If there is no
'timestamp' or 'timestamps' entry on a record, it will be timestamped at
the date of its reception by the server.

###### 3.4.1. Examples

~~~~{.lua}
asset :pushData('foo', {x=1, y=2}) -- sends <assetroot>.foo.x=1, <assetroot>.foo.y=2
asset :pushData('foo.x', 1, 'midnight') -- will send <assetroot>.foo.x=1 according to the "midnight" policy
asset :pushData('foo', {x=1, y={a=2, b=3}}) -- sends <assetroot>.foo.x=1, <assetroot>.foo.y.a=2, <assetroot>.foo.y.b=3
~~~~

##### 3.5. Predeclared Data Tables

Data tables can be declared as follows:

~~~~{.lua}
tbl = asset :newTable(path, columns, ?storage, ?policy)
~~~~

A table is associated with a given path (relative to the asset's root)
and policy (default policy if unspecified). It can also have either a
`"ram"` or `"file"` storage mode. Finally, it predeclares the columns
which it handles; all records later pushed in the table must conform to
this set of columns. The paramater columns is a list of column names as
strings.

In the future, columns will support more advanced settings, including
serialization policies which affect their bandwidth usage vs. data
precision compromise.

The result of this function is a table proxy object `tbl`, which
supports methods `:pushRow()` to feed data in it, and `:send()` to force
an immediate flush to the server, bypassing any policy setup.

###### 3.5.1. Example

~~~~{.lua}
tbl = asset :newTable('position', {'latitude', 'longitude', 'altitude', 'timestamp'}, 'ram', 'hourly')
~~~~

###### 3.5.2. Pushing a Row in a Predeclared Table

~~~~{.lua}
tbl :pushRow{k_1=v_1, ..., k_n=v_n}
~~~~

Push a row of data in a predeclared table. Keys must match the
predeclared column names.

###### 3.5.3. Example

~~~~{.lua}
tbl :pushRow{latitude=y, longitude=x, altitude=z, timestamp=os.time()}
~~~~

###### 3.5.4. Forcing the Emission of a Table Content

~~~~{.lua}
tbl :send(?no_reset)
~~~~

Force the immediate emission of the table's content to the server. The
table content will be erased unless either the transmission fails, or
the `no_reset` argument is true.

###### 3.5.5. Forcing a Policy Flush

All the data attached to a given policy can be flushed to the server
immediately with function:

~~~~{.lua}
racon.triggerPolicy(?policy_name)
~~~~

If no `policy_name` is passed, the default policy is used. If the
special name `"*"` is passed, all policies are flushed.

#### 4. Data Reception (Data Writing)

##### 4.1. Asset `Data Tree`

The Racon asset instance (returned by a call to
`racon.newAsset()`) contains a `data tree`: a set of nested lua
tables, stored in the asset's `tree` field. Data sent by the server will
be written in this tree, under the proper path, unless a handler
function has been set up in the tree. If a suitable handler function is
found in the tree, data writings sent by the server are passed to that
handler rather than written in the tree.

FYI, server can only modify data defined in datamodel as a setting, and
not a variable.

##### 4.2. Registering a Function on the `Data Tree`

The `data tree`, or any sub path (branch) of it, can be set to a
function.\
 In that case, a data writing will trigger a function call with the
remaining path and value as parameters.\
 It is possible to set a default function handler for a path by setting
the key `__default` to a function in the given path.

Data writing handlers can either:

-   fail with nil+message, or by returning a non-zero numeric status
    code; in this case, if the server required an acknowledgement, a
    failure will be reported to it.
-   succeed and return either the `"ok"` string or the number 0: if the
    server required an acknowledgement, success will be reported to it
    as a response
-   succeed and return the `"async"` string as a result: this means the
    handler takes acknowledgement in charge, and will eventually call
    `portal.acknowledge` to report to the server.

Data handlers receive the following parameters:

-   the Racon asset instance to which the data was destined;
-   the data, as a table of key/value pairs; keys are suffixes to the
    common path prefix (see next parameter);
-   a path prefix, common to all data; actual data paths are the set of
    `utils.path.concat(path, key)` for all keys in the 2nd parameter;
-   an optional ticketid number for acknowledgement. Unless the handler
    needs to perform asynchronous acknowledgement, which is a rather
    unusual use case, this parameter should be ignored.

> **WARNING**
>
> The `data tree` contains some default handlers for common tasks.
> Overwriting the tree with a fresh table will disable these pre-wired
> tasks.

The following example illustrates the registration of function on the
`data tree`:

~~~~{.lua}
-- create an asset
asset = racon.newAsset("myasset")

-- create a sub path 'engine' in the asset data tree
-- in developper studio datamodel, 'engine' is represented as a node
asset.tree.engine = {}

-- dummy function to set on the data tree
local function writebranch(tablevalues, remaingpath)
    print(remaingpath)
    for k,v in pairs(tablevalues) do
        print("set", k ,v)
    end
end

-- add function on path 'myasset.engine.temperature': called whenever a data writing is done on 'myasset.engine.temperature'
asset.tree.engine.temperature = writebranch

-- add default function on path 'myasset.engine': called whenever a data writing is done on 'myasset.engine'
asset.tree.engine.__default = writebranch

-- register the asset
asset:start()
~~~~

##### 4.3. Pre-wired Tasks

The `'commands'` sub-table is preset in an asset's `data tree` when it's
created. It contains the `'ReadNode'` command. The `'ReadNode'` command
is registered on the asset's `data tree` at 'commands.ReadNode'.

###### 4.3.1. Command Reception

TODO: Update this section

AWTDA3 deprecates AWTDA2's concept of commands. Instead, the protocol
transports commands as regular data writing. The concept of command
remains supported, through the convention that data writings under the
`"commands.``"` **path represent commands, and command callbacks should
be registered as handlers under** `"commands.``"`.

> **WARNING**
>
> A command is just a function registered in the asset `data tree` under
> the "special" path `@assetid.commands`.\
> The asset `data tree` comes with:
>
> -   Pre-wired commands (ex: `ReadNode`, function registered on
>    `@assetid.commands.ReadNode`)
> -   A default command (registered on `@assetid.commands.__default`)
>    returning an `"unknown command"` error.

The following example illustrates the registration of commands on the
asset `data tree`:

~~~~{.lua}
asset = portal.newasset("myasset")

-- This function will be called when the Command StartEngine is sent to the asset
asset.tree.commands.StartEngine = function(asset, tablevalues, remaingpath) end -- do something that starts the engine...

asset:start()
~~~~

#### 5. Setting and Getting Agent Internal Variables

The Agent has a list of internal variables that can be manipulated
by a user application. For instance, these variables allow to interact
with the Monitoring Engine, Agent configuration.

The provided `devicetree.get` and `devicetree.set` API allow to access
those Agent internal variables.\
 The Get API allows you to retrieve the value of a terminal variable as
well as to retrieve and discover branches of the tree representation of
variables.\
 The Set API allows you to set the value of a terminal variable, to set
a sub tree at once, and to create a new variable when it does not exist.

The Register API allows you to subscribe to change on individual
variables, on whole sub-trees, or on a mix thereof. It also handles
passive variables: normally, when a callback is triggered because some
registered variables changed, it only receives the variables whose
values changed as parameters. If some passive variables are listed
during the registration, their content is always passed to the callback,
whether it changed or not. A change in a passive variable won't trigger
the callback, though--unless it is also listed as an active one.

#### 6. Software Update

The Agent Connector library provides an easy software update
mechanism that is well coupled with the Platform Server. The final
update process (replace a binary file, update a text configuration file,
upgrade a device firmware, etc.) is really dependent on the application,
and thus this part is let at user discretion. There are exceptions to
the process. For example, for updating application managed by the Application
Container, everything is managed by the Agent.

The Platform Server and Agent are actually in charge of the
transfer and notification of new software update packages. The system
guarantees that the software package transfer is safe and different
options are available: checksums, authentication, and encryption.

In order to receive Software Update notifications, the user must
register a listener on the Agent Connector context. The
notification provides a `packageName`, a `packageVersion`, an `url`
that specifies where the file is located (on the local file system) and
some custom applicative `parameters` to help the update process.

#### 7. Sending and Receiving SMS

The library provides an API in order to register a listener for SMS
reception. In order to register for SMS reception the user has to give
two patterns: one on the emitter and one on the message content. If the
emitter or the message of a received SMS matches the corresponding
pattern, then this listener is called.

A simple send SMS API is provided in order to send SMS.\
 When sending an SMS, the user has to provide a format parameter that
fits the modem (and network) capabilities of the device.\
 Available options are 7bits, 8bits, and UCS2.

The Agent relays incoming and outgoing SMS and handles the
concatenation if the SMS content is larger than the single SMS capacity.
There is no restriction on the emitter/recipient of an SMS at the
Agent itself, though it must be connected to a modem with an
activated SIM card.
