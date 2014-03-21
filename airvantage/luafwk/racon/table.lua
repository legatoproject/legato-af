-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Racon objects handling staging database tables, to buffer, consolidate
-- and send structured data.
--
-- @{#table} instances should be created with @{racon.asset#(asset).newTable}.
--
--**NOTE:** The @{#table} API is currently in BETA, and is subject to change in
-- the subsequent release.
--
-- @module racon.table
--

local common = require 'racon.common'

local LEGAL_STORAGE  = { ram=1, file=1 }
local DEFAULT_STORAGE = "ram"

local MT_TABLE = { __type='racon.table' }; MT_TABLE.__index=MT_TABLE

--------------------------------------------------------------------------------
-- Column descriptors describes a table column with a name, a serialization
-- method and some serialization options.
--
-- Depending on serialization methods, some fields can be either unused,
-- optional or mandatory. Whenever a full column descriptor is expected, a
-- simple string can be passed: in this case, the default serialization method
-- and options will be used.
--
-- @type columnspec
--
-- @field #string name column name
--
-- @field serialization serialization method. Possible values are `fastest`,
--  `smallest`, `list`, `deltasvector`, `quasiperiodicvector`. Optional,
--  defaults to `list`.
--
-- @field factor this is the DeltasVector factor, i.e. the precision to use when
--  serializing floating point numbers. Set it to 1 to round to integer, 0.1 to
--  round all numbers to 1/10 (that is 12.345 will be sent as 12.3). This field
--  is mandatory for `deltasvector` serialization and optional for
--  `smallest` (otherwise GCD will be computed, and non-integer numbers will
--  cause fallback to vector).
--
-- @field period period for Quasi-periodic vector (number). Mandatory for
--  `quasiperiodicvector` serialization.
--
-- @field asfloat boolean to force encoding of doubles as float (default: false).
--
-- @field consolidation consolidation method, mandatory for
--  table:@{racon.table#(table).newConsolidation} calls. Possible values are `first`,
--  `last`, `max`, `mean`, `median`, `middle`, `min`, `sum`.
--
-- **Note:** QuasiPeriodic Vector is **not** part of AWT-DA 2 and should not be
-- used (it will not be choosen by `smallest` encoding).
--


--FIXME AWTDA3 QPV (update doc)

--------------------------------------------------------------------------------
-- Creates and returns a @{#table} instance.
--
-- @function [parent=#racon.asset] newTable
-- @param self
-- @param path (relative to the asset root) where the data will be sent.
-- @param columns list of either @{racon.table#columnspec} or column names (to
--  use default values).
-- @param storage either "file" or "ram", how the table must be persisted.
-- @param sendPolicy name of the policy controlling when the table content is
--  sent to the server.
-- @param purge boolean indicating if existing table (if any) is recreated
--  (`true`) or reused (`false`/`nil`). Recreated means the table will be
--  dropped and then created from scratch (so any data inside table will be
--  lost).
--
-- @return @{#table} to store and consolidate structured data.
-- @return #nil,#string error message.
--

local function newtable(asset, path, columns, storage, sendpolicy, purge)
    checks("racon.asset", "string", "table", "?string", "?string", "?")
    if not storage then
        storage = DEFAULT_STORAGE
    elseif LEGAL_STORAGE[storage] then
        -- pass
    elseif LEGAL_STORAGE[sendpolicy] then -- reversed args
        storage, sendpolicy = sendpolicy, storage
    else -- no storage, even misplaced
        sendpolicy, storage = sendpolicy or storage, DEFAULT_STORAGE
    end
    local r, table_id = common.sendcmd("TableNew", {
        asset   = asset.id,
        storage = storage,
        policy  = sendpolicy,
        path    = path,
        columns = columns,
        purge   = not not purge })
    if not r then return nil, table_id end
    return setmetatable({id=table_id}, MT_TABLE)
end

--- @type #table

--------------------------------------------------------------------------------
-- Adds a row of data to the table.
-- The table needs to be created with @{racon.asset}:newTable().
--
-- @function [parent=#table] pushRow
-- @param self
-- @param row table of key/value pairs,
--  value must be a @{#columnspec},
--  keys must match creation column names.
-- @return #string "ok" on success.
-- @return #nil,#string error message otherwise.

function MT_TABLE :pushRow(row)
    checks("racon.table", "table")
    return common.sendcmd("TableRow", { table=self.id, row=row })
end

--------------------------------------------------------------------------------
-- Sends the table content to the server and empties it unless dont_reset is true.
--
-- @function [parent=#table] send
-- @param self
-- @param #boolean dont_reset if true, the table content is not emptied after being sent.
-- @return "ok" on success.
-- @return #nil,#string error message otherwise.

function MT_TABLE :send(dont_reset)
    checks("racon.table", "?")
    return common.sendcmd("SendTrigger", { table=self.id, dont_reset=dont_reset })
end

--------------------------------------------------------------------------------
-- Sets the maximum number of rows for a given table. Once a table's number of
-- rows reaches its max, table content is either sent or consolidated, depending
-- on policies: `never` policies are not triggered. This means that if a table
-- is consolidated (that is, its send policy is `never`), data will be
-- consolidated and not sent. Table is emptyed after trigering policy.
--
-- @function [parent=#table] setMaxRows
-- @param self
-- @param n max number of rows accepted in the table.
-- @return "ok" on success.
-- @return nil followed by an error message otherwise.
--

function MT_TABLE :setMaxRows(n)
    checks("racon.table", "number")
    return common.sendcmd("TableSetMaxRows", { table=self.id, maxrows=n })
end

--------------------------------------------------------------------------------
-- Creates a table where data will be consolidated.
--
-- When a consolidation is declared, a new table (*destination*), whose column
-- names must be a subset of the *source* table column names, will be created.
-- Everytime the source table is consolidated, it will flush everything from
-- source and create in a single row in destination. The values in this row
-- depend on *consolidation method* (average value, minimum, median, ...) which
-- is set for each column.
--
-- A table can have only one consolidation table (trying to set a consolidation
-- twice will result in a error).
--
--     Example : 
--     --create the polices:
--     agent.config.data.policy.everyminute={period=60}  --consolidation policy every minute
--     agent.config.data.policy.every10minutes={period=10*60}  --sending data policy every 10 minutes
--     --create source and destination tables
--     local src = asset:newTable('example', {'timestamp', 'temp'}, 'ram', 'never')
--     local dst = src:newConsolidation('consolidated', 
--             { timestamp='median', temp='mean' }, 'everyminute', 'every10minutes')
--     -- fill data into src
--     src:pushRow{ timestamp=1, temp=25 }
--     src:pushRow{ timestamp=2, temp=28 }
--     src:pushRow{ timestamp=3, temp=25 }
--     
--     --Every minute, the source table will be consolidated and a row { timestamp=2, temp=26 } will be created in destination
--     --Every 10 minutes, the consolidated data destination will be sent to server
--
-- The data can be consolidated by:
--
--   * Calling @{#table.consolidate}
--   * Setting @{#table.setMaxRows} on source table
--   * Giving the consoPolicy parameter (see next paragraph)
--
-- **Note:** a consolidation policy can be set only if the send policy of the
-- source table is `never` (trying to call this method otherwise will result
-- in a error). This means that automatic policies are mutually  exclusive for
-- sending and consolidating data. However @{racon.table:send} and
-- @{racon.table:consolidate} methods are always available for more advanced
-- use cases.
--
-- **Note 2:** when the policies of the consolidation of a source and the
-- sending of its destination are the same, consolidation is guaranteed to be
-- done first. However, they are in different policies and if both are
-- triggered at the same time, the behavior is undefined.
--
-- **Note 3:** in order to be consolidated, data must be numeric, any other
-- kind of value will result in inserting `null` in destination table.
--
-- @function [parent=#table] newConsolidation
-- @param self
-- @param path (relative to the asset root) where the data will be sent.
-- @param columns either a list of @{racon.table.columnspec} with `consolidation`
--  field and the same names as source table which associate each column
--  name (key) to its consolidation method (value), like in example.
-- @param storage either "file" or "ram", how the table must be persisted.
-- @param consoPolicy name of the policy controlling when the *source* table
--  content is consolidated.
-- @param sendPolicy name of the policy controlling when the *destination* table
--  content is sent to the server.
-- @param purge boolean indicating if existing table (if any) is supposed to be
--  recreated (`true`) or reused (`false`/`nil`). If you want to use this
--  feature in consolidation, you have to recreate tables in the same order as
--  they have been created (that is start by recreate the main source and then
--  its consolidation, and so on). In particular you cannot recreate a
--  destination table as long as its source is still active.
--
-- @return #table where consolidated data will be stored.
-- @return nil followed by an error message otherwise.
--

function MT_TABLE :newConsolidation (path, columns, storage, consoPolicy, sendPolicy, purge)
    checks("racon.table", "string", "table", "?string", "?string", "?string", "?")

    local r, table_id = common.sendcmd("ConsoNew", {
        src          = self.id,
        path         = path,
        columns      = columns,
        storage      = storage or DEFAULT_STORAGE,
        send_policy  = sendPolicy,
        conso_policy = consoPolicy,
        purge        = not not purge })

    if not r then return nil, table_id end
    return setmetatable({id=table_id}, MT_TABLE)
end

--------------------------------------------------------------------------------
-- Consolidates the table content into its consolidation table, and empties it
-- unless `dont_reset` is true.
--
-- @function [parent=#table] consolidate
-- @param self
-- @param dont_reset if true, the table content is not emptied after being
--  consolidated.
-- @return "ok" on success.
-- @return nil followed by an error message otherwise.
--

function MT_TABLE :consolidate (dont_reset)
    checks("racon.table", "?")
    return common.sendcmd("ConsoTrigger", { table=self.id, dont_reset=dont_reset })
end

--------------------------------------------------------------------------------
-- Empties all content in the table.
--
-- @function [parent=#table] reset
-- @param self
-- @return "ok" on success.
-- @return nil followed by an error message otherwise.
--

function MT_TABLE :reset()
    checks("racon.table")
    return common.sendcmd("TableReset", { table=self.id })
end

require 'utils.loweralias' (MT_TABLE)

return newtable
