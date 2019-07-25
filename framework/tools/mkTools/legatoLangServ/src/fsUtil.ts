//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as fs from 'fs';
import * as path from 'path';



//--------------------------------------------------------------------------------------------------
/**
 * Check to see if this directory should be filtered from fs find results.  If symlinks are followed
 * outside of the base directories, then they are filtered out.
 *
 * We also ignore other standard directories.
 *
 * @param baseDir The directory the search is occurring under.
 * @param name The directory name to check.
 */
//--------------------------------------------------------------------------------------------------
export function ignoreDir(basePath: string, dirPath: string): boolean
{
     if (dirPath.startsWith(basePath) === false)
    {
        return true;
    }
    let name = path.basename(dirPath);

    return    (name === 'leaf-data')
           || (name === '.git')
           || (name.startsWith('_build_'));
}



//--------------------------------------------------------------------------------------------------
/**
 * Find all files with the given extension, recursively from the given root directory.
 *
 * @param baseDir Start the search in this dir.
 * @param extension The extension we are looking for, including the '.', pass * for all files.
 */
//--------------------------------------------------------------------------------------------------
export function recursiveFileFind(baseDir: string, extension: string): string[]
{
    let results: string[] = [];
    let contents = fs.readdirSync(baseDir);

    contents.forEach(
        function (item: string)
        {
            if (ignoreDir(baseDir, item))
            {
                return;
            }

            let fullPath = path.join(baseDir, item);
            let info = fs.statSync(fullPath);

            if (info.isDirectory())
            {
                results = results.concat(recursiveFileFind(fullPath, extension));
            }
            else if (   info.isFile()
                     || info.isSymbolicLink)
            {
                if (   (extension === '*')
                    || (path.extname(item) === extension))
                {
                    results.push(fullPath);
                }
            }
        });

    return results;
}



//--------------------------------------------------------------------------------------------------
/**
 * Find all subdirectories of the given base directory.
 *
 * @param baseDir The directory to search from.
 */
//--------------------------------------------------------------------------------------------------
export function recursiveDirFind(baseDir: string): string[]
{
    let results: string[] = [];
    let contents = fs.readdirSync(baseDir);

    contents.forEach(
        function (item: string)
        {
            if (ignoreDir(baseDir, item))
            {
                return;
            }

            let fullPath = path.join(baseDir, item);
            let info = fs.statSync(fullPath);

            if (info.isDirectory())
            {
                results.push(fullPath);
                results = results.concat(recursiveDirFind(fullPath));
            }
        });

    return results;
}
