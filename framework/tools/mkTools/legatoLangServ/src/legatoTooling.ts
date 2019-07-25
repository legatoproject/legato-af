
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
              ...args: string[]): [ number, string ]
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

    let output = (process.output !== null) ? process.output.join('') : '';

    if (process.status !== null)
    {
        return [ process.status, output ];
    }

    return [ 0, output ];
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
export function mkInfo(profile: lspCli.Profile, defFilePath: string): jdoc.Document | string
{
    let result: jdoc.Document | string = undefined;
    let [ , output ] = exec(path.dirname(defFilePath),
                            profile.environment,
                            findInPath(profile.path, 'mkparse'),
                            '-t', profile.legatoTarget,
                            defFilePath);

    try
    {
        result = JSON.parse(output);
    }
    catch (e)
    {
        result = output;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of the types of search paths mkedit can update.
 */
//--------------------------------------------------------------------------------------------------
export enum SearchPath
{
    AppSearch = 'appSearch',
    ComponentSearch = 'componentSearch',
    InterfaceSearch = 'interfaceSearch',
    ModuleSearch = 'moduleSearch'
}


//--------------------------------------------------------------------------------------------------
/**
 * Call on mkEdit to modify the search paths in the current .sdef.
 *
 * @param profile Information about the active LSP client.
 * @param pathType What kind of search path are we adding?
 * @param newPath THe path to add.
 */
//--------------------------------------------------------------------------------------------------
export function mkEditAddSearchPath(profile: lspCli.Profile,
                                    pathType: SearchPath,
                                    newPath: string): boolean
{
    let [ code, ] = exec(path.dirname(profile.activeDefFile),
                         profile.environment,
                         findInPath(profile.path, 'mkedit'),
                         'add',
                         pathType.toString(),
                         newPath);

    return code === 0;
}
