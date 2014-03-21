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

require 'm3da.bysant.core'

local core = m3da.bysant.core


local checks = require 'checks'
local isarray = require 'utils.table'.isArray

local M = { }
M.core = core
M.classes = {
    { id=0, name='Envelope',
        { name='header',  context='listmap' },
        { name='payload', context='unsignedstring' },
        { name='footer',  context='listmap' },
    },
    { id=1, name='Message',
        { name='path',     context='unsignedstring' },
        { name='ticketid', context='unsignedstring' },
        { name='body',     context='listmap' },
    },
    { id=2, name='Response',
        { name='ticketid', context='unsignedstring' },
        { name='status',   context='number' },
        { name='data',     context='unsignedstring' },
    },
    { id=3, name='DeltasVector',
        { name='factor', context='number' },
        { name='start',  context='number' },
        { name='deltas', context='listmap' },
    },
    { id=4, name='QuasiPeriodicVector',
        { name='period', context='number' },
        { name='start',  context='number' },
        { name='shifts', context='listmap' },
    },
}

local function add_classes (self, method)
    for _, classdef in ipairs(M.classes) do
        local r, errmsg = self[method] (self, classdef)
        if not r then return nil, errmsg end
    end
    return 'ok'
end

--- Creates a new M3DA serializer.
-- It differs from a plain Bysant serializer in that a series of standard
-- M3DA classes are predefined before it's used.
--
-- @param writer either a table or a function, being fed serialized data
--  stream. If `writer` is a table, data bits are insterted at the end of
--  the table. If it is a function, it must return the number of bytes it
--  successfully read.
-- @return the serializer object and the `writer`.
--
-- @usage
--
-- m3da = require 'm3da.bysant'
-- serializer1, acc = m3da.serializer{ }
-- serializer1 :number(1234)
-- printf("the serialized representation of 1234 is %q", table.concat(acc))
-- serializer2 = m3da.serializer(function(x) printf("Received %q", x"); return #x end)
-- serializer2 :map() :string'x' :number(1) :string'y' :number(2) :close()
--
function M.serializer(writer)
    checks('table|function')
    local function null_writer(str) return #str end

    local self, errmsg = core.init(null_writer)
    if not self then return nil, errmsg end

    local r, errmsg = add_classes (self, 'class')
    if not r then return nil, errmsg end

    local r, errmsg = self :setwriter (writer)
    if not r then return nil, errmsg end

    return self, writer
end

--- Creates a new M3DA deserializer object. M3DA standard classes are predefined
--  in the deserializer.
--
-- It deserializes data, passed as a string or list of strings parameter, through
-- method `:deserialize(data)`.
--
-- @usage
--
-- m3da = require 'm3da.bysant'
-- d = m3da.deserializer()
-- assert(d.deserialize("\228\145") == 1234)
-- assert(d.deserialize({"\228", "\145"}) == 1234)
--
function M.deserializer()
    local instance, errmsg = core.deserializer()
    if not instance then return nil, errmsg end

    local r, errmsg = add_classes (instance, 'addClass')
    if not r then return nil, errmsg end

    return instance
end

local function serialize_value(self, x)
    if(x==m3da.niltoken) then return core["null"](self) end
    local t = type(x)
    local f = core[t]
    if f then return f(self, x)
    else return nil, "cannot serialize a "..t end
end

local function serialize_object(self, x)
    local class = core.dumpclass(self, x.__class)
    if not class then return nil, "class "..classname.." not found" end
    local status, err_msg
    status, err_msg = core.object(self, class.id)
    if not status then return status, err_msg end
    for i, fieldname in ipairs(class) do
        status, err_msg = serialize_value(self, x[fieldname])
        if not status then return nil, err_msg end
    end
    status, err_msg = core.close(self)
    if not status then return status, err_msg end
    return self
end

local function serialize_array(self, x)
    local status, err_msg
    status, err_msg = core.list(self, #x)
    if not status then return status, err_msg end
    for _, v in ipairs(x) do
        status, err_msg = serialize_value(self, v)
        if not status then return status, err_msg end
    end
    status, err_msg = core.close(self)
    if not status then return status, err_msg end
    return self
end

local function serialize_map(self, x)
    local status, err_msg
    status, err_msg = core.map(self) -- TODO: consider opportunity of precomputing size
    if not status then return status, err_msg end
    for k, v in pairs(x) do
        status, err_msg = serialize_value(self, k)
        if not status then return status, err_msg end
        status, err_msg = serialize_value(self, v)
        if not status then return status, err_msg end
    end
    status, err_msg = core.close(self)
    if not status then return status, err_msg end
    return self
end

local function serialize_table(self, x)
    local classname = x.__class
    if x.__class then return serialize_object(self, x)
    elseif isarray(x) then return serialize_array(self, x)
    else return serialize_map(self, x) end
end

--- Creates an envelope ltn12 filter, which will encapsulate an ltn12 source
--  content into an M3DA envelope.
--
-- The `header` and `footer` fields of the envelope are passed as parameters,
-- but the `payload` must be streamed later as input to the resulting ltn12
-- filter.
--
-- This design allows to stream large data which wouldn't fit in RAM. If this
-- is not useful, one can do as follows:
--
--     local m3da = require 'm3da.bysant'
--     local payload, header, footer = XXX
--     local s1, t1 = m3da.serializer{ }
--     assert(s1(payload))
--     local serialized_payload = table.concat(t1)
--     local s2, t2 = m3da.serializer{ }
--     assert(s2{
--         __class = 'Envelope', header = header, footer = footer,
--         payload = serialized_payload })
--     local serialized_envelope = table.concat(t2)
--
-- Cutting the payload string in optimal chunks requires to keep fine details of
-- the Bysant encoding in mind. Specifically, in a string context such as defined
-- for the `payload` field of the `Envelope` class:
--
-- * Strings of size <= 67631 should be encoded as a single string;
--
-- * Strings bigger than that should be encoded in chunks of up to 65525 bytes,
--   terminated by an empty chunk 0x0, `"0x0"`.
--
-- @param header: the envelope's header, either `nil`, a map, or a function
-- returning `nil` or a map. If passed as a function, it is avaluated as late
-- as possible.
--
-- @param footer: the envelope's footer, either `nil`, a map, or a function
-- returning `nil` or a map. If passed as a function, it is avaluated as late
-- as possible. Among others, there's a guarantee that it will only be evaluated
-- after all of the payload


function M.envelope(header, footer)
    checks ('?table|function', '?table|function')

    local CHUNK_SIZE, SINGLE_MAX_SIZE = 65535, 67631
    local header_sent, payload_sent, footer_sent, several_chunks =
        false, false, false, false

    local result, buffer, buf_size = nil, { }, 0

    local function writer(x) table.insert (result, x); return #x end

    local serializer = M.serializer(writer)


    local function stream_header()
        log('BYSANT-M3DA', 'DEBUG', "Streaming header")
        if type(header) == 'function' then header = header() end
        serializer :object 'Envelope' (header)
        header_sent = true
    end

    local function stream_footer()
        if type(footer) == 'function' then footer = footer() end
        log('BYSANT-M3DA', 'DEBUG', "Streaming footer "..tostring(footer))
        serializer (footer) :close()
        footer_sent = true
    end

    local function filter (chunk)

        -- If there much more than 64KB in the buffer, it's because a lot
        -- of data has been received from last filter invocation. subsequent
        -- calls to the filter will be made with argument '' to give it a chance
        -- to flush itself.
        assert (chunk == '' or
            buf_size < CHUNK_SIZE or
            not several_chunks and buf_size < SINGLE_MAX_SIZE,
            "ltn12.pump filter protocol violation")

        result = { }

        -- If any payload data is provided, save it
        if chunk ~= nil and chunk ~= '' then
            log('BYSANT-M3DA', 'DEBUG', "Streaming %d bytes of payload", #chunk)
            table.insert(buffer, chunk); buf_size = buf_size + #chunk
        end

        -- Send the header first
        if not header_sent then stream_header() end

        if buf_size > SINGLE_MAX_SIZE or several_chunks and buf_size >= CHUNK_SIZE then
            -- There are enough data to require more than one chunk: offload the
            -- complete chunks to the sink
            if not several_chunks then -- switch to multi-chunk mode
                log('BYSANT-M3DA', 'DEBUG', "Switch to multi-chunk encoding")
                serializer :chunked()
                several_chunks = true
            end
            local whole = buffer[2] and table.concat(buffer) or buffer[1]
            local left, right = whole :sub(1, CHUNK_SIZE), whole :sub(CHUNK_SIZE+1, -1)
            buffer, buf_size = { right }, #right
            log('BYSANT-M3DA', 'DEBUG', "Flushing %d bytes of payload chunk", #left)
            serializer :chunk (left)
        elseif chunk == nil then -- body sent
            if not payload_sent then
                local result = table.concat (buffer)
                payload_sent = true
                if several_chunks then
                    serializer :chunk (result) :close()
                elseif result == '' then
                    serializer :null()
                else
                    serializer :string (result)
                end
            elseif not footer_sent then
                stream_footer()
            else -- footer sent
                return nil, nil
            end
        end
        return table.concat(result)
    end
    return filter
end

core['nil'] = core.null
core.value  = serialize_value
core.table  = serialize_table

global 'm3da'; if type(m3da) == 'table' then m3da.bysant = M end

return M
