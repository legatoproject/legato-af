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
-------------------------------------------------------------------------------

local common     = require "agent.update.common"
local config     = require "agent.config"
local utable     = require "utils.table"
local socket     = require "socket"
local log        = require "log"
local lfs        = require "lfs"
local upderrnum  = require "agent.update.status".tonumber
local system     = require "utils.system"
local data       = common.data
local escapepath = common.escapepath

local state = {}
local M = {}


local checkmanifest;
local checkcomponent;
local resultcode

-- pre update checks: extraction, manifest loading, manifest checking, ...
-- return nil, error_string in case of an error
-- REMOVES THE ARCHIVE AFTER EXTRACTION
-- DOES NOT CLEAN EXTRACTED FILES
local function checkpkg()
    local res,err
    local updatefile

    --this the beggining of a step
    -- init internal values
    -- no partial resume here:

    --precond checking
    if string.find(data.currentupdate.updatefile, "'") then
        return state.stepfinished("failure", upderrnum 'INVALID_UDPATE_FILE_NAME' , "Invalid update file name")
    end

    -- create a directory with the same name that the updatefile
    -- where the update package will be extracted
    local pkgname = string.match(data.currentupdate.updatefile, ".*/(.*)%.tar.-")
    if not pkgname then
        --report FUMO_RESULT_CLIENT_ERROR (previous check must avoid that)
       return state.stepfinished("failure", upderrnum 'UDPATE_FILE_NAME_PARSING_ERROR', "Cannot parse the update file name correctly")
    end

    local dirname = common.tmpdir .. pkgname
    updatefile = data.currentupdate.updatefile
    
    if dirname:sub(1,1) == "/" then
        --common.tmpdir was already an absolute path, dirname can be used directly
        data.currentupdate.update_directory = dirname
    else
        --common.tmpdir was a relative path (with starting "." char or not...), dirname needs to be changed.
        --Get absolute path if we can get it: the absolute path will be sent to user callback
        local currentdir, error =  lfs.currentdir()
        if not currentdir then
            log("UPDATE", "WARNING", "Cannot get the absolute path: err=%s, user callback will receive relative path!", tostring(error))
        end
        data.currentupdate.update_directory = ((currentdir and currentdir .. string.gsub(dirname, "^%.","",1)) or dirname)  --gsub remove the "." before dirname, if any.
    end

    --preventive directory removal before mkdir
    res, err  = os.execute("rm -rf "..escapepath(dirname))
    res, err = lfs.mkdir(dirname)
    if not res then
        --mkdir error
        return state.stepfinished("failure", upderrnum 'EXTRACTION_FOLDER_CREATION_FAILED', string.format("Cannot create folder to extract update package, err =[%s]", tostring(err)))
    end

    -- test extraction: check free space
    -- local freespace = statvfs.f_bfree * statvfs.f_bsize
    -- local _, nb_files = system.pexec("tar -tvf testasset_standalone_pkg.tar | wc -l")
    -- local nb_files = tonumber(nb_files)
    -- local uncompressed_size = system.pexec("tar -xvf testasset_standalone_pkg.tar -O | wc -c")
    -- this lefts a little margin: uncompressed_size of each file is majored by blocksize
    -- local size_needed = uncompressed_size + (nb_files * statvfs.f_bsize)
    -- if size_needed >= freespace then end

    -- extract file to that directory
    res, err = os.execute("tar -C "..escapepath(dirname).." -xvf "..escapepath(updatefile))
    if 0 ~= res then --[[and 512 ~=res ]]--
       return state.stepfinished("failure", upderrnum 'PACKAGE_EXTRACTION_FAILED', string.format("Cannot extract update package, tar res=[%s, %s]", tostring(res), tostring(err)))
    end

    --load manifest
    local mfile, err = io.open(dirname.."/".."Manifest", "r")
    if not mfile then return state.stepfinished("failure", upderrnum 'MANIFEST_OPENING_FAILED', string.format("Cannot open manifest file, err=[%s]", err)) end
    local manifest_data, err = mfile:read("*a")
    if not manifest_data then return state.stepfinished("failure", upderrnum 'MANIFEST_READING_FAILED', string.format("Cannot read manifest file, err=[%s]", err)) end
    mfile:close()
    manifest_data = "return "..manifest_data

    local manifest_chunk, err = loadstring(manifest_data, "update:manifest_data")
    if not manifest_chunk or "function" ~=  type(manifest_chunk) then
       return state.stepfinished("failure", upderrnum 'MANIFEST_LOADING_FAILED', string.format("Cannot load manifest chunk, err =[%s]", err))
    end

    --get the table from the chunk
    local res, manifest = pcall(manifest_chunk)
    if not res then
        --not valid file
        return state.stepfinished("failure", upderrnum 'MANIFEST_INVALID_LUA_FILE',  string.format("Manifest file is not valid lua file, err=[%s]", manifest or "error unknown"))
    elseif "table" ~= type(manifest) then
        --doesn't return a table
        return state.stepfinished("failure", upderrnum 'MANIFEST_INVALID_CONTENT_TYPE',  string.format("Manifest file doesn't returns a table but %s", type(manifest)))
    end

    -- check format / syntax / dependencies
    local res, errcode, errstr = checkmanifest(manifest)
    if not res then
        errstr = errstr or errcode or "unknown"
        errcode = tonumber(errcode) or upderrnum 'MANIFEST_DEFAULT_ERROR'
        return state.stepfinished("failure", errcode, "Manifest error:"..tostring(errstr))
    end

    --we're good
    data.currentupdate.manifest = manifest
    --no need to call savecurrentupdate: stepfinished will do it for us
    log("UPDATE", "INFO", "Software Update Package from %s protocol is accepted.", data.currentupdate.infos.proto)
    return state.stepfinished("success")
end

-- this should not be necessary but we want to be sure that none of those operation crashes
local checkpackage = function ()
    local f = socket.protect(checkpkg);
    local res, err = f();
    if not res then
        log("UPDATE", "INFO", "checkpackage failed: %s", tostring(err))
        return state.stepfinished("failure", upderrnum 'DEFAULT_INTERNAL_ERROR', string.format("checkpackage: internal error: %s", tostring(err)))
    end
end
local updateswstate

local versionvalidchars = "%w%.%-_:"

local function checkversion(version)
    return not version:match("[^"..versionvalidchars.."]")
end

local function checkman(manifest)

    --type checking fct for depends and provides fields
    --provides table values must be valid version strings
    local function testtabletypes(t, checkvalue)
        for k, v in pairs(t) do
            if "string" ~= type(k) or "string" ~= type(v) or k == "" or v == "" then return nil end
            if checkvalue and (not checkversion(v)) then return nil end
        end
        return "ok"
    end

    assert( (not manifest.version) or ( type(manifest.version) == "string" and  manifest.version ~= "" and checkversion(manifest.version) ), "manifest 'version' not valid")
    assert( (not manifest.force) or type(manifest.force) == "boolean", "manifest 'force' not valid")
    assert(manifest.components and type(manifest.components)== "table", "manifest 'components' not valid")

    --prepare dependency checking
    --cmps and feats status table to keep during update check process
    local swstate={cmps={}, feats={}}
    -- fill in the initial software status
    -- this is the only place where data.swlist.components has to be used (for dependencies checks)
    -- after the simulated swstate is used.
    for i, cmp in pairs(data.swlist.components) do
        updateswstate(swstate, cmp, true)
    end

    --check each component
    -- go through manifest.components *in the order* (swstate is simulated step by step in the order of installation, in order to check deps)
    for _, cmp in ipairs(manifest.components) do
        --type checks
        assert(cmp.name and type(cmp.name) == "string" and cmp.name ~= "", "component 'name' not valid")
        assert( ( not cmp.location) or ( type(cmp.location) == "string" and cmp.location ~= ""), cmp.name.." 'location' not valid")
        assert( ( not cmp.version)  or ( type(cmp.version) == "string" and cmp.version ~= ""  and checkversion(cmp.version) ), cmp.name.." 'version' not valid")
        --when given, location must point to a valid file.
        if cmp.location then
            cmp.file = data.currentupdate.update_directory.."/"..cmp.location
            local res, err = lfs.attributes(cmp.file, "mode")
            assert(res, string.format("%s path [%s] targeted by 'location' field cannot be accessed", cmp.name, cmp.location)) --use custom error in assert, not lfs error
        end

        --TODO location mandatory when version not nil (install/update)
        assert( (not cmp.version) or cmp.location, cmp.name.." 'location' is needed when version is set" )
        --
        assert( cmp.version or ( (not cmp.provides) and ( not cmp.depends) ), cmp.name.." set for removal must not declare 'provides' or 'depends' fields")
        --checks `depends` content if set
        if cmp.depends then
            assert("table" == type(cmp.depends) and testtabletypes(cmp.depends, false), cmp.name.." 'depends' field is invalid")
        end
        --checks `provides` content if set
        if cmp.provides then
            assert("table" == type(cmp.provides) and testtabletypes(cmp.provides, true), cmp.name.." 'provides' field is invalid")
            assert( not cmp.provides[cmp.name],  cmp.name.." 'provides' field is invalid:  cannot contain feature with same name than component")
        end

        -- default values to ease next steps
        cmp.provides = cmp.provides or {}
        cmp.depends = cmp.depends or {}

        if not manifest.force then
            --check dependencies for that component
            local res, err = checkcomponent(swstate, cmp)
            if not res then return nil, upderrnum 'DEPENDENCY_ERROR', err or "Dependency error for cmp "..cmp.name end
        end
        --prepare retry counts
        cmp.retries = 0
    end

    return "ok"
end

checkmanifest = socket.protect(checkman);


-- return false, error in case of malformed dependency or
-- return false, 0 when the dependency is satisfied
-- return true, ~=0 when the dependency is not satisfied
local function cmp_version(version, deps)
    --each dep is separated by a space
    --the whole dependencies is a logical AND of all dependency
    for dep in deps:gmatch("([^ ]*) ?") do
        if dep == "" then break end -- last match, TODO improv
        local op, vdep = dep:match("^([><=#!][=]?)(.+)$")
        --list management
        if op == "#" then
            local res
            for listval in vdep:gmatch("([^,]*),?") do
                if listval == "" then break end -- last match, TODO improv
                if not listval or not checkversion(listval) then return nil, dep.." malformed dependency" end
                if version == listval then res="ok"; break; end
            end
            if not res then return nil,  dep.." failed" end
        else
            if not vdep or not checkversion(vdep) then return nil, dep.." malformed dependency" end
            --regular operator management
            if op == "=" then if not (version==vdep) then return nil, dep.." failed" end
            elseif op == "!=" then if not (version~=vdep) then return nil, dep.." failed" end
            elseif op == ">" then if not (version>vdep) then return nil, dep.." failed" end
            elseif op == "<" then if not (version<vdep) then return nil, dep.." failed" end
            elseif op == ">=" then if not (version>=vdep) then return nil, dep.." failed" end
            elseif op == "<=" then if not (version<=vdep) then return nil, dep.." failed" end
            else return nil, dep.." : malformed dependency" end
        end
    end
    return "ok"
end


--as each feature is given as:
--feats[featname][cmpthatprovides]=version
--this is the same to install a new cmp or update it regarding the features it provides
--just fill all feats as shown above, fields to be updated will be automatically.
function updateswstate(swstate, cmp, install)
    --local cmps = swstate.cmps
    --local feats = swstate.feats
    --p("updateswstate before ", "cmps", swstate.cmps, "feats", swstate.feats, "cmp", cmp, install)
    if install then
        if swstate.feats[cmp.name] then return nil, string.format("component %s can not be installed because a feature with the same name exists", cmp.name) end
        swstate.cmps[cmp.name] = {}
        utable.copy(cmp, swstate.cmps[cmp.name], true) --do not alter cmp content (for default values)
        swstate.cmps[cmp.name].provides = swstate.cmps[cmp.name].provides or {}
        swstate.cmps[cmp.name].depends = swstate.cmps[cmp.name].depends or {}
    else
        --cmp = swstate.cmps[cmp.name] --will need to go though installed feats
        swstate.cmps[cmp.name] = nil
    end
    --recompute feature swstate (so that it takes care of feature added, feature updated and feature removed.)
    swstate.feats={}
    for _, c in pairs(swstate.cmps) do
        for feature, version in pairs(c.provides) do
            if swstate.cmps[feature] then return nil,  string.format("invalid swstate while acting on component %s: feature %s has the same name as a component", cmp.name, feature) end
            if not swstate.feats[feature] then swstate.feats[feature] = {} end
            swstate.feats[feature][c.name] = version
        end
    end
    --p("updateswstate after ", "cmps", swstate.cmps, "feats", swstate.feats, "cmp", cmp, install)
    return "ok"
end

--only components have theirs dependencies checked
local function checkswstate(swstate)
    --p("checkswstate", "cmps", swstate.cmps, "feats", swstate.feats)
    --check each cmp dep are ok
    for cmpname, cmp in pairs(swstate.cmps) do
        --check each dep of the current cmp being validated
        for depname, depval in pairs(cmp.depends) do
            if swstate.cmps[depname] then
                -- only one version of a cmp
                assert(cmp_version(swstate.cmps[depname].version, depval), string.format("%s depends on component %s as %s, the component is present but the dependency is not satisfied", cmpname, depname, depval ))
            elseif swstate.feats[depname] then
                local res
                --check each version of a feature, at least one must be good
                for _, vprovided in pairs(swstate.feats[depname]) do
                    if cmp_version(vprovided, depval) then res = "ok"; break; end
                end
                assert(res, string.format("%s depends on feature %s as %s, the feature is present but the dependency is not satisfied",  cmpname, depname, depval))
            else
                --no cmp/feature match dep
                error(string.format("%s depends on element %s as %s, the element is not found",  cmpname, depname, depval))
            end
        end
    end
    return "ok"
end

local checkstate = socket.protect(checkswstate)


local function checkcmp(swstate, newcmp)
    local type
    if not newcmp.version then
        assert(swstate.cmps[newcmp.name], string.format("no installed component %s found to be removed", newcmp.name))
        type="removed"
    elseif not swstate.cmps[newcmp.name] then type="installed"
    else type="updated" end

    assert(updateswstate(swstate, newcmp, type ~="removed"))
    local res, err = checkstate(swstate)
    return res, string.format("%s cannot be %s, error=%s", newcmp.name, type, tostring(err))
end

checkcomponent = socket.protect(checkcmp)

--to be run externally

local function checkswlist()
    --prepare dependency checking
    local swstate={cmps={}, feats={}}
    if not data.swlist or not data.swlist.components then return nil, "invalid swlist, swlist and swlist.components must exist" end
    for i, cmp in ipairs(data.swlist.components) do
        updateswstate(swstate, cmp, "installed")
    end
    return checkstate(swstate)
end

local function init(step_api)
    state = step_api
    return "ok"
end


--public API.
M.checkpackage = checkpackage
M.init = init
M.checkswlist = checkswlist
--those one are public only for tests purpose...
M.cmp_version = cmp_version
M.checkmanifest = checkmanifest


return M;

