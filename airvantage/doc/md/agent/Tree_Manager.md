Tree Manager
============

#### Purpose

Offer a generic mechanism to present a device's data as a set of
variables, organized in a hierarchical tree, which can be read, written,
and monitored for changes by user applications.

The architecture consists of 3 parts:

-   A logical tree, whose hierarchical organization is user-friendly and
    portable, is presented to user applications.

-   Actual data manipulation is performed by device-specific handlers,
    which work on handler trees. The handler trees' layout is organized
    in a way which eases implementation, and doesn't have to match the
    logical tree's layout. Handler API is exposed to developers who want
    to contribute new variables to the tree manager, but not to normal
    applications.

-   a mapping definition, allowing the core tree engine to translate
    logical paths into handler paths and the other way around. This
    mapping is compiled into CDB databases, so that large mappings don't
    have to fit entirely in RAM.

#### Sub-modules

The tree manager is organized in several sub-modules:

-   `treemgr` is the core engine, which coordinates handlers with the
    logical view of the tree.

-   `treemgr.db` interfaces with the CDB databases which describe the
    mapping.

-   `treemgr.build` compiles `".map"`**files into a set of `"`**`.cdb"`
    database files, organized for quick access to translation info.

-   `treemgr.handlers.*` are handler implementations.

#### Conceptz

The API works with paths (sequences of identifiers separated by dots),
which denote nodes in trees. The node can be leaf nodes or non-leaf
nodes. The non-leaf nodes have children, but carry no data to read or
write. Leaf nodes carry some data to read/write/monitor, but have no
children nodes.

In the example below, `a` and `a.b` are non-leaf nodes, with children
{`"a.b", "a.d"`} and {`"a.b.c"`} respectively. Leaf nodes `"a.b.c"` and
`"a.d"` have no children, but carry a value each.

~~~~ {.bash}
-a
  +-b
  | +-c = 123
  +-d = 234
~~~~

There are two kinds of trees involved in the tree manager : the logical
tree, which is the one visible to users and servers, and the handler
trees. There are many handlers, with different implementations of the
get/set/notify services, and these handler trees can be mounted on the
logical tree to actually implement it. For instance, if a handler's root
is mounted on logical node "a.b", then the logical path "a.b.c.d" is
associated with the handler path "c.d". One can also mount non-root
nodes of a handler, for instance mount handler node "x.y" on logical
node "a.b". In that case, logical path "a.b.c.d" is mapped with handler
path "x.y.c.d". Leaf nodes can be mounted as well as non-leaf nodes. It
is the tree manager's job to maintain all mappings between the handler
trees and the logical tree: when implementing a handler, one never has
to ever think of logical paths.

Here is an example of mapping between a logical tree and two handlers:

Logical tree:

~~~~{.bash}
+system
| +-position
| | +-latitude
| | +-longitude
| | +-elevation
| +-time
+-config
       +-server
       +-agent
       +-modem
       +-...
~~~~

Handler trees:

~~~~{.bash}
aleos
  +-GPS_LATITUDE
  +-GPS_LONGITUDE
  +-GPS_ELEVATION
  +-TIME

config
  +-server
  +-agent
  +-modem
  +-...
~~~~

In the example above, we want to map:

-   `system.position.latitude` with `aleos:GPS_LATITUDE`;

-   `system.position.longitude` with `aleos:GPS_LONGITUDE`;

-   `system.position.elevation` with `aleos:GPS_ELEVATION`;

-   `system.time` with `aleos:TIME`.

-   `config` with `config:<root>`

Each aleos variable above is mapped individually, but the whole `config`
tree is mapped recursively: by mapping `config:<root>=config`, one gets
for instance `config:server.url=config.server.url`,
`config:mediation.pollingperiod.GPRS=config.mediation.pollingperiod.GPRS`,
etc.

#### Naming conventions

In this module, the following variable naming conventions are chosen:

-   `hpath` stands for a path relative to a handler, which might denote
    a leaf node as well as a non-leaf node

-   `hlpath` stands for a handler's leaf node.

-   `lpath` is an absolute path in the logical tree (leaf or non-leaf).

-   `llpath` is an absolute path to a leaf node in the logical tree.

-   `nlnpath` is a non-leaf logical path.

-   `hmap` is an `hpath->value` table.

-   `lmap` is an `lpath->value` table.

#### Handlers

Handlers work with paths relative to themselves; handler paths are not
shown directly to user applications, they need to be mapped into the
logical tree first.

The features needed from handlers are provided through methods; that is,
every Lua object providing the get/set/register/unregister methods below
is considered as a valid handler:

-   `handler:get(hpath)` allows to retrieve the value associated with a
    leaf-node path, or the set of children under a non-leaf node path.
    Get can return:
    -   a value, followed by nil
    -   nil, followed by a string error message
    -   nil, followed by a children table. The children must be in the
        table's keys, not in its values, e.g. {{ { x=true, y=true } }}
        is correct, but {{ { 'x', 'y' } }} is not. The path toward the
        children must be relative to the `hpath` argument.

-   `handler:set(hmap)` allows to write a map of `hlpath->value` pairs
    in the handler. It is not expected to return anything meaningful.
    Notice that a whole map is passed, which can contain several
    path/value pairs.

-   `handler:register(hpath)` signals that any change of a variable's
    value, or a value change of any variable under a non-leaf node, must
    be notified to the tree management system. Notification must be
    performed by the handler, by calling `treemgr.notify()` everytime it
    modifies a registered variable.

-   `handler:unregister(hpath)` signals that the logical tree doesn't
    need to be notified about changes to the `hpath` variable anymore.

-   `treemgr.notify(handler_name, hmap)` must be called by handlers to
    signal that a set of variables has changed. The engine will take
    care of converting hpaths into lpaths, retrieving the hooks to
    notify, sort out the variables (remove the irrelevant ones, add the
    missing associated ones) for each hook.

#### Logical tree

This is the API accessible to user applications. It offers get / set /
notification services, on variables organized according to a map which
is independent from the organization by handlers or within handlers.

Applications can read values with `get()`, write values with `set()`,
register hook functions to be triggered everytime a variable changes
with `register()`:

-   `treemgr.get(lpath)` returns a value or a list of children node
    names, depending on whether the path denotes a leaf node or a
    non-leaf node.

-   `treemgr.get(lpath_list)` performs a batch reading, returns an lmap
    of values and/or a list of children node lpaths.

-   `treemgr.set(llpath, value)` sets the value of a leaf node.

-   `treemgr.set(lpath, map)` where map keys `k_n` are strings such that
    `lpath .. "." .. k_n` are logical leaf paths sets a set of leaf node
    values in a single operation.

-   `treemgr.set(lmap)`, where map keys are logical leaf paths, sets a
    set of leaf node values in a single operation.

-   `treemgr.register(lpath_list, hook, associated_lpath_list)`
    registers a hook to be triggered everytime one of the variables
    denoted by lpath\_list changes. The hook receives an `llpath->value`
    map argument, which lists the union of every variable in
    `lpath_list` which changed, plus every vaiable in
    `associated_lpath_list`. If a variable must be monitored, and its
    value is needed by the hook even if it didn't change, then it needs
    to be listed both in `lpath_list` and in `associated_lpath_list`.

#### Mapping and handler loading

A treemgr configuration consists of handlers and a mapping. Handlers are
Lua objects which implement the `handler:get()`, `handler:set()`,
`handler:register()` and `handler:unregister()` methods. The mapping is
a set of bidirectional correspondances between logical tree nodes and
handler nodes.

The mapping is stored in a CDB (Constant DataBase): it ensures
conversions between the user view (logical paths) and the implementation
view (handler path) in constant memory and time. It is built from
`"*.map"` files, but once the DB is built, map files are not needed
anymore.

The handlers are loaded lazily, when they are needed to fulfill a user
request. They are identified by the name of the Lua module which
implements them: if a handler is called
`'agent.treemgr.handlers.ramstore'`, then a call to require
`'agent.treemgr.handlers.ramstore'` must return the handler object. By
sitting directly above Lua's module management system, the handler
loading system benefits from its flexibility and its various predefined
loaders.

To facilitate build and deployment, treemgr is able to recompile its CDB
from map files on target, if they are present and more recent than the
DB. By convention, all the map files in `persist/treemgr` are compiled
into the DB. Each map file describes the mappings of one handler.

A given treemgr configuration is described through a set of `"*.map"`
files: each map file lists one handler, and a list of mappings, between
nodes in this handler and nodes on the global, logical tree. The link
between the handler's map and its code is maintained through Lua's
`require` module system: the handler's name must be a valid Lua module
name, and the result of loading this module must be the handler object,
ready to run.

Map files are precompiled into CDB databases, for faster access in
constant memory. CDB results could be cached in RAM if necessary,
although it is not currently implemented.

Each architecture might have different ways to provide the same service,
and might not provide the exact same set of services as others,
depending e.g. on available hardware. By assembling the correct set of
specific handlers, together with the map files which put variables at a
standard place in the logical tree, one builds the target-specific
implementation of the portable treemgr interface.

Implementation
==============

One strong implementation constraint is that the logical tree can be
big, and must not be required to fit entirely in RAM. It is therefore
built as a read-only database, based on [cdb](http://cr.yp.to/cdb.html).

There are four dictionaries to be kept in the database:

-   `lpath -> handler_name:hpath` allows `get` and `set` to translate
    logical paths into handler paths, to which the actual read / write
    operations are delegated.

-   `handler_name:hpath -> lpath` allows `notify` to translate handler
    notifications into logical ones, which will be presented to the
    relevant apllicative hook. A given handler + path can be mounted in
    more than one place, and therefore have several values associated
    with it in the database.

-   `lpath -> direct_children_lpaths_list` is used by `get` to retrieve
    the children of non-leaf nodes. Parent and children lpaths are both
    absolute.

-   `lpath -> mounted_handlers_below_node` is used by `register`: when a
    hook is registered on a non-leaf node, it must register every
    handler mounted below it. This database lists these, indexed by
    ancestor nodes (this means that handlers mounted below the root node
    are indexed more than once).

#### get

The get operation on leaf node is straightforward: `llpath` is
translated to `handler_id, hlpath`, then the handler is retrieved and
its get method is called with the proper argument.

A get on a non-leaf node must return the list of every direct child of
the node. If the node is under a handler, then this handler's `get`
method is in charge of providing this list. Hence, the `get` of a
non-leaf node depends on the handlers mounted above it ("above" being
understood inclusively, i.e. a node is considered to be above itself.
For instance, if there's a non-leaf mountpoint on lpath `"a.b"`, then a
get request on `"a.b"` depends on this mountpoint, not on any mountpoint
on `"a"` nor on the root node).

However, it also depends on handlers mounted below it. For instance, if
there's a handler mounted on path `"a.b.c"`, and the application gets
the list of `"a"`'s children, then `"b"` must be included in this list,
whether there's also a handler mounted on `"a"` or not.

If a get operation covers several paths mapping several handlers, a
logical get can trigger several handler get operations. However, it
makes sure to perform at most one get operation per handler, thus giving
the handler an opportunity to optimize retrieval operations.

#### set

Set operations are mostly the same as leaf-node get requests. The
differing part (writing hpaths rather than reading them) is specific to
each handler.

#### register

Hook registrations call the `register` method(s) of the corresponding
handler(s), so that they will know they must provide notifications.
Those notifications are produced by calling `notify`, which will:

-   convert `hlpath` s into `llpaths`;

-   request and add the variables in `associated_lpath_list`;

-   call the appropriate hook with the resulting map.

The logical registration on a non-leaf node must translate into
registrations:

-   on the first mountpoint node above it;

-   on the root of every mountpoint below it.

#### unregister

The application can unregister from logical paths it's not interested in
anymore. Before unregistering from the corresponding `hpath` s, though,
one must first ensure that no other hook needs this `hpath`. This
requires to check whether the hpath is mapped to other `lpaths`, as it
can be mapped more than once, either directly or through an ancestor
node.


