-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched   = require "sched"
local log     = require "log"
local ltn12   = require "ltn12"
local persist = require "persist"
local m3da    = require "m3da.bysant"
local m3da_deserialize = m3da.deserializer()

local hmac   = require 'crypto.hmac'
local aes    = require 'crypto.aes'
local random = require 'crypto.random'

require 'print'

local M  = { }
local MT = { __index=M, __type='m3da.session' }

-------------------------------------------------------------------------------
-- Indexes of cryptographic keys
M.IDX_PROVIS_KS  = 1 -- server provisioning key
M.IDX_PROVIS_KD  = 2 -- device provisioning key
M.IDX_CRYPTO_K   = 3 -- encryption/decryption key
M.IDX_AUTH_KS    = 4 -- server authentication key
M.IDX_AUTH_KD    = 5 -- device authentication key

-------------------------------------------------------------------------------
-- Save an error status before causing an error.
-- This is intended to be call in `:unprotectedsend()`, directly or indirectly, and the
-- error to be caught in `:send()`. Thanks the the error status number, an
-- error message can be sent to the peer.
--
-- @param status an M3DA status, to be sent to the perr
-- @param msg a string error message to display in local logs
-- @return never returns
--
local function failwith(self, status, msg)
    checks('m3da.session', 'number|string', 'string')
    self.last_status = status
    error(msg)
end

-------------------------------------------------------------------------------
-- Returns an authentication object + corresponding LTN12 filter.
-- Data must go either through the filter or the object's `:update(data)`
-- method; the resulting hash can be retrieved with the object's `:digest(bool)`.
--
-- @param authid an authentication scheme, `'<methodname>-<hashname>'`.
-- @param keyidx an index in the keys table.
-- @return an authentication object instance.
--
function M :getauthentication(keyidx, method_hash)
    checks('m3da.session', 'number', 'string')
    local hash = method_hash :match "^hmac%-(.+)$"
    if not hash then failwith(self, 400, 'bad auth scheme') end
    local obj = assert(hmac(hash, keyidx))
    return obj
end

-------------------------------------------------------------------------------
-- Signs, encrypts and sends a message through the transport layer.
-- Will cause an error if anything goes wrong.
--
-- @param msg_src an LTN12 source producing the message's serialized stream
-- @param current_nonce the nonce to be used for encryption+authentication
-- @param next_nonce the nonce to be used next time, and to be sent to the peer.
--
function M :sendmsg(msg_src, inner_headers, current_nonce, next_nonce)
    checks( 'm3da.session', 'function', '?table', 'string', 'string')

    if log.musttrace('M3DA-SESSION', 'DEBUG') then
        -- WARNING: this detailled logging will unravel the whole incoming
        -- stream; the whole messages stream must fit in RAM.
        local str = require 'utils.ltn12.source'.tostring(msg_src)
        msg_src = ltn12.source.string(str)
        local messages, offset, msg = { }, 1
        while offset <= #str do
            msg, offset = m3da_deserialize(str, offset)
            table.insert(messages, msg)
        end
        log('M3DA-SESSION', 'DEBUG', "<- Sending  %d messages: %s",
            #messages, sprint(messages) :sub(2, -2))
    end

    inner_headers = inner_headers or { }
    inner_headers.status = inner_headers.status or 200
    inner_headers.nonce = next_nonce

    local inner_envelope = m3da.envelope(inner_headers)

    -- Authentication envelope.
    -- Authentication filter will transparently compute the mac of everything
    -- that went through it. auth_handler allows to retrieve that hash.
    -- The footer of the envelope is wrapped in a closure, so that the mac
    -- is only computed after all data went through.
    local auth = self :getauthentication(M.IDX_AUTH_KD, self.authentication)

    local function auth_footer()
        auth :update (current_nonce)
        return { mac = auth :digest (true) }
    end
    local auth_envelope = m3da.envelope({ id=self.localid }, auth_footer)

    local envelopes -- All envelopes and filters: auth envelope, optional ciphering, private envelope

    -- Encryption filter.
    -- If an encryption method is specified, then the content of the payload
    -- must go through a cipher filter to be scrambled. This filter is added
    if  self.encryption then
        local cipher_filter = aes.encrypt_filter(self.cipher_chaining, self.cipher_length, current_nonce, M.IDX_CRYPTO_K)
        envelopes = ltn12.filter.chain(inner_envelope, cipher_filter, auth:filter(), auth_envelope)
    else
        envelopes = ltn12.filter.chain(inner_envelope, auth:filter(), auth_envelope)
    end
    
    local env_src = ltn12.source.chain(msg_src, envelopes)
    if false then -- Log a copy of the data sent through transport; terrible RAM preformances!
        local snk, acc = ltn12.sink.table{ }
        ltn12.pump.all(env_src, snk)
        local env_str = table.concat(acc)
        env_src = ltn12.source.string(env_str)
        log('M3DA-SESSION', 'DEBUG', "Sending data %s", sprint(env_str))
    end
    assert(self.transport :send (env_src))
end

-------------------------------------------------------------------------------
-- Sends a challenge, i.e. an envelope with an empty body and a synchronization nonce.
--
-- @param nonce the nonce to be passed to the peer, and used in the challenge's
--   response.
--
function M :sendchallenge (nonce)
    checks('m3da.session', 'string')

    log('M3DA-SESSION', 'WARNING', "Received msg with bad nonce; sending an authentication challenge")

    local envelope = m3da.envelope {
        id        = self.localid,
        challenge = self.authentication,
        nonce     = nonce,
        status    = 401 } -- TODO 450 when missing encryption?

    local src = ltn12.source.chain(ltn12.source.empty(), envelope)
    assert(self.transport :send (src))
end

-------------------------------------------------------------------------------
-- Waits for a complete envelope to be received from the transport layer,
-- deserialize it and return it.
--
function M :receive()
    checks('m3da.session')
    log('M3DA-SESSION', 'DEBUG', "Waiting for a response")
    self.waitingresponse = true
    local ev, envelope = sched.wait(self, {'*', 60})
    self.waitingresponse = false
    if      ev == 'envelope_received' then return envelope
    elseif ev == 'timeout' then failwith(self, 408, 'reception timeout')
    elseif ev == 'reception_error' then failwith(self, 'NOREPORT', envelope) -- envelope is an error msg
    else assert(false, 'internal error') end
end

-------------------------------------------------------------------------------
-- Transport sink state:
-- `pending_data` is a string which contains the beginning of an incomplete
-- envelope;
-- `partial` is the deserializer's frozen state, to be passed back at next
-- invocation to resume the parsing where it stopped due to lack of data.
--
local pending_data = ''
local partial = nil

-------------------------------------------------------------------------------
-- sink to be passed to the transport layer, to accept incoming data from the
-- server. Push a whole envelope through the `self.incoming` pipe whenever it is
-- received.
--
-- @param data a fragment of a serialized envelope.
-- @return `"ok"` to indicate that the LTN12 sink accepted `data`.
--
function M :newsink()
    checks('m3da.session')
    return function (src_data, src_error)
        --log('M3DA-SESSION', 'DEBUG', "Received some data")
        if not src_data then
            sched.signal(self, 'reception_error', src_error)
            return nil, src_error
        else -- some data received
            pending_data = pending_data .. src_data
            local status, envelope, offset
            status, envelope, offset, partial = pcall(m3da_deserialize, pending_data, partial)
            if not status then
                local err_msg = envelope
                local log_msg =  #pending_data>80 and pending_data:sub(1, 75).."..." or pending_data
                log('M3DA-SESSION', 'ERROR', "Received non-M3DA DATA (%s): %s", err_msg, sprint(log_msg))
                sched.signal(self, 'reception_error', "Received non-M3DA data")
                return nil, err_msg
            elseif offset=='partial' then -- incomplete envelope, wait for more data
                return 'ok'
            else -- got a complete envelope: broadcast it
                assert(envelope)
                if self.waitingresponse then -- response to a request
                    sched.signal(self, 'envelope_received', envelope)
                else -- unsollicited incoming message
                    sched.run(self.parse, self, envelope)
                end
                pending_data = pending_data :sub (offset, -1)
                return 'ok'
            end
        end
    end
end

-------------------------------------------------------------------------------
-- Checks that the deserialized envelope `incoming` is properly signed.
-- Does not decrypt the payload.
--
-- @param incoming the deserialized envelope to check
-- @param nonce the current nonce for authentication and encryption
-- @return `true` or `false` depending on whether the envelope matches the
--   protocol.
--
function M :verifymsg (incoming, nonce)
    checks('m3da.session', 'table', 'string')

    local auth       = self :getauthentication(M.IDX_AUTH_KS, self.authentication)
    local actual_mac = auth :update (incoming.payload) :update (nonce) :digest (true)
    local accepted   = actual_mac==incoming.footer.mac

    if not accepted then
        log('M3DA-SESSION', 'ERROR', "Incoming message signature rejected")
    end

    return accepted
end

-------------------------------------------------------------------------------
-- Tries to send an unencrypted and unauthenticated error to the peer, with the
-- status code explaining the failure.
-- Disabled (unsupported by server).
--
function M :reporterror()
    --local envelope = m3da.envelope{ id=self.localid, status=self.last_status or 500 }
    --local src = ltn12.source.chain(ltn12.source.empty(), envelope)
    --self.transport :send (src)
end

-------------------------------------------------------------------------------
-- Implementation of the main request/response cycle. Anything that goes
-- wrong causes an error, to be caught with a `copcall`.
--
-- The cycle consists of:
--
-- * Trying to send the message, properly signed and encrypted;
-- * Wait for the response;
-- * If the response is a challenge, resend with the challenge's nonce and
--   wait for the next response;
-- * If the response isn't properly signed and encrypted, send a challenge and
--   wait for the next response, which must then be based on the proper nonce;
-- * Broadcast the authenticated and decrypted message payload to `srvcon`;
-- * Return the nonce from the peer's latest response.
--
-- @param src_factory a function returning an LTN12 source which in turn streams
--   the serialized message to send. The factory function might be called more
--   than once.
-- @param current_nonce the nonce to be used for the first
--   encryption/authentication. Other nonces might be used, e.g. if the peer
--   responds with a challenge.
-- @return the nonce to be used next time.
--
function M :unprotectedsend (current_nonce, src_factory, inner_headers)
    checks('m3da.session', 'string', 'function', '?table')
    local next_nonce = random(16)

    log("M3DA-SESSION", "INFO", "Sending data through authenticated%s session",
        self.encryption and ' and encrypted' or '')

    -- Send request and wait for response.
    self :sendmsg (src_factory(), inner_headers, current_nonce, next_nonce)
    local incoming = assert(self :receive())

    -- If the peer isn't happy with my auth/encryption,
    -- change the nonce and resend my message.
    if incoming.header.challenge then
        log('M3DA-SESSION', 'WARNING', "Server issued a challenge, resending with a new nonce")
        if log.musttrace('M3DA-SESSION', 'DEBUG') then -- WARNING: this log unstreams the source!
            local function k2h(k) return string.format("%02x", k:byte()) end
            local bad_nonce = current_nonce :gsub('.', k2h)
            local new_nonce = incoming.header.nonce :gsub('.', k2h)
            log('M3DA-SESSION', 'DEBUG', " | Refused nonce  = %q", bad_nonce)
            log('M3DA-SESSION', 'DEBUG', " | New nonce      = %q", new_nonce)
            log('M3DA-SESSION', 'DEBUG', " | Auth & crypto  = %s, %s",
                self.authentication, tostring(self.encryption))
        end
        current_nonce = assert(incoming.header.nonce)
        self :sendmsg (src_factory(), inner_headers, current_nonce, next_nonce)
        incoming = assert(self :receive())
        if incoming.header.challenge then failwith (self, 407, 'multiple challenges')
        else log('M3DA-SESSION', 'INFO', "Resynchronized message accepted by server") end
    end

    log('M3DA-SESSION', 'DEBUG', "Received a response to message sent.")

    -- Check and dispatch the incoming message.
    return self :unprotectedparse (next_nonce, incoming)
end

-------------------------------------------------------------------------------
-- Reacts to an incoming message:
--
-- * rejects it with a challenge and waits for the corrected response, if the
--   message isn't acceptable because of
--   desynchronized nonces / bad protocols / bad signature;
-- * dispatches the decrypted payload;
-- * handles the nonces, the one expected for the incoming message and the
--   next one advertized to the peer.
--
-- @param outer_env the incoming (outer) envelope, deserialized. Its serialized
--   payload should be the inner envelope.
-- @param nonce the nonce which the incoming message is expected to have used.
-- @return the nonce to be used next time.
--
function M :unprotectedparse(nonce, outer_env)
    checks('m3da.session', 'string', 'table')

    if not self :verifymsg (outer_env, nonce) then -- bad message, send a challenge and retry
        nonce = random(16)
        self :sendchallenge (nonce)
        outer_env = self :receive()
        -- Must be right the second time: we don't want to be DoS'ed
        if not self :verifymsg(outer_env, nonce) then
            failwith(self, "NOREPORT", "Bad response to a challenge")
        end
    end

    local payload = outer_env.payload
    if self.encryption then
        payload = assert(aes.decrypt_function(self.cipher_chaining, self.cipher_length, nonce, M.IDX_CRYPTO_K, payload))
    end

    log("M3DA-SESSION", "INFO", "Accepted authenticated%s response from server",
        self.encryption and " and encrypted" or "")

    local inner_env = m3da_deserialize(payload)

    local payload = inner_env.payload

    if log.musttrace('M3DA-SESSION', 'DEBUG') then
        log('M3DA-SESSION', 'DEBUG', "-> Outer header = %s", sprint(outer_env.header))
        log('M3DA-SESSION', 'DEBUG', "-> Inner header = %s", sprint(inner_env.header))
        log('M3DA-SESSION', 'DEBUG', "-> Deserialized payload = %s", sprint((m3da_deserialize(payload))))
        log('M3DA-SESSION', 'DEBUG', "-> Outer footer = %s", sprint(outer_env.footer))
    end

     -- '\0' is the bysant encoding of `nil`.
     local has_payload = payload~=nil and payload~='' and payload~='\0'

    if has_payload then self.msghandler(payload)
    else log('M3DA-SESSION', 'DEBUG', 'No payload in envelope') end

    nonce = inner_env.header.nonce
    return nonce
end


-------------------------------------------------------------------------------
-- Common helper for `:send()` and `:parse()`: wraps unprotected function, which
-- might cause an error, in a pcall, and handles the success or failure.
--
-- @param unprotected_func the function to wrap
-- @return the wrapped function
--
-- TODO: reorder parse args to improve factoring (self, nonce, ...)
local function protector_factory (unprotected_func)
    --arg1: src_factory for send, outer env for parse
    return function(self, ...)
        checks('m3da.session')
        if not self.started then
            local s, errmsg = self :start()
            if s then self.started = true else return s, errmsg end
        end
        self.last_status = false -- to be filled in case of error
        local current_nonce = persist.load("security.nonce") or random(16)
        local success, next_nonce = copcall(unprotected_func, self, current_nonce, ...)
        --local success, next_nonce = coxpcall(function() return unprotected_func(self, current_nonce, ...) end, debug.traceback)
        if success then -- success
            persist.save("security.nonce", next_nonce)
            return self.last_status or 200
        else
            local errmsg = tostring(next_nonce)
            log('M3DA-SESSION', 'ERROR', "Failed with status %s: %q",
                self.last_status or 500, errmsg)
            if self.last_status ~= "NOREPORT" then self :reporterror() end
            return tonumber(self.last_status) or 500
        end
    end
end


-------------------------------------------------------------------------------
-- Reacts to an unsollicited M3DA message. Most of the work is done by
-- `:unprotectedparse()`; `:parse()` is mainly here to catch and report errors.
--
-- Notice the similarities between this method and `:send()`.
--
-- @param incoming deserialized incoming (outer) envelope.
--
M.parse = protector_factory(M.unprotectedparse)


-------------------------------------------------------------------------------
-- Sends an M3DA message, passed as an LTN12 source factory, through a security
-- session. Most of the work is done by `:unprotectedsend()`; `:send()` is
-- mainly here to catch and report errors.
--
-- @param src_factory a function returning an LTN12 source which in turn streams
--   the serialized message to send. The factory function might be called more
--   than once.
--
M.send = protector_factory(M.unprotectedsend)


M.mandatory_keys = {
    transport=1, msghandler=1, localid=1, peerid=1, authentication=1 }

function M :start()
    if not hmac('md5', M.IDX_AUTH_KS) then
        assert(hmac('md5', M.IDX_PROVIS_KS)) -- tested by session.new()
        local P = require 'm3da.session.provisioning'
        return P.downloadkeys(self)
    else
        return 'ok'
    end
end

-------------------------------------------------------------------------------
-- Creates and returns a session instance
--
-- @param cfg a configuration table, with the fields listed in
-- `M.mandatory_keys` and `M.optional_keys`.
-- @return `"ok"`
-- @return nil + error msg
--
function M.new(cfg)
    local self = { waitingresponse = false; started = false }
    if not (hmac('md5', M.IDX_AUTH_KS) or 
            hmac('md5', M.IDX_PROVIS_KS)) then
        return nil, "Neither provisioning nor authenticating crypto keys"
    end 
    for key in pairs(M.mandatory_keys) do
        local val = cfg[key]
        if not val then return nil, 'missing config key '..key end
        self[key]=val
    end
    
    if cfg.encryption then 
        local chaining, length = cfg.encryption :match 'aes%-([a-z]+)%-([0-9]+)'
        if chaining then
            self.encryption = cfg.encryption
            self.cipher_chaining = chaining
            self.cipher_length = tonumber(length)
        else
            return nil, "Invalid encryption scheme"
        end
    end

    setmetatable(self, MT)

    self.transport.sink = self :newsink()
    return self
end

return M
