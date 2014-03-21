-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--[[
script to create update package "Software Update Package" ready to be uploaded as *OMADM* package only on AirVantage Services Platform UI.

args: folder path to package, the content of the resulting update package must be directly in the specified folder

result: *in* the provided folder:
- the *_OMADM_pkg.zip file is the one to be uploaded in Services Platform UI for OMADM update job.
- the *_standalone_pkg.tar is the file to be hosted for update using M3DA Software Update Command
- the *_standalone_pkg.tar.md5 contains the md5 to provide as second param of M3DA Software Update Command

For the update content, read:
https://confluence.anyware-tech.com/display/PLT/Software+Update+Package
--]]

--to be able to run with copcall (needed to be used by update test suite
--or without copcall, when using createpkg.sh in std Lua VM.
local protectcall = copcall or pcall

local M={}

local function capture(cmd, raw)
    local f = assert(io.popen(cmd, 'r'))
    local s = assert(f:read('*a'))
    f:close()
    if raw then return s end
    s = string.gsub(s, '^%s+', '')
    s = string.gsub(s, '%s+$', '')
    s = string.gsub(s, '[\n\r]+', ' ')
    return s
end

local function cpkg(dirpath, outputdir)
    --readlink cleans dirpath and give absolute path
    dirpath = capture("readlink -e "..dirpath)
    outputdir = capture("readlink -e "..outputdir)

    assert(dirpath and dirpath ~= "" and outputdir and outputdir~="", "bad params")

    local pkgname
    local dir

    --readlink does keep trailing "/"
    --Keep only last folder part, ex useless/uselesstoo/tokeep
    pkgname = string.match(dirpath,".*/(.+)$")
    dirpath = dirpath.."/"
    outputdir = outputdir.."/"

    --Tar stuff
    local tarfilepath = outputdir..pkgname.."_standalone_pkg.tar"
    assert(0 == os.execute("tar -czvf "..tarfilepath.." -C "..dirpath.." . 2>&1 > /dev/null"))

    -- MD5 stuff
    local md5sum = capture("md5sum "..tarfilepath)
    -- remainder MD5 file
    local md5filepath = tarfilepath..".md5"
    local md5file,err = io.open(md5filepath, "w")
    md5file:write(md5sum)
    md5file:write("\n")
    md5file:close()
    -- get the MD5 hash
    md5sum = string.match(md5sum, "(%x*)%s")
    local res = {}
    for i=1,32,2 do
        table.insert(res, string.char(tonumber(string.sub(md5sum,i,i+1),16)))
    end
    md5sum = (table.concat(res))


    -- Concat MD5 and Tar
    local fotofilepath = outputdir..pkgname..".foto"

    local fotofile= assert(io.open(fotofilepath, "w"))
    assert(fotofile:write(md5sum))
    assert(fotofile:close())
    assert(0 == os.execute("cat "..tarfilepath.." >> "..fotofilepath))


    -- Zip the result
    local zipfilepath = outputdir..pkgname.."_OMADM_pkg.zip"

    assert(0 == os.execute("zip "..zipfilepath.." "..fotofilepath.." 2>&1 > /dev/null"))

    -- cleaning and moving result files
    assert(0 == os.execute("rm "..fotofilepath))

    return "ok", dirpath, outputdir
end


local function createpkg (...)
    local res= {protectcall(cpkg, ...)}
    if not res[1] then return nil, select(2, unpack(res))
    else return select(2, unpack(res)) end
end

M.createpkg = createpkg

return M



