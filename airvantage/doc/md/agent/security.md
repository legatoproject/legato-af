# M3DA Server connection and security

## Overview

Data transmission from the agent to the server are handled in M3DA. It goes
through several modules:

* A transport module, which exchanges LTN12 byte streams with the
  server, called `m3da.transport.xxx`. Currently supported transport
  layers are `tcp` and `http`.

* A session module, which converts between M3DA messages on the agent
  side, and byte streams on the transport side. Messages come from the
  agent as LTN12 source factories, i.e. functions returning an LTN12
  source; the use of LTN12 allows to stream large messages without
  holding them entirely in RAM; they're passed as factories, because
  the session might have to emit them more than once, e.g. if an
  authentication failure occurred at the first attempt.

  In the other direction, server messages going to the agent, they're
  received by the session through an LTN12 sink, then pushed, one
  serialized message at a time, to a callback passed at initialization
  time. Those messages are expected to be rather short, hence are
  passed as strings rather than streams.

  The session layer is in charge of putting sent messages into M3DA
  envelopes (and extracting incomping messages from their
  envelopes). Currently supported session layer are
  `m3da.session.default` and `m3da.session.security`, the latter
  supporting mandatory authentication, plus optional encryption and
  password provisioning.

* The provisioning module is triggered by the security session module
  when it lacks its authentication+encryption password. It uses a
  registration password (generally shared by several devices) to
  retrieve an actual password (proper to a single device).

* agent.srvcon handles communications for the agent: it initializes
  transport and session layers, accumulates data streams to send to
  the server, dispatches incoming server messages to the appropriate
  asset connector.

## Detailed API

### Transport modules

Transport modules must have a function `new(url)`, which returns `nil`
+ error message upon failure, or an object with the following properties:

* a `:send(src)` method, sending the content of an LTN12 source to the
  server, returning a non-nil value upon success, `nil`+error message
  upon failure;

* a `sink` field, to be set externally, which contains an LTN12 sink
  where data coming from the server are fed.

The `url` parameter of the `new()` function must be sufficient to
fully configure the transport layer, and the scheme part of the url
(the initial word before `"://"`) must match the last part of the
module name.

### Session modules

Session modules must have a function `new(cfg)`, which returns `nil`
+ error message upon failure, or an object with the following
properties:

* a `:send(src_factory, headers)` method, whose parameter is a function
  returning an LTN12 source. The source must stream a serialized M3DA
  message, and if `src_factory` is called more than once, it must be
  able to serve the source more than once. If `src_factory` is called
  a second time, returning a second source, the first source will not
  be used anymore. `:send()` return non-`nil`, or `nil` + error
  message.  `headers` is an optional table of key/value pairs, which will
  be passed as the envelope header (inner envelope header in case of
  secured connection). This table will be modified by `:send()`: it will
  add an `id` field in it.

* a `:newsink()` method, which returns an LTN12 sink. This sink will
  receive data from the transport layer; those data represent
  (possibly partial) M3DA envelopes sent by the server to the agent.

* optionally, a `:start()` method. If it exists, it will be run with the
  network connection unabled before any other send/receive operation is
  requested from the session instance object. If this method exists and
  returns `nil` + message, the session object will be considered invalid.
  This mechanism is used by security session objects to provision encryption
  and authentication keys when they are missing.


The `cfg` parameter for `new()` is a table. It must have the following
fields, plus optionally some others specific to a given session type:

* `transport` is an already initialized transport object. The session
  initializer must set its `sink` field.

* `msghandler` is a function which will be called by the session
  object everytime a message is received from the server. This
  function takes a single parameter, which consists of one or several
  M3DA messages serialized into a string.

* `localid` is the device's identifier.

* `peerid` is the server's identifier.

### agent.srvcon

The server connector has the following public APIs:

* `init()` initializes the module; it retrieves its parameters from
  `agent.config`.

* `pushtoserver(src_factory, callback)` accumulates data to be sent to
  the server. Data is represented as an LTN12 source factory; an
  optional callback function can be provided: it will be executed once
  the data will have been successfully sent to the server.

* `connect(delay)` ensures that all data accumulated with
  `pushtoserver()` and not yet successfully sent, will be sent to the
  server in no more than `delay` seconds.

## Key management

Key are stored on embedded devices in a file
`crypto/crypto.keys`. They are obfuscated with an AES key, but this
doesn't constitute proper encryption, as the key is in the code and
can be retrieved by someone with access to the hardware. The
possibility is contemplated, for future versions of the agent, to let
users plug alternative obfuscation methods in the system.

Another risk mitigation measure is that keys retrieved from the key
store are never passed directly to Lua. Instead, all Lua cryptographic
primitives can access keys through their index in the key store. This
means that every key used to encrypt or sign data must be put in the
store, even if it is derived from a key already present in the store.

The keys are, in order:

    PROVIS_KS = MD5(server_id .. MD5(registration_password))
    PROVIS_KD = MD5(device_id .. MD5(registration_password))
    CRYPTO_K  = MD5(password)
    AUTH_KS   = MD5(server_id .. MD5(password))
    AUTH_KD   = MD5(device_id .. MD5(password))

Where `registration_password` is the provisioning secret, generally
common to a series of devices, and used to download the actual signing
and encryption key `CRYPTO_K`. password is the final, device-specific
secret used to generate the actual security keys.

The keys have indexes 1...5 in Lua. Beware that in the keystore and
the C code, they have indexes 0...4.

Registration passwords as well as actual passwords can be provisionned
in a device through telnet as follows:

* after building the agent, run `make agent_provisioning` in the build
  directory (the manual key provisioning system isn't built included
  by default);
* launch the agent and connect through telnet on port 2000;
* then you can set the preshared secret:
  `require 'agent.provisioning' .registration_password "foobar"`
* you can also set the cipher+authentication password directly:
  `require 'agent.provisioning' .password "letMeIn"`

There's also a C API, but at a lower level; you have to compute the
keys yourself before writing them down in the store. The writing is
preformed by `set_plain_bin_keys( first_key, n_keys, keys)`, in
`libs/keystore/keystore.{c,h}`.

In the keystore, keys are ciphered with an AES-128 key embedded in the
code. Moreover, this key is rotated n bytes to the left to encrypt the
key at index n (this prevents identical keys from having the same
encrypted forms).
