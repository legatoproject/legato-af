-------------------------------------------------------------------------------
-- Copyright (c) 2013 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
--
-- Contributors:
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local checks     = require 'checks'
local cipher     = require 'crypto.cipher'
local md5        = require 'crypto.md5'
local ltn12      = require 'ltn12'
local src2string = require 'utils.ltn12.source'.tostring

local function get_aes_filter(mode, chaining, length, nonce, keyidx)
    checks('string', 'string', 'number', 'string', 'number')
    local obj, err = cipher.new({ -- cipher cfg
        name    = "aes",
        mode    = mode,
        nonce   = nonce,
        keyidx  = keyidx,
        keysize = length/8
    }, { -- chaining cfg
        name = chaining,
        iv   = md5():update(nonce):digest(true)
    })
    return obj:filter({name = (chaining == "cbc") and "pkcs5" or "none"})
end

local M = { }
 
-------------------------------------------------------------------------------
--- Decrypts a ciphered payload, passed as a single string
--  @param chaining name of the chaining algorithm, among "ecb", "cbc" and "ctr".
--  @param length length of keys and blocks in bits, either 128 or 256.
--  @param nonce the nonce, shared with the encrypting party, used to prime
--   the ciphered and to get an initial chaining vector.
--  @param keyidx index of the key in the key store, 1-based
--  @param ciphered_text data to decrypt.
--  @return plain text, or nil+error message, or throws an error
function M.decrypt_function(chaining, length, nonce, keyidx, ciphered_text)
    checks('string', 'number', 'string', 'number', 'string')
    local f = get_aes_filter('dec', chaining, length, nonce, keyidx)
    local src = ltn12.source.string(ciphered_text)
    return src2string(ltn12.source.chain(src, f))
end

-------------------------------------------------------------------------------
--- Encrypts a plain payload, passed as a single string
--  @param chaining name of the chaining algorithm, among "ecb", "cbc" and "ctr".
--  @param length length of keys and blocks in bits, either 128 or 256.
--  @param nonce the nonce, shared with the encrypting party, used to prime
--   the ciphered and to get an initial chaining vector.
--  @param keyidx index of the key in the key store, 1-based
--  @param plain data to encrypt.
--  @return ciphered text, or nil+error message, or throws an error
function M.encrypt_function(chaining, length, nonce, keyidx, plain)
    checks('string', 'number', 'string', 'number', 'string')
    local f = get_aes_filter('enc', chaining, length, nonce, keyidx)
    local src = ltn12.source.string(plain)
    return src2string(ltn12.source.chain(src, f))
end

-------------------------------------------------------------------------------
--- Returns an ltn12 filter deciphering a ciphered text with the appropriate
--  decryption scheme, priming nonce and keystore key.
--  @param chaining name of the chaining algorithm, among "ecb", "cbc" and "ctr".
--  @param length length of keys and blocks in bits, either 128 or 256.
--  @param nonce the nonce, shared with the encrypting party, used to prime
--   the ciphered and to get an initial chaining vector.
--  @param keyidx index of the key in the key store, 1-based
--  @return and ltn12 filter, or nil+error message, or throws an error
function M.decrypt_filter(chaining, length, nonce, keyidx)
    checks('string', 'number', 'string', 'number')
    return get_aes_filter('enc', chaining, length, nonce, keyidx)
end

-------------------------------------------------------------------------------
--- Returns an ltn12 filter ciphering a plain text with the appropriate
--  encryption scheme, priming nonce and keystore key.
--  @param chaining name of the chaining algorithm, among "ecb", "cbc" and "ctr".
--  @param length length of keys and blocks in bits, either 128 or 256.
--  @param nonce the nonce, shared with the encrypting party, used to prime
--   the ciphered and to get an initial chaining vector.
--  @param keyidx index of the key in the key store, 1-based
--  @return and ltn12 filter, or nil+error message, or throws an error
function M.encrypt_filter(chaining, length, nonce, keyidx)
    checks('string', 'number', 'string', 'number')
    return get_aes_filter('enc', chaining, length, nonce, keyidx)
end


return M
