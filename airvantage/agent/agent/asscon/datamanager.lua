-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Julien Desgats     for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Reception of EMP messages sent by the asset application, normally through the
-- asset library.
--------------------------------------------------------------------------------

require 'stagedb'
require 'persist'
local niltoken = require 'niltoken'
local asscon   = require 'agent.asscon'
local srvcon   = require 'agent.srvcon'
local upath    = require 'utils.path'
local utable   = require 'utils.table'
local m3da     = require 'm3da.bysant'
local errnum   = require 'returncodes'.tonumber

--------------------------------------------------------------------------------
-- A policy features:
-- * records, a support->records list table
-- * tables, an id->{path, sdb} table
-- * ack_ram, a table of unsent acknowledgements, indexed by ticketid
-- * ack_rom, an optional persisted array of unsent acknowledgements,
--   indexed by ticketid
--
-- Acknowledgements are stored as records with fields 'ticket', 'status' and
-- 'message'.
--------------------------------------------------------------------------------

local POLICIES      = { }
local TABLES        = { }
--local LOCKED_POLICIES = { }
--local LOCKED_TABLES = { }

local M = { }

local num2handle = {
    [30]='PData',
    [32]='PFlush',
    [33]='PAcknowledge',

    [40]='TableNew',
    [41]='TableRow',
    [43]='TableSetMaxRows',
}

--------------------------------------------------------------------------------
-- handle.xxx(asset_id, x) execute command xxx, sent by the user application,
-- on behalf of asset and with message content x.
--------------------------------------------------------------------------------
local handle = { }

--------------------------------------------------------------------------------
-- data records can be sent together in a single message if they have both
-- the same path and the same set of keys.
-- such records have the same support, as returned by this function.
--------------------------------------------------------------------------------
local function record_support (record)
    local columns, i, with_subrecords = { }, 1, false
    for k, v in pairs(record) do
        if type(k)~='string' then return nil, "bad record key" end
        if type(v)~='table' and not k :find ('.', 1, true) then
            columns[i], i = k, i+1
        else with_subrecords = true end
    end
    return columns, with_subrecords
end

--------------------------------------------------------------------------------
-- Register a record in the policy
--------------------------------------------------------------------------------
local function record_insert(P, columns, path, record)
    checks('table', 'table', 'string', 'table')
    local support = path..":"..table.concat(columns, '&')
    local sdb, msg = P.records[support]
    local has_niltokens = false

    -- stagedb doesn't handle niltokens: remove them if necessary, without
    -- modifying the existing `record` which belongs to the caller.
    -- This is done *after* the support has been computed, because we want
    -- the key corresponding with `nil` to show up in the support.
    -- TODO: it's a local function, it's probably OK to mess with `record` actually.
    for _, v in pairs(record) do if v==niltoken then has_niltokens=true; break end end
    if has_niltokens then -- remove niltokens
        local rec2 = { }
        for k, v in pairs(record) do rec2[k] = niltoken(v) end
        record = rec2
    end

    -- Create the table if necessary before pushing data in it.
    if not sdb then
        sdb, msg = stagedb('ram:'..support, columns)
        if not sdb then return nil, msg end
        P.records[support] = sdb
        local r
        r, msg = sdb :row (record)
        if r then sdb :trim() else return nil, msg end
    else return sdb :row (record) end
end

local function get_table_id(path, storage, columns)
    local names = { }
    for k, v in pairs(columns) do
        local name
        if type(k)=='string' then name=k
        elseif type(v)=='string' then name=v
        elseif type(v)=='table' then name=v.name end
        assert(name, "Invalid column")
        table.insert(names, name)
    end
    table.sort(names)
    return storage .. ":" .. path .. '-' .. table.concat(names, '-') .. '.tbl'
end

--------------------------------------------------------------------------------
-- Check whether the proposed column set is compatible with an existing table.
-- Currently, only column count and name is tested, not serialization settings.
-- @param tbl existing table to check
-- @param columns columnspec list to check against `tbl`
-- @return true if `tbl` has the same columns as `columns`
--------------------------------------------------------------------------------
local function match_columns(tbl, columns)
    local tblcols = tbl.sdb:state().columns
    local newcols = { } -- column names list for the new table

    for key, col in pairs(columns) do
        -- column name is key for consolidation, value for short columnspec or name field for full columnspec
        table.insert(newcols, type(key) == 'string' and key or (type(col) == 'string' and col or col.name))
    end

    if #newcols ~= #tblcols then return false end
    -- both table has same length, compare names
    table.sort(newcols)
    table.sort(tblcols)
    for i=1, #columns do
        if newcols[i] ~= tblcols[i] then return false end
    end
    return true
end

--------------------------------------------------------------------------------
-- Create a new table, which can be filled with command tablerow, and will
-- be flushed
-- x fields: table, policy, path, columns
--------------------------------------------------------------------------------
function handle.TableNew(asset, x)
    asset=x.asset--Need to work with assetname
    local path = upath.concat(asset, x.path)
    local table_id = get_table_id (path, x.storage, x.columns)
    local P = POLICIES[x.policy or 'default']
    if not P then return errnum 'BAD_PARAMETER', "no such policy" end

    -- check whether the table is created or reused from existing
    local t = TABLES[table_id]
    if t then
        -- table already exists, if it is a destination table, it is incorrect
        -- to retrieve it from a this method
        if t.src_table then return errnum 'BAD_PARAMETER', "cannot create or retrieve "..table_id.." because it is a destination table." end
        if x.purge then
            local ok, msg = M.close_table(nil, table_id)
            if not ok then return errnum 'BAD_PARAMETER', msg end
            t = nil
        elseif not match_columns(t, x.columns) then
            return errnum 'BAD_PARAMETER', "cannot reuse "..table_id..": columns does not match"
        end
    end

    -- create table if not exists or purged
    if not t then
        log('DATAMGR', 'DEBUG', "Create new table %s", table_id)
        local sdb, errmsg = stagedb(table_id, x.columns)
        if not sdb then return errnum 'UNSPECIFIED_ERROR', errmsg end
        t = { id=table_id, path=path, sdb=sdb }
    else log('DATAMGR', 'DEBUG', "Retrieving existing table %s", table_id) end

    t.send_policy=P
    P.tables[table_id] = t
    TABLES[table_id] = t
    return 0, table_id
end

--------------------------------------------------------------------------------
-- Register a data block, possibly nested, in sdb table(s) in P.records.
-- x fields: policy(string), ticket(int), status(int), message(string),
--           persisted(boolean)
--------------------------------------------------------------------------------
function handle.PAcknowledge(asset, x)
    local msg
    local policy = x.policy or 'now'
    local P = POLICIES[policy]
    if not P then return errnum 'BAD_PARAMETER', "no such policy" end
    log('DATAMGR', 'DEBUG', "Storing ACK for ticket #%d under policy %q",
        x.ticket, policy)
    local item = { ticket=x.ticket, status=x.status, message=x.message }
    local store = x.persisted and P.ack_rom or P.ack_ram
    if store[x.ticket] then
        log('DATAMGR', 'ERROR', "duplicate ticket number to acknowledge %d", x.ticket)
    end
    store[x.ticket] = item
    local f = P.latency_trigger
    if f then f() end
    return 0
end

--------------------------------------------------------------------------------
-- Register a data block, possibly nested, in sdb table(s) in P.records.
-- x fields: path, data, ?policy
--------------------------------------------------------------------------------
function handle.PData(asset, x)
    asset=x.asset--Need to work with assetname
    local P = POLICIES[x.policy or 'default']
    if not P then return errnum 'BAD_PARAMETER', "no such policy" end
    --local lb = LOCKED_POLICIES[x.Queue]
    --if lb then lb :add(x, payload); return true end

    -- Extract flat records, register them in the appropriate table.
    local function put_in_records(path, record)
        local columns, with_subrecords = record_support (record)
        if not columns then return errnum 'BAD_PARAMETER', with_subrecords end -- support error
        if not with_subrecords then
            record_insert(P, columns, path, record)
        else -- separate top-level record from sub-records
            local top_record = { }
            for k, v in pairs(record) do
                if type(v)=='table' then
                    local r, err = put_in_records(path..'.'..k, v)
                    if not r then return errnum 'BAD_PARAMETER', err end
                elseif string.find(k, '.', 1, true) then -- multi-segment key
                    local suffix, k2 = upath.split(k, -1)
                    put_in_records(path..'.'..suffix, { [k2] = v})
                else top_record[k] = v end
            end
            if next(top_record) then record_insert(P, columns, path, record) end
        end
        return 0
    end
    local record, r, msg = x.data
    if type(record)=='table' then
        r, msg = put_in_records(upath.concat(asset, x.path), record)
    else -- extract last path segment, use it as a single key for the non-table value
        if x.path == '' then return errnum 'BAD_PARAMETER', "no path and no key" end
        local path, key = upath.split(x.path, -1)
        r, msg = put_in_records(upath.concat(asset, path), {[key]=record})
    end
    local f = P.latency_trigger
    if f then f() end
    return r, msg
end

--------------------------------------------------------------------------------
-- Check that source/destination dependency is handled for consolidation.
-- If guiven table is a destination of a consolidation, checks if consolidation
-- is scheduled to be done at the same moment than that function is called (the
-- 'same moment' is defined with the resolution of the scheduler).
-- If this is the case, the function will block until the consolidation is done.
-- If table is not a destination, does nothing.
--------------------------------------------------------------------------------
local function check_source_consolidated(tbl)
    local src = tbl.src_table
    if not src then return end
    -- check that source is to be consolidated right now (and has not be consolidated yet)
    -- the test and the wait MUST be atomic, any yield between them could cause a miss for
    -- the signal (in particular, this is the reason why there is no log)
    if src.conso_policy.nextevent and src.conso_policy.nextevent <= os.time() and
       (src.lastconso == nil or src.lastconso < src.conso_policy.nextevent) then
        sched.wait(src, 'consolidated')
    end
end

--------------------------------------------------------------------------------
-- Send an sdb to the server through srvcon, unless it is already being sent.
--------------------------------------------------------------------------------
local sdb_sendings_in_progress = { }

local function sdb2srv(path, sdb, policy_to_kill, key_to_kill, dont_reset)
    if sdb_sendings_in_progress[sdb] then return nil, "already sending" end
    sdb_sendings_in_progress[sdb] = true
    local function src_factory()
        local s = sdb :state()
        log('DATAMGR', 'DETAIL', "sending table %s (%d rows of %d columns) to server",
            s.id, s.nrows, #s.columns)
        assert (sdb :serialize_cancel())

        local prolog_sent, epilog_sent = false, false
        local bss = m3da.serializer{ }

        local function prolog_src()
            if prolog_sent then return nil else prolog_sent = true end
            local buff = { }
            assert(bss :setwriter(buff))
            assert(bss :object 'Message')
            assert(bss (path))
            assert(bss (0))
            return table.concat(buff)
        end

        local payload_src = sdb :serialize (bss)

        local function epilog_src()
            if epilog_sent then return nil else epilog_sent = true end
            local buff = { }
            bss :setwriter(buff)
            bss :close()
            return table.concat(buff)
        end

        local src = ltn12.source.cat(prolog_src, payload_src, epilog_src)

        return src
    end

    local function result_callback(status, errmsg)
        if not status then -- sending error
            sdb :serialize_cancel() -- just to be sure
            log('DATAMGR', 'ERROR', "Server emission error: %s", errmsg)
        elseif policy_to_kill then -- destroy the (undeclared) table altogether
            sdb :close() -- no need to wait for GC for resource release
            policy_to_kill[key_to_kill] = nil
        elseif not dont_reset then -- empty the table, but leave it there
            sdb :reset()
        end
        sdb_sendings_in_progress[sdb] = nil
    end
    if sdb:state().nrows>0 then -- Don't send empty tables
        srvcon.pushtoserver(src_factory, result_callback)
        return true
    else
        sdb_sendings_in_progress[sdb] = nil
        return nil, "no need to connect"
    end
end

-- simpler wrapper for sdb2srv to be consistent with consolidate for predeclared tables.
local function tbl2srv(t, dont_reset)
    return sdb2srv(t.path, t.sdb, nil, nil, dont_reset)
end

--------------------------------------------------------------------------------
-- Consolidate data in table.
--------------------------------------------------------------------------------
local flush_table -- function defined below, for recusive calls
local function consolidate(src, dont_reset)
    local c = false
    local dst = src.conso_table

    log('DATAMGR', 'DETAIL', "consolidating table %s into %s", src.id, dst.id)
    local ok, msg = src.sdb :consolidate()
    if not ok and msg ~= 'EMPTY' then
        log('DATAMGR', 'ERROR', 'consolidation error: %s', msg)
        return
    end

    if msg ~= 'EMPTY' then
        -- only if a row was actually inserted
        local maxrows = dst.maxrows
        if maxrows then
            local nrows = dst.sdb :state() .nrows
            if nrows >= maxrows then
                log('DATAMGR', 'DETAIL', "Flushing consolidation table for %s (full with %d rows)", src.id, nrows)
                flush_table(dst)
            end
        end

        local f = dst.send_policy.latency_trigger
        if f then f() end

        if not dont_reset then
            src.sdb :reset()
        end
    end

    src.lastconso = os.time()
    sched.signal(src, 'consolidated')
    return c
end

--------------------------------------------------------------------------------
-- Either send or consolidate a full table. This function is local,
-- it has been declared as such above to resolve mutual recurrence.
--------------------------------------------------------------------------------
function flush_table(t, dont_reset)
    local c
    if t.conso_policy and t.conso_policy ~= POLICIES.never then
        c = consolidate(t, dont_reset)
    elseif t.send_policy ~= POLICIES.never then
        c = tbl2srv(t, dont_reset)
    elseif not dont_reset then
        -- if both policies are 'never', empty table any way
        t.sdb :reset()
    end
    if c then srvcon.connect() end
    return true
end

local function send_ack(x, store)
    local ticket   = x.ticket
    local status   = tonumber(x.status) or (x.status and 0 or -1)
    local message  = x.message or (status==0 and "ok" or "unknown")
    local function src_factory()
        local serialize, acc = m3da.serializer{ }
        assert(serialize {
            __class = 'Response',
            ticketid = ticket,
            status = status,
            data = message })
        local serialized = table.concat(acc)
        return ltn12.source.string(serialized)
    end
    log('DATAMGR', 'DETAIL', "Send ACK for ticket #%d to SRVCON", ticket)
    local function remove_from_store(status, errmsg)
        if status then store[ticket] = nil
        else log('DATAMGR', 'ERROR', "Failed to send ACK #%d: %q", ticket, errmsg) end
    end
    srvcon.pushtoserver(src_factory, remove_from_store)
    return true -- true: there's always something pushed, connect() must happen
end

-- Send a single policy; return true if there's a need to connect to the server.
local function send_policy(P)
    log('DATAMGR', 'INFO', "flushing policy %q", P.name)

    local c = false -- whether a connection has been requested

    -- 1/ flush undeclared data record tables
    for support, sdb in pairs(P.records) do
        local path = support :match("^[^:]+")
        c = sdb2srv(path, sdb, P.records, support) or c
    end

    -- 2/ consolidate tables
    for table_id, t in pairs(P.consolidations) do
        check_source_consolidated(t)
        c = consolidate(t) or c
    end

    -- 3/ flush predeclared tables
    for table_id, t in pairs(P.tables) do
        check_source_consolidated(t)
        c = tbl2srv(t) or c
    end

    -- 4/ flush persisted and RAM acknowledgements
    c = c or next(P.ack_ram)
    for _, item in pairs(P.ack_ram) do
        send_ack(item, P.ack_ram)
    end
    if P.ack_rom then
        for _, item in pairs(P.ack_rom) do
            c = true
            send_ack(item, P.ack_rom)
        end
    end

    -- 5/ reset periodic flush policy, if applicable
    local t = P.periodic_timer
    if t then timer.rearm(t) end

    -- 6/ record next event date for the policy
    t = t or P.cron_timer
    P.nextevent = t and t.nd or nil

    if c then return true else return nil, "no need to connect" end
end

--- Perform a `send_policy` on a single policy, then forces a connection to
--  the server if appropriate.
local function send_and_connect(P)
    if send_policy(P) then srvcon.connect() end
end

--------------------------------------------------------------------------------
-- Insert a row in an explicitly created table.
-- x fields: table, row
--------------------------------------------------------------------------------
function handle.TableRow (asset, x)
    local t = TABLES[x.table]
    if not t then return nil, "no such table" end
    local r, msg = t.sdb :row (x.row)
    local maxrows = t.maxrows
    if maxrows then
        local nrows = t.sdb :state() .nrows
        if nrows >= maxrows then
            log('DATAMGR', 'DETAIL', "Flushing table %s (full with %d rows)", x.table, nrows)
            flush_table(t)
        end
    end
    local f = t.send_policy.latency_trigger
    if f then f() end
    return r and 0 or 1, msg
end

--------------------------------------------------------------------------------
-- x fields: policy, which might be set to '*' to flush all policies
--------------------------------------------------------------------------------
function handle.PFlush(asset, x)
    local c = false -- c is true if there's a need to connect (some data to send)
    if x.policy=='*' then
        for name, P in pairs(POLICIES) do
            if name ~= 'never' then c = send_policy(P) or c end
        end
    elseif x.policy == 'never' then
        return errnum 'BAD_PARAMETER', "Don't flush the 'never' policy"
    else
        local P = POLICIES[x.policy or 'default']
        if not P then return errnum 'BAD_PARAMETER', "no such policy" end
        c = send_policy(P)
    end
    if c then return 0, srvcon.connect() end
    return 0
end

function handle.ConsoNew(asset, x)
    local src_table_id = x.src
    local src = TABLES[src_table_id]
    if not src then return errnum 'BAD_PARAMETER', "no such source table" end
    local assetname = upath.split(src.path, 1)

    local dst_path = upath.concat(assetname, x.path)
    local dst_table_id = get_table_id (dst_path, x.storage, x.columns)
    local dst = TABLES[dst_table_id]

    local SP = POLICIES[x.send_policy or 'default']
    local CP = POLICIES[x.conso_policy or 'default']
    if not SP then return errnum 'BAD_PARAMETER', "no such send policy" end
    if not CP then return errnum 'BAD_PARAMETER', "no such consolidation policy" end
    if src.send_policy ~= POLICIES.never and CP ~= POLICIES.never then
        return errnum 'BAD_PARAMETER', "either send or consolidation policy for source must be 'never'"
    end

    if dst then
        -- if consolidation table already exists, then it is returned as is and cannot be purged
        if src.conso_table == dst then
            if x.purge then return nil, "cannot purge consolidation table while source is still open" end
            if not match_columns(dst, x.columns) then return nil, "cannot reuse "..table_id..": columns does not match" end
            log('DATAMGR', 'DEBUG', "Retrieving existing conso table %s", dst_table_id)
        else
            if src.conso_table then return nil, src_table_id.." already has a consolidation table" end
            if not x.purge then return nil, "table already exists" end
            local ok, msg = M.close_table(nil, dst_table_id)
            if not ok then return errnum 'UNSPECIFIED_ERROR', msg end
            dst = nil
        end
    end

    if not dst then
        log('DATAMGR', 'DEBUG', "Creating consolidation table %s for %s", dst_table_id, src_table_id)
        local dst_sdb, errmsg = src.sdb :newconsolidation(dst_table_id, x.columns)
        if not dst_sdb then return errnum 'UNSPECIFIED_ERROR', errmsg end
        dst = { id=dst_table_id, path=dst_path, sdb=dst_sdb, src_table=src }
    end

    dst.send_policy=SP
    SP.tables[dst_table_id] = dst
    src.conso_table = dst
    src.conso_policy = CP
    CP.consolidations[src_table_id] = src
    TABLES[dst_table_id] = dst
    return 0, dst_table_id
end

function handle.ConsoTrigger(asset, x)
    local t = TABLES[x.table]
    if not t then return errnum 'BAD_PARAMETER', "no such table" end
    if consolidate(t, x.dont_reset) then return 0, srvcon.connect() end
    return 0
end

function handle.SendTrigger(asset, x)
    local t = TABLES[x.table]
    if not t then return errnum 'BAD_PARAMETER', "no such table" end
    if tbl2srv(t, x.dont_reset) then  return 0, srvcon.connect() end
    return 0
end

function handle.TableReset(asset, x)
    local t = TABLES[x.table]
    if not t then return errnum 'BAD_PARAMETER', "no such table" end
    t.sdb:reset()
    return 0
end

function handle.TableSetMaxRows(asset, x)
    local t = TABLES[x.table]
    if not t then return errnum 'BAD_PARAMETER', "no such table" end
    t.maxrows = x.maxrows
    return 0
end

local function start_periodic_timer(P, period)
    assert(period>0)
    P.periodic_timer = timer.new(-period, send_and_connect, P)
    P.nextevent = P.periodic_timer.nd
end

local function start_cron_timer(P, cron)
    local t = timer.new(cron, send_and_connect, P)
    if not t then log('DATAMGR',  'ERROR', "Invalid cron config %q", cron)
    else
        P.cron_timer = t
        P.nextevent = t.nd
    end
end

local function setup_latency_callback(P, l)
    local function delayed_flush()
        log('DATAMGR', 'DETAIL', "Policy %q setup to triggered after latency", P.name)
        send_and_connect(P)
    end
    P.latency_trigger = function()
        timer.latencyexec(delayed_flush, l)
    end
end

function M.new_policy(pname, cfg)
    local P = {
        name           = pname;
        records        = { };
        tables         = { };
        consolidations = { };
        ack_ram        = { };
        ack_rom        = false }

    if POLICIES[pname] then return nil, "policy name conflict" end

    for k, v in pairs(cfg or { }) do
        if type(k)=='number' or k=="manually" then
            --    disregard
        elseif k=='onboot' then
            local delay = tonumber(v)
            if delay and delay>0 then
                timer.new(delay, send_and_connect, P)
            else log('DATAMGR', 'ERROR', "onboot policy config needs a positive value in seconds") end
        elseif k=='period' then
            local period = tonumber(v)
            if period and period>0 then start_periodic_timer(P, period)
            else log('DATAMGR', 'ERROR', "period policy config needs a positive value in seconds") end
        elseif k=='cron' then start_cron_timer(P, v)
        elseif k=='latency' then
            local l = tonumber(v)
            if not l then log('DATAMGR', 'ERROR', "latency policy config needs a positive value in seconds")
            else setup_latency_callback(P, l) end
        else log('DATAMGR', 'ERROR', "Unknown policy config name %s", tostring(k)) end
    end

    local ack_rom = persist.table.new('ack_policy_'..pname)
    if not ack_rom then return nil, "Cannot init ack rom storage: "..msg end
    P.ack_rom = ack_rom
    POLICIES[pname] = P
    return P
end

function M.close_table(asset_name, table_id)
    local t = TABLES[table_id]
    if not t then return nil, "no such table" else
        if t.src_table then return nil, "cannot close a destination table before its source." end
        for name, P in pairs(POLICIES) do
            P.tables[table_id] = nil
            P.consolidations[table_id] = nil
        end
        TABLES[table_id] = nil
        -- if the table to close is a source of a consolidation, then break the link
        if t.conso_table then t.conso_table.src_table = nil end
        return t.sdb :close()
    end
end


function M.init(cfg)
    -- Check policy setup consistency:
    -- * there must be a default policy, whose content is free
    -- * there must be a 'now' policy, it must be triggered with a latency
    --   and with an 'onboot' criterion
    -- * there must be a 'never' policy; it must be empty.

    for pname, pcfg in pairs(cfg.policy or { }) do M.new_policy(pname, pcfg) end

    for _, name in pairs{ 'default', 'now', 'never', 'on_boot' } do
        if not POLICIES[name] then
            log('DATAMGR', 'ERROR',
            "Missing mandatory data policy in agent config: data.policy.%s",
            name)
            return nil, "No "..name.." policy"
        end
    end

    if not cfg.policy.now.latency or not cfg.policy.now.onboot then
        log('DATAMGR', 'ERROR',
        "Missing mandatory latency or onboot param in data.policy.now policy")
        return nil, "Invalid now policy"
    end

    if not cfg.policy.on_boot.onboot  then
        log('DATAMGR', 'ERROR',
        "Missing mandatory  onboot param in data.policy.on_boot policy")
        return nil, "Invalid on_boot policy"
    end

    for k, v in pairs(cfg.policy.never) do
        if type(k)=='string' and k~='manually' then
            log('DATAMGR', 'ERROR',
            "Triggering config %q forbidden in \"never\" policy.", k)
            return nil, "Invalid never policy"
        end
    end

    -- Each function in the `handle` table must be attached, under its name,
    -- as an EMP handler.
    for name, f in pairs(handle) do
        asscon.registercmd(name, f)
    end
    return true
end

-- for debug:
M.policies = POLICIES
M.tables = TABLES

return M