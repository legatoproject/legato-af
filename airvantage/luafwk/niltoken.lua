-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--- Module to provide Nil token.
--
-- Purpose
-- -------
--
-- The `nil` value is somewhat special in Lua, and can be difficult to use
-- in some contexts. This module offers a unified way to circumvent these
-- difficulties.
--
-- The problem
-- -----------
--
-- In Lua, `nil` means "nothing". As such, in a table, there is no way to
-- tell apart a key associated with the value `nil` from a key which doesn't
-- exist at all in this table. For instance, `{ foo=123, bar=nil }` is exactly
-- the same as `{ foo=123 }`.
--
-- However, in some contexts, Lua tables are used to represent data structures
-- which might contain a `NULL`/`nil` value. For instance, if the server wants
-- to indicate that it erased the variable `foo`, it cannot simply send the
-- record `{ foo=nil }`: it would be undistinguishable from the empty record
-- `{ }`. Similarly, Hessian has a `Null` value which almost naturally maps to
-- Lua's `nil`; but Hessian's `Null` can be used as a record value, and even as
-- a record key, whereas Lua's `nil` can't.
--
-- The solution
-- ------------
--
-- The way to address those issues of representing a null value when `nil` can't
-- be used is to have a token, an arbitrary object which by convention means null.
-- Whenever the need for such a token appears, we use the same `niltoken` object
-- as this token.
--
-- It is preferable for `niltoken` not to be a naturally easy to serialize type,
-- such as a table. This mainly leaves `userdata` and functions as good candidates.
-- For reasons exposed below, it has been decided to represent `niltoken` as
-- a function.
--
-- Converting between `niltoken` and `nil`
-- ---------------------------------------
--
-- There are three common operations to perform on `niltoken` and tables likely
-- to contain references to `niltoken`:
--
-- * Test for equality with `niltoken`, as in `if foo==niltoken then ... end`;
-- * Convert `niltoken` references into actual `nil`;
-- * Convert `nil` values into `niltoken` references.
--
-- Equality tests are performed by the usual `==` Lua operator. Conversions
-- are performed by `niltoken` itself, which is a function:
--
-- * It converts `niltoken` references into `nil`:
--   `assert(niltoken(niltoken)==nil)`;
-- * It converts `nil` values into `niltoken` references:
--   `assert(niltoken(nil)==niltoken)`.
-- * It leaves everything else alone:
--   `if a~=nil and a~=niltoken` then assert(niltoken(a)==a) end`.
--
-- Packaging
-- ---------
--
-- the `niltoken` function is directly returned as the `niltoken` module, i.e.
-- the idiomatic way to use it is to put a `local niltoken = require 'niltoken'`
-- statement at the top of modules which might need it.
--
-- @module niltoken
--

local function niltoken(v)
    if v==niltoken then return nil
    elseif v==nil then return niltoken
    else return v end
end

return niltoken