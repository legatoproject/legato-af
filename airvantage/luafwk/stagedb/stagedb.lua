-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sdb_core = require 'stagedb.core'
local m3da = require 'm3da.bysant'

require 'checks'
require 'ltn12'

--function printf(...) return print(string.format(...)) end

local SDB_TABLE = { __type='stagedb.table' }; SDB_TABLE.__index = SDB_TABLE

-- Create and return an ltn12 source serving the table's serialized content.
-- The source serves chunks whose size is determined by self.BLOCKSIZE.
-- If not defined, it defaults to ltn12.BLOCKSIZE.
-- While serializing, the table is locked and doesn't support other operations,
-- including data reading.
function SDB_TABLE :serialize(bss)
    checks('stagedb.table', '?bysant.serializer')

    if self.sdb :state() .state ~= 'reading' then return nil, "bad state" end
    local n, i, buffer
    local first_time, done = true, false

    -- Write data to the output buffer until:
    --  - either the whole table has been serialized
    --  - or the buffer contains ltn12.BLOCKSIZE bytes of unsent data
    local function writer(data)
        local ndata = #data
        if ndata<=n then
            n=n-ndata; i=i+1; buffer[i]=data
            return ndata
        elseif n==0 then
            return 0
        else
            local prev_n=n; buffer[i+1] = data :sub( 1, n); n=0
            return prev_n
        end
    end


    -- Put data in the output buffer through a call to `:serialize()`,
    -- then return the result.
    local function source()
        if first_time then
            if bss then bss :setwriter(writer)
            else bss = m3da.serializer(writer) end
            first_time = false
        end
        if done then return nil, nil end
        n, i, buffer = ltn12.BLOCKSIZE, 0, { }
        local error
        done, error = self.sdb :serialize (bss)
        if done then -- last chunk
            sched.signal(self, 'serialized')
            return table.concat(buffer)
        elseif error=='again' then -- partial response
            return table.concat(buffer)
        else -- serializer error
            return nil, error
        end
    end
    return source
end

-- Automatically reset the table after its serialization has been completed.
function SDB_TABLE :serializeandreset()
    local r, msg = self :serialize()
    if r then
        sched.sigonce(self, 'serialized', function() self :reset() end)
        return r
    else return nil, msg end
end

-- Create a consolidation table
function SDB_TABLE :newconsolidation(idsm, columns)
    checks('stagedb.table', 'string', 'table')
    local sm, id = idsm :match '^(.-):(.+)$'
    if not sm then error "Invalid identifier" end
    local sdb, msg = sdb_core.newconsolidation(self.sdb, id, sm, columns)
    if not sdb then return nil, msg end
    return setmetatable({sdb=sdb}, SDB_TABLE)
end

for _, name in pairs{ 'state', 'close', 'reset', 'consolidate', 'row', 'trim',
    'serialize_cancel' } do
    local f = sdb_core[name]
    SDB_TABLE[name] = function(self, ...)
        local sdb = self.sdb
        local r, msg = f(sdb, ...)
        if r==sdb then return self else return r, msg end
    end
end

--------------------------------------------------------------------------------
--- Create a StageDB table.
--
-- The table is composed of columns and is filled row-by-row. Once filled, a table
-- can be either serialized (see `serialize` method) on consolidated into another
-- table (see `newconsolidation` and `consolidate`) on a column-by-column basis.
--
-- The serialization method for each column is defined at table creation by the
-- column specifications. Two forms are supported :
--   * Simple form: just a string to name the column, all other parameter are
--     default ones
--   * Full form: a @{column_spec} table that fully describes the column.
--
-- Example:
--     local db = stagedb("ram:dbname", { "col1", { name="col2", serialization="smallest" } })
--
-- @param idsm (string) String in format "<storage method>:<identifier>".
-- @param columns (table) sequence of @{racon.table.columnspec}.
--------------------------------------------------------------------------------
function stagedb(idsm, columns)
    checks('string', 'table')
    local sm, id = idsm :match '^(.-):(.+)$'
    if not sm then error "Invalid identifier" end
    local sdb, msg = sdb_core.init(id, sm, columns);
    if not sdb then return nil, msg end
    return setmetatable({sdb=sdb}, SDB_TABLE)
end

return stagedb