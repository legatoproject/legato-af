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

local ltn12   = require "ltn12"
local bit32   = require 'bit32'
local m3da    = require 'm3da.bysant'
local cipher  = require "crypto.cipher"
local hmac    = require "crypto.hmac"
local md5     = require "crypto.md5"
local ecdh    = require "crypto.ecdh"
local random  = require "crypto.random"

local m3da_deserialize = m3da.deserializer()


require 'print'


local M = { }

-------------------------------------------------------------------------------
-- Perform bitwise
local function xor_string(s1, s2)
    assert(#s1==#s2)
    local t1, t2 = { s1 :byte(1, -1) }, { s2 :byte(1, -1) }
    local t3 = { }
    for i=1, #s1 do t3[i] = bit32.bxor(t1[i], t2[i]) end
    return string.char(unpack(t3))
end

-------------------------------------------------------------------------------
-- Requests crypto keys from the server with the provisioning keys,
-- stores them in the keystore for future use.
--
-- The crypto key should be asked only once in the device's lifetime, the first
-- time the agent successfully connects to Internet.
--
-- The sequence is:
-- * message 1D to server: send a device signing salt against replay attacks.
-- * message 1S from server: receive a server signing salt against replay attacks.
-- * message 2D to server: send a signed DH pubkey to establish shared secret.
-- * message 2S from server: send the 2nd half of DH, signed, and DH-ciphered key K.
-- * message 3D to server: acknowledge provisioning success, signed.
--
function M.downloadkeys(session)
    checks('m3da.session')

    log('M3DA-PROVISIONING', 'WARNING', "Requesting definitive credential from the server")

    -- Generates signing stuff: an LTN12 filter to pass data trhough, and
    -- an envelope footer generator.
    local function signer(salt)
        local handler = session :getauthentication(session.IDX_PROVIS_KD, 'hmac-md5')
        local function footer()
            handler :update (salt)
            return { autoreg_mac = handler :digest (true) }
        end
        return handler:filter(), footer
    end

    -- message 1D: send device signing salt (neither signed nor ciphered)
    local salt_device = random(16)
    local env_1d = ltn12.source.chain(
        ltn12.source.empty(),
        m3da.envelope{ id = session.localid, autoreg_salt = salt_device })
    log('M3DA-PROVISIONING', 'DEBUG', "Sending device salt")
    local s, err = session.transport :send (env_1d) -- Second exchange (1): send device pubkey
    if not s then return nil, "env_1d: "..err end

    -- message 1S: receive server signing salt (neither signed nor ciphered)
    local env_1s
    env_1s, err = session :receive()
    if not env_1s then return nil, "env_1s: "..err end
    if env_1s.header.challenge then
        local msg = "Server won't re-provision the cipher+auth key"
        log('M3DA-PROVISIONING', 'ERROR', msg)
        return nil, msg
    end 
    local salt_server = env_1s.header.autoreg_salt
    if not salt_server then return nil, "missing server salt in 1st provisioning msg" end
    if log.musttrace('M3DA-PROVISIONING', 'DEBUG') then
        log('M3DA-PROVISIONING', 'DEBUG', "Received server salt %s", sprint(salt_server))
    end

    local privkey_device, pubkey_device = ecdh.new()

    -- message 2D: send signed ECC-DH pubkey (signed with PROVIS_KD + server salt)
    local env_2d_inner = m3da.envelope{ autoreg_pubkey=pubkey_device }
    local filter_2d, footer_2d = signer(salt_server)
    local env_2d_outer = m3da.envelope({ id=session.localid }, footer_2d)
    local chain  = ltn12.filter.chain(env_2d_inner, filter_2d, env_2d_outer)
    local env_2d = ltn12.source.chain(ltn12.source.empty(), chain)
    if false then
        local str = require 'utils.ltn12.source'.tostring(env_2d)
        env_2d = ltn12.source.string(str)
        local ext = m3da_deserialize(str)
        local int  = m3da_deserialize(ext.payload)
        printf("Header Ext=%s\nHeader Int=%s\nFooter Ext=%s",
            sprint(ext.header), sprint(int.header), sprint(ext.footer))
    end
    log('M3DA-PROVISIONING', 'DEBUG', "Sending authenticated device public ECC-DH key")
    s, err = session.transport :send (env_2d)
    if not s then return nil, "env_2d: "..err end

    -- message 2S: receive peer ECC-DH pubkey and ciphered K, check signature (PROVIS_KS + device salt)
    local env_2s, err = session :receive()
    if not env_2s then return nil, err end
    log('M3DA-PROVISIONING', 'DEBUG', "Received authenticated server public ECC-DH key + ciphered provisionned key")
    local hmac = session :getauthentication(session.IDX_PROVIS_KS, 'hmac-md5')
    local expected = hmac :update(env_2s.payload) :update(salt_device) :digest(true)
    assert (expected == env_2s.footer.autoreg_mac, "Bad server signature for 2nd provisioning msg")
    local env_2s_inner = m3da_deserialize(env_2s.payload)
    local pubkey_server = env_2s_inner.header.autoreg_pubkey
    local ctext = env_2s_inner.header.autoreg_ctext
    if not pubkey_server or not ctext then return nil, "malformed provisioning response env_2s" end

    -- decipher K, compute KS and KD, put in store
    local secret = ecdh.getsecret(privkey_device, pubkey_server)
    local secret_md5 = md5() :update (secret) :digest(true)
    local K  = xor_string(secret_md5, ctext)
    log('M3DA-PROVISIONING', 'DEBUG', "Got provisionned key")
    local KS = md5() :update (session.peerid)  :update (K) :digest(true)
    local KD = md5() :update (session.localid) :update (K) :digest(true)

    -- Verify that key indexes are consecutive, as they must be
    assert(session.IDX_AUTH_KS == session.IDX_CRYPTO_K + 1)
    assert(session.IDX_AUTH_KD == session.IDX_CRYPTO_K + 2)
    assert(cipher.write(session.IDX_CRYPTO_K, { K, KS, KD }))

    -- message 3D: acknowledge success (signed with PROVIS_KD + server salt)
    local env_3d_inner = m3da.envelope{ status=200 }
    local filter_3d, footer_3d = signer(salt_server)
    local env_3d_outer = m3da.envelope({ id=session.localid }, footer_3d)
    local chain = ltn12.filter.chain(env_3d_inner, filter_3d, env_3d_outer)
    local env_3d = ltn12.source.chain(ltn12.source.empty(), chain)
    log('M3DA-PROVISIONING', 'DEBUG', "Sending final acknowledgment")
    s, err = session.transport :send (env_3d)
    if not s then return nil, "env_3d: "..err end
    log('M3DA-PROVISIONING', 'DEBUG', "Acknowledgment sent")

    log('M3DA-PROVISIONING', 'INFO', "Credential provisioning successful")

    log('M3DA-PROVISIONING', 'WARNING', "Temporization (workaround)")
    sched.wait(10)

    return 'ok'
end

return M
