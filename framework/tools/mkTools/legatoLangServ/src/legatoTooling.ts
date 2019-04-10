
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as proc from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';

import * as lspCli from './lspClient';
import * as jdoc from './model/jsonDocument';



//--------------------------------------------------------------------------------------------------
/**
 * Internal function for executing our command line tooling and returning it's output as a string.
 */
//--------------------------------------------------------------------------------------------------
function exec(workDir: string, env: NodeJS.ProcessEnv, command: string,
              ...args: string[]): string
{
    // Check to see if we're running under Windows.  If we are, then we need to take care of running
    // our child processes under WSL.
    if (os.type() == 'Windows_NT')
    {
        // Prepend the wsl command onto the command line to request the WSL subsystem to start our
        // new process.
        args.unshift(command);
        command = 'wsl.exe';
    }
    // TODO: Maybe use async/await, or proc.spawn.
    let process = proc.spawnSync(command, args,
        {
            encoding: 'utf8',
            stdio: 'pipe',
            cwd: workDir,
            env: env
        });

    return process.output.join('');
}



//--------------------------------------------------------------------------------------------------
/**
 *  Attempt to find the given executable within the file path set.
 *
 * @param paths  The set of paths to search within.
 * @param executable  The executable we are looking for.
 */
//--------------------------------------------------------------------------------------------------
function findInPath(paths: string[], executable: string): string
{
    for (let pathStr of paths)
    {
        let fullPathStr = path.join(pathStr, executable);

        if (fs.existsSync(fullPathStr))
        {
//            let permissions =
//                perm.modeToPermissions(fs.statSync(fullPathStr).mode);
//
//            if (   (permissions.execute.owner)
//                || (permissions.execute.group)
//                || (permissions.execute.others))
            {
                return fullPathStr;
            }
        }
    }

    return '';
}



//--------------------------------------------------------------------------------------------------
/**
 * Run the mkinfo command line tool against a given definition file and return a JSON version of
 * that build model.
 *
 * @param profile  The profile to execute under.
 * @param defFilePath  Path to the Legato definition file to model..
 */
//--------------------------------------------------------------------------------------------------
export function mkInfo(profile: lspCli.Profile, defFilePath: string): jdoc.Document
{
    let output = exec(path.dirname(defFilePath),
                      profile.environment,
                      findInPath(profile.path, 'mkparse'),
                      '-t', profile.legatoTarget,
                      defFilePath);

    return JSON.parse(output);
}
