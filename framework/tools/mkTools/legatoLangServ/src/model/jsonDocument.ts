
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as fs from 'fs';



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface SearchPaths
{
    interfaceDirs: string[];
    moduleDirs: string[];
    appDirs: string[];
    componentDirs: string[];
    sourceDirs: string[];
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface CompilerFlags
{
    cFlags: string;
    cxxFlags: string;
    ldFlags: string;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface CompilerInfo
{
    flags: CompilerFlags;
    crossToolPaths: string[];
    cCompilerPath: string;
    cxxCompilerPath: string;
    toolChainDir: string;
    toolChainPrefix: string;
    sysrootDir: string;
    stripPath: string;
    objcopyPath: string;
    readelfPath: string;
    compilerCachePath: string;
    linkerPath: string;
    archiverPath: string;
    assemblerPath: string;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface LegatoDirs
{
    libOutputDir: string;
    outputDir: string;
    workingDir: string;
    debugDir: string;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface SigningInfo
{
    privKey: string;
    pubCert: string;
    signPkg: string;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface BuildParams
{
    beVerbose: boolean;
    jobCount: number;
    target: string;
    codeGenOnly: boolean;
    isStandAloneComp: boolean;
    binPack: boolean;
    noPie: boolean;

    args: string[];

    search: SearchPaths;

    compiler: CompilerInfo;

    directories: LegatoDirs;
    signing: SigningInfo;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export enum TokenType
{
    EndOfFile = "end-of-file",
    OpenCurly = "{",
    CloseCurly = "}",
    OpenParenthesis = "(",
    CloseParenthesis = ")",
    Colon = ":",
    Equals = "=",
    Dot = ".",
    Star = "*",
    Arrow = "->",
    Whitespace = "whitespace",
    Comment = "comment",
    FilePermissions = "file permissions",
    ServerIpcOption = "server-side IPC option",
    ClientIpcOption = "client-side IPC option",
    Arg = "argument",
    FilePath = "file path",
    FileName = "file name",
    Name = "name",
    DottedName = "dotted name",
    GroupName = "group name",
    IpcAgent = "IPC agent",
    Integer = "integer",
    SignedInteger = "signed integer",
    Boolean = "Boolean value",
    Float = "floating point number",
    String = "string",
    Md5Hash = "MD5 hash",
    Directive = "directive",
    OptionalOpenSquare = "optional",
    ProvideHeaderOption = "provide-header"
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface Token
{
    column?: number;
    line?: number;
    text?: string;
    type: TokenType;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface TokenMap
{
    [fileName: string]: Token[];
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface Document
{
    version: string;
    buildParams: BuildParams;
    tokenMap: TokenMap;
}
