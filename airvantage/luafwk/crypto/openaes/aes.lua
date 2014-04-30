-------------------------------------------------------------------------------
-- Copyright (c) 2013 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local core  = require 'crypto.openaes.core'
local md5   = require 'crypto.md5'

local M = { }

-- OpenAES expects a header to describe configuration, in front of data to decrypt
local OAES_HEADER="OAES\001\002\002\000\000\000\000\000\000\000\000\000"

-------------------------------------------------------------------------------
--- Decrypts a ciphered payload, passed as a single string
--  @param scheme The encryption scheme, as a string following the pattern
--   "{algorithm}-{chaining}-{keysize}"
--  @param nonce the nonce, shared with the encrypting party, used to prime
--   the ciphered and to get an initial chaining vector.
--  @param keyidx index of the key in the key store, 1-based
--  @param ciphered_text data to decrypt.
--  @return plain text, or nil+error message, or throws an error
function M.decrypt_function(chaining, length, nonce, keyidx, ciphered_text)
    checks('string', 'number', 'string', 'number', 'string')
    if chaining~='cbc' or length~=128 then return nil, "scheme not supported" end
    assert(#ciphered_text % 16 == 0, "bad ciphered_text size")
    local iv    = md5():update(nonce):digest(true)
    local oaes  = core.new(nonce, keyidx)
    local plain = core.decrypt(oaes, OAES_HEADER..iv..ciphered_text)
    local n = plain :byte(-1)
    assert(0<n and n<=16)
    return plain :sub (1, -n-1)
end

-------------------------------------------------------------------------------
--- Returns an ltn12 filter ciphering a plain text with the appropriate
--  encryption scheme, priming nonce and keystore key.
--  @param scheme The encryption scheme, as a string following the pattern
--   "{algorithm}-{chaining}-{keysize}"
--  @param nonce the nonce, shared with the encrypting party, used to prime
--   the ciphered and to get an initial chaining vector.
--  @param keyidx index of the key in the key store, 1-based
--  @return and ltn12 filter, or nil+error message, or throws an error
function M.encrypt_filter(chaining, length, nonce, keyidx)
    checks('string', 'number', 'string', 'number')
    if chaining~='cbc' or length~=128 then return nil, "scheme not supported" end
    local iv = md5():update(nonce):digest(true)  -- initialization vector
    local oaes = core.new(nonce, keyidx, iv)
    local buffer = ''      -- yet unsent data (must be sent by 16-bytes chunks)
    local lastsent = false -- has last padded segment been sent yet?
    local function filter(data)
        if lastsent then return nil
        elseif data=='' then return ''
        elseif data==nil then -- end-of-stream, flush last padded chunk
            local n = 16 - #buffer
            assert(0<n and n<=16)
            buffer = buffer .. string.char(n) :rep(n)
            -- There a header + IV, added by OpenAES, to remove.
            local result = core.encrypt(oaes, buffer) :sub(33, -1)
            lastsent = true
            return result
        else -- non-last chunk, send all completed 16-byte chunks, keep the remainder in buffer
            buffer = buffer..data
            if #buffer < 16 then return '' -- not enough data, keep in buffer until more data arrives
            else -- send every completed 16-bytes chunks
                local nkept = #buffer % 16 -- how many bytes must be kept in buffer
                local tosend = buffer :sub (1, -nkept-1)
                local tokeep = buffer :sub (-nkept, -1)
                buffer = tokeep
                local result = core.encrypt(oaes, tosend) :sub(33, -1)
                return result
            end
        end
    end
    return filter
end

function M.encrypt_function(chaining, length, nonce, keyidx, plain)
    checks('string', 'number', 'string', 'number', 'string')
    local ltn12 = require 'ltn12'
    local filter, msg = M.encrypt_filter(chaining, length, nonce, keyidx)
    if not filter then return nil, msg end
    local src = ltn12.source.string(plain)
    local snk, result = ltn12.sink.table{ }
    ltn12.pump.all(ltn12.source.chain(src, filter), snk)
    return table.concat(result)
end

function M.decrypt_filter() return nil, "not implemented" end

return M
