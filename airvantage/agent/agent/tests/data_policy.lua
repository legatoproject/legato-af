--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
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
--     Cuero Bugot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------


local u = require 'unittest'
local racon = require 'racon'
local tm = require 'agent.treemgr'


local t = u.newtestsuite("data_policy")

function t:setup()
  u.assert(racon.init())
end

function t:teardown()
end




function t:test_trigger_unknown_policy()
  local s, err = racon.triggerpolicy("__0123randompolicy__")
  u.assert_nil(s)
  u.assert_equal("no such policy", err)
end

function t:test_trigger_existing_policy()
  local s, err = racon.triggerpolicy("now")
  u.assert_equal("ok", s)
  u.assert_nil(err)
end

function t:test_trigger_never_policy()
  local s, err = racon.triggerpolicy("never")
  u.assert_nil(s)
  u.assert_equal("Don't flush the 'never' policy", err)
end

function t:test_add_new_manual_policy()
  u.assert(tm.set("config.data.policy.newfakepolicy1", {manual=true}))
  u.assert(racon.triggerpolicy("newfakepolicy1"))
end

function t:test_add_new_latency_policy()
  u.assert(tm.set("config.data.policy.newfakepolicy2", {latency=15}))
  u.assert(racon.triggerpolicy("newfakepolicy2"))
end

function t:test_add_new_onboot_policy()
  u.assert(tm.set("config.data.policy.newfakepolicy3", {onboot=12}))
  u.assert(racon.triggerpolicy("newfakepolicy3"))
end

function t:test_add_new_period_policy()
  u.assert(tm.set("config.data.policy.newfakepolicy4", {period=13}))
  u.assert(racon.triggerpolicy("newfakepolicy4"))
end

function t:test_add_new_cron_policy()
  u.assert(tm.set("config.data.policy.newfakepolicy5", {cron="* * * * *"}))
  u.assert(racon.triggerpolicy("newfakepolicy5"))
end


function t:test_reconfigure_policy()
  u.assert(tm.set("config.data.policy.now", {latency=7}))
  u.assert(racon.triggerpolicy("now"))
end
