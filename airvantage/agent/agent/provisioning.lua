local security   = require 'm3da.session.security'
local write_keys = require 'crypto.keystore'
local md5       = require 'crypto.md5'
local lfs        = require 'lfs'

local M = { }

local function x(s)
    log('AGENT-PROVISIONING', 'WARNING', "%s", s)
    print(s)
end

local function k2s(str)
    local bytes = { str :byte(1, -1) }
    for i, k in ipairs(bytes) do bytes[i] = string.format("%02x", k) end
    return table.concat(bytes, ' ')
end

local function h2b(h)
    local b = { }
    for n in h :gmatch('[0-9a-zA-Z][0-9a-zA-Z]') do
        local k = string.char(tonumber('0x'..n))
        table.insert(b, k)
     end
     assert(#b==16, "A MD5 hash must be 16 bytes long")
    return table.concat(b)
end

local function md5_bin(str)
    return md5() :update (str) :digest(true)
end

local function create_directory_if_needed()
    local cryptopath = (LUA_AF_RW_PATH or lfs.currentdir())
    assert(os.execute('mkdir -p '..cryptopath..'/'..'crypto'), "Can't create crypto folder")
end

-------------------------------------------------------------------------------
-- Writes the 3 keys needed for encryption and authentication in the key store,
-- from the password's binary MD5.

function M.password_md5(K)
    local serverid = assert(agent.config.server.serverId, "Missing server.serverId in config")
    local deviceid = assert(agent.config.agent.deviceId,  "Missing agent.deviceId in config")
    x("Setting authentication and encryption key")
    create_directory_if_needed()
    x("K ="..k2s(K))
    local KS = md5() :update (serverid) :update (K) :digest(true)
    x("KS="..k2s(KS))
    local KD = md5() :update (deviceid) :update (K) :digest(true)
    x("KD="..k2s(KD))
    assert(security.IDX_AUTH_KS == security.IDX_CRYPTO_K + 1)
    assert(security.IDX_AUTH_KD == security.IDX_CRYPTO_K + 2)
    assert(write_keys(security.IDX_CRYPTO_K, { K, KS, KD }))
    x("Keys written in store")
end

-------------------------------------------------------------------------------
-- Writes the 3 keys needed for provisioning, from the preshared password's
-- binary MD5.
--
function M.registration_password_md5(K)
    checks('string')
    x("Setting pre-shared key")
    create_directory_if_needed()
    x("K ="..k2s(K))
    local serverid = assert(agent.config.server.serverId, "Missing server.serverId in config")
    local deviceid = assert(agent.config.agent.deviceId, "Missing agent.deviceId in config")
    local KS = md5() :update (serverid) :update (K) :digest(true)
    x("KS="..k2s(KS))
    local KD = md5() :update (deviceid) :update (K) :digest(true)
    x("KD="..k2s(KD))
    assert(security.IDX_PROVIS_KD == security.IDX_PROVIS_KS + 1)
    assert(write_keys(security.IDX_PROVIS_KS, { KS, KD }))
    x("Keys written in store")
end

-------------------------------------------------------------------------------
-- Stores the provisioning key, from an MD5 hash given in hexa "[0-9a-z]+".
function M.registration_password_md5_hex(Kh)
    M.preshared_md5(h2b(Kh))
end

-------------------------------------------------------------------------------
-- Stores the auth+cipher key, from an MD5 hash given in hexa "[0-9a-z]+".
function M.password_md5_hex(Kh)
    M.password_md5(h2b(Kh))
end

-------------------------------------------------------------------------------
-- Stores the provisioning key, from the original password.
function M.registration_password(K)
    M.registration_password_md5(md5_bin(K))
end

-------------------------------------------------------------------------------
-- Stores the auth+cipher key, from the original password.
function M.password(K)
    M.password_md5(md5_bin(K))
end

return M