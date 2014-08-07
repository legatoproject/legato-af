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
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Minh Giang         for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local os          = require "os"
local systemutils = require "utils.system"
local M = {}

-- internal data, persisted and read from server
-- data table is just a proxy to enable to have update data load/unloaded as a cache (see cache functions below for more desc)
-- data table organization:
--
-- data.swlist (table) contains software status
-- data.swlist.lastupdate.result: (integer) last update job result, to be interpreted using agent.update.status.result.
-- data.swlist.lastupdate.resultdetails: (string) error details on update job
-- data.swlist.lastupdate.starttime: (integer) when the last update job was started, as returned by os.time(seconds since Epoch).
-- data.swlist.components: (table) contains "installed"/"provisioned" software components
-- data.swlist.components[uid] (table) contains the description of a component
-- data.swlist.components[uid].name: string
-- data.swlist.components[uid].uid: unique id of the component. see getcmpuid.
-- data.swlist.components[uid].version (string) version of this component
-- data.swlist.components[uid].provides (table) with features provided by this components
-- data.swlist.components[uid].depends (table) with features/components needed by this components
-- data.swlist.components[uid].parameters: (table) with parameters given in update package at install time
-- data.swlist.components[uid].force (boolean) whether this component was installed using dependency checking
--
-- data.currentupdate: (table) contains current update description if any.
-- data.currentupdate.result: (integer) global update job result, to be interpreted using agent.update.status.
-- data.currentupdate.resultdetails: (string) error details on update job failure.
-- data.currentupdate.starttime: (integer) when the update job started, as returned by os.time(seconds since Epoch).
-- data.currentupdate.status: (string) whether the current udpate is "paused" or "in_progress".
-- data.currentupdate.request: (string) pending request from user to control update process, can be "pause, "abort", "resume".
-- data.currentupdate.step: (integer) index to access STEPS tables in agent.update, identifies the current step of the update.
-- data.currentupdate.infos: (table) contains information to perform an update (especially first and last steps), fields depends on `proto` field value.
-- data.currentupdate.infos.proto: (string) describe the kind of update in progress, can be "m3da", "localupdate" or "localuninstall".
-- data.currentupdate.infos.updatefile: (string) used for "localupdate" update type only, the absolute path to the update package.
-- data.currentupdate.infos.url: (string) used for "m3da" update type only, the URL to download the update package archive.
-- data.currentupdate.infos.signature: (string) used for "m3da" update type only, the signature computed by M3DA server to be used to check archive integrity.
-- data.currentupdate.infos.ticketid: (integer) used for "m3da" update type only, ticket id to acknowledge the M3DA SoftwareUpdate command.
-- data.currentupdate.updatefile: (string) absolute path to the update package archive file.
-- data.currentupdate.update_directory: (string) absolute path to the folder where the update package have been extracted.
-- data.currentupdate.index: (integer) current component in `data.currentupdate.manifest` being updated.
-- data.currentupdate.manifest: (table) manifest table loaded from update package `Manifest` file.
-- data.currentupdate.manifest.components: see `data.swlist.components` table and sub fields description.
-- data.currentupdate.manifest.components[index].result: (integer) update job result for one component, to be interpreted using agent.update.status.
--
local data = {}


--updatepath: where update files are put: tmp files, current update files, etc
local updatepath = (LUA_AF_RW_PATH or "./").."update/"
--tmp dir to put packages stuff
local tmpdir = updatepath.."tmp/"
-- dropdir: where to look for local update package
local dropdir = updatepath.."drop/"

local function pathcheck()
    log("UPDATE", "DEBUG", "Update paths: updatepath=%s, tmpdir=%s, dropdir=%s", tostring(updatepath), tostring(tmpdir), tostring(dropdir))
    --TODO get absolute path for updatepath
    if string.find(updatepath, "'") then return nil, "pathcheck: Current working dir contains ' character, aborting!" end
    local function createpath(path)
        assert(systemutils.mktree(path))
    end
    createpath(updatepath)
    createpath(tmpdir)
    createpath(dropdir)
    return "ok"
end

-- function to escape path
--it "escapes" the whole string by surrounding it with ', but it does *not* escape embedded ' char if any.
local function escapepath(path)
    return "'"..path.."'"
end

local persistedbase="updateV4_"

local function saveobj(name) return persist.save(persistedbase..name, data[name]) end
local function saveswlist() return saveobj("swlist") end
local function savecurrentupdate() return saveobj("currentupdate") end

--cache impl:
--longevity: time to keep data in ram
--subtable.tmr: var holding timer to unload ram data, set to nil after data unload
--subtable.t: var holding the ram value of the subtable:
--subtable.default: default value to be used if nothing is load from flash
-- note: subtable.t can be nil after subtable unload or because persisted data is nil
-- so the subtable is loaded only when subtable.tmr is non nil
local cache = {longevity=30, swlist = {tmr=nil, t=nil, default={ components={} }}, -- cannot be nil, set a default value
    currentupdate = {tmr=nil, t=nil} -- is nil when no update is in progress, do not set default value
}

local function unloadsubtable(name)
    log("UPDATE", "DEBUG", "cache: unloadsubtable %s %s", tostring(name), tostring(os.time()) )
    cache[name].t=nil
    cache[name].tmr=nil
end

local checkplatformcmp -- function, implementation below

local function cachefunc_index(t, name)
    log("UPDATE", "DEBUG", "cache: cachefunc_index %s %s", tostring(name), tostring(os.time()) )
    if not cache[name] then return nil
    elseif cache[name].tmr then --table is loaded in ram
        log("UPDATE", "DEBUG", "cache: cachefunc_index found %s %s", tostring(name), tostring(os.time()) )
        cache[name].tmr:rearm()
        return cache[name].t
    else--load table
        if name=='swlist' then checkplatformcmp() end
        log("UPDATE", "DEBUG", "cache: cachefunc_index loading %s %s", tostring(name), tostring(os.time()))
        cache[name].t = persist.load(persistedbase..name) or cache[name].default
        cache[name].tmr = timer.new(cache.longevity, unloadsubtable, name)
        log("UPDATE", "DEBUG", "cache: cachefunc_index loaded %s %s", tostring(name), tostring(os.time()))
        return cache[name].t
    end
end

local function cachefunc_newindex(t, k, v)
    log("UPDATE", "DEBUG", "cache: cachefunc_newindex %s %s %s %s", tostring(t), tostring(k), tostring(v), tostring(os.time()) )
    if not cache[k] then error"cachefunc_newindex unknown sub table"
    else
        if cache[k].tmr then timer.cancel(cache[k].tmr); cache[k].tmr = nil end
        cache[k].tmr = timer.new(cache.longevity, unloadsubtable, k)
        cache[k].t = v
        saveobj(k)
    end
    log("UPDATE", "DEBUG", "cache: cachefunc_newindex end")
end

--- return cmp unique id
--- uid is used as key to present components to the server
--  cmp.name is already defining an unique id for component but cmp.name may contains '.'
--  characters. When sending the swlist to server, having a map with keys containing '.'
--  is likely to be a problem because '.' is the path separator for m3da.
--  so we need to used another id.
--
--- same uid is kept over component updates
--- accepted limitations:
---   - uid are limited to Lua number max value, i.e. 2^50 or something :)
---
local function getcmpuid(cmp)

    --search for component in data.swlist using cmp.name as id
    for i, cmptmp in pairs(data.swlist.components) do
        --if it already here, return its current uid
        if(cmptmp.name == cmp.name) then return i end
    end

    --the component was not found then generate a new uid
    local newid = data.swlist.appuid and (data.swlist.appuid + 1) or 1
    data.swlist.appuid =  newid
    saveswlist() --save newid
    return newid
end

--Update list of components 'swlist'
--if the version of component is set, we add the component in the list, otherwise we remove it
local function updateswlist(cmp, force)
    if cmp.version then -- it is an install
        local c = {} --to keep only useful component info
        c.name=cmp.name; c.version = cmp.version
        c.provides = cmp.provides and next(cmp.provides) and cmp.provides
        c.depends = cmp.depends and next(cmp.depends) and cmp.depends
        c.parameters = cmp.parameters and next(cmp.parameters) and cmp.parameters
        c.force = force and true or nil -- whether this component installation resulted from a forced update
        --using cmp.uid here is only useful when provisioning "manually" a component
        -- (e.g. "platform" cmp, see checkplatformcmp), then we use the uid given in cmp (it must be given!)
        --otherwise we use "regular" getcmpuid() that look for existing uid or create a new one
        c.uid = cmp.uid or getcmpuid(cmp)
        data.swlist.components[c.uid]=c
    else
        --if no version then it's software removal
        cmp.uid = cmp.uid or getcmpuid(cmp)
        data.swlist.components[cmp.uid] = nil
    end
    saveswlist()
end


-- Before the first access to a sw component version in `swlist`, the
-- platform-provided features are loaded.
--
-- This is done at the latest time possible rather than during init, because
-- gathering these informations might create dependencies (e.g. to `treemgr`).
--
-- The platform-specific features are to be provided in an optional global
-- function `agent.platformgetupdateplatformcomponent()`.
--
-- Once the platform features are loaded, the state consistency is checked,
-- so that users have a chance to be warned about config problems.
--
-- The function is protected to run only once per session, as it it costly.
local platformcmp_done = false
function checkplatformcmp() -- declared local on top of file
    local platform = require 'agent.platform'.getupdateplatformcomponent
    if not platformcmp_done and platform then
        platformcmp_done = true
        log('UPDATE', 'INFO', "Checking platform feature versions")
        local provides = platform()
        local cmp = {
            name     = "platform",
            version  = "N/A",
            uid      = 0,
            provides = provides }
        updateswlist(cmp, true) -- load it, indicates those cmp and features come from forced provisioning
        local check = require 'agent.update.pkgcheck'
        local res, err = check.checkswlist() --look for inconsistencies
        if not res then
            log("UPDATE", "WARNING", "The software state is not valid at boot, err=%s", tostring(err))
        end
    end
end





local function init()
    pathcheck()
    --data cache init
    data = assert(setmetatable( data, {__index=cachefunc_index, __newindex=cachefunc_newindex}))
    return "ok"
end

local updatecleanupflag = true

local function updatecleanup()
    local res, err
    --remove files
    if updatecleanupflag and data.currentupdate then
        if data.currentupdate.update_directory and lfs.attributes(data.currentupdate.update_directory) then
            local res, err = os.execute("rm -rf "..escapepath(data.currentupdate.update_directory))
            if res ~= 0 then log("UPDATE", "WARNING", "Cannot remove last update directory for cleaning, err=%s",tostring(err)) end
        end
        --the archive file should already be removed.
        if data.currentupdate.updatefile and lfs.attributes(data.currentupdate.updatefile) then
            local res, err = os.execute("rm -rf "..escapepath(data.currentupdate.updatefile))
            if res ~= 0 then log("UPDATE", "WARNING", "Cannot remove update archive for cleaning, err=%s",tostring(err)) end
        end
    end
end

--Get free space on Linux partition, Linux partition is deduced from path argument
--@param path the path as a string to be used to deduced the Linux partition, at least one char long.
--@return nil and display message error  when error happened
--@return free space as a number of bytes when it worked
local function getfreespace(path)
    if not string.match(path,"%a") then
        return nil, "the path is not valid"
    end
    --df returns size in Kbytes.
    local cmdspace = "df " .. path .. " | tail -1 | awk '{print $4}' "
    local err, output = systemutils.pexec(cmdspace)
    --check conversion to number: pexec status might not be enough, especially because of the pipes in previous sys cmd.
    local res = tonumber(output)
    if err ~= 0 or not res then
        return nil, string.format("Cannot get free space for storage, err=%s",tostring(output or err))
    end
    return res*1024 -- return size in bytes.
end




M.data = data
M.escapepath = escapepath
M.updatepath = updatepath
M.dropdir = dropdir
M.tmpdir = tmpdir
M.saveswlist = saveswlist
M.savecurrentupdate = savecurrentupdate
M.updateswlist = updateswlist
M.updatecleanup = updatecleanup
M.init = init
M.getfreespace = getfreespace

return M;
