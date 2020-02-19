
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as lsp from 'vscode-languageserver';
import Uri from 'vscode-uri';
import * as path from 'path';
import * as fs from 'fs';
import * as fs_watcher from 'chokidar';

import * as model from './model/annotatedModel';
import * as jdoc from './model/jsonDocument';
import * as loader from './model/loader';
import * as tooling from './legatoTooling';
import * as conversion from './vscTypeConvert';
import * as ext from './lspExtensionDefs';
import * as fsUtil from './fsUtil';



//--------------------------------------------------------------------------------------------------
/**
 * This class represents the client's build environment and active system/application model.
 */
//--------------------------------------------------------------------------------------------------
export class Profile
{
    /** The directories in the user's editing workspace. */
    public workspaceFolders: lsp.WorkspaceFolder[];

    /** The Json model as loaded from mkparse. */
    private jsonDocument: jdoc.Document;

    /** A loaded version of that definition file. */
    public activeModel: model.System | model.Application;

    /** The error message occurs when the language server fails to parse Legato system file. */
    private errorMessage: string;

    /** Watch the filesystem for any changes to any of the files that make up the active model. */
    private fileWatcher: fs_watcher.FSWatcher;

    /** Keep track of if the client is receiving model load updates. */
    private clientReceiveModelUpdates: boolean;

    /** Properties of the attached client. */
    private clientProps: ext.le_ExtensionClientCapabilities;

    /**
     * Allow for multiple file change events to come in quickly.  (They often come in multiples,
     * even for the same file.)  So, we simply wait for a given number of milliseconds without a
     * file change event before we attempt to reload the model. */
    private reloadTimer?: NodeJS.Timeout;

    /** The connection of this language server to the attached client. */
    private client: LspClient;

    /** Simply construct with default values. */
    public constructor(client: LspClient)
    {
        this.workspaceFolders = [];

        this.activeModel = undefined;
        this.errorMessage = "";
        this.fileWatcher = undefined;
        this.clientReceiveModelUpdates = false;

        this.clientProps =
            {
                env: {},
                supportsModelUpdates: false
            };

        this.reloadTimer = undefined;
        this.client = client;
    }

    public set clientProperties(newProps: ext.le_ExtensionClientCapabilities)
    {
        this.clientProps = newProps;
        this.environmentChanged();
    }

    public get clientProperties(): ext.le_ExtensionClientCapabilities
    {
        return this.clientProps;
    }

    /**
     * The environment our tooling is to run under.  Write to this value to force a re-evaluation
     * of the active model.
     * */
    public get environment(): NodeJS.ProcessEnv
    {
        if (this.jsonDocument !== undefined)
        {
            let modelEnv = this.jsonDocument.env;
            let clientEnv = this.clientProperties.env;

            Object.keys(clientEnv).forEach(
                function (name: string)
                {
                    modelEnv[name] = clientEnv[name];
                });

            return modelEnv;
        }

        return this.clientProperties.env;
    }

    public set environment(envVars: NodeJS.ProcessEnv)
    {
        this.clientProps.env = envVars;
        this.environmentChanged();
    }

    private environmentChanged()
    {
        if (this.activeDefFile != "")
        {
            this.reloadActiveModel();
            this.notifyModelUpdate();
        }
    }

    /**
     * Type guard to check if the argument is a string or if it's a json document.
     *
     * @param arg The argument to check.
     */
    private isDocument(arg: jdoc.Document | string): arg is jdoc.Document
    {
        return typeof arg !== 'string';
    }

    /** Reload the active module from definition files. */
    public reloadActiveModel()
    {
        let watchPaths: string[] = [];

        this.clearFileWatch();

        if (   (this.activeDefFile !== undefined)
            && (!fs.existsSync(this.activeDefFile)))
        {
            console.log("No definition file has been set, can not load a model.");
            return;
        }

        let mkResponse = tooling.mkInfo(this, this.activeDefFile);

        if (this.isDocument(mkResponse))
        {
            if (model.DefFile.isSystemDef(this.activeDefFile))
            {
                let isIncluded = false;
                let systemInfo = loader.parseSystem(mkResponse, this.activeDefFile, isIncluded);

                for (let def in mkResponse.tokenMap)
                {
                    isIncluded = true;
                    watchPaths.push(def);

                    if (   (model.DefFile.isSystemDef(def))
                        && (def !== this.activeDefFile))
                    {
                        let newInfo = loader.parseSystem(mkResponse, def, isIncluded);

                        systemInfo.searchPathSections =
                            systemInfo.searchPathSections.concat(newInfo.searchPathSections);

                        systemInfo.appSections =
                            systemInfo.appSections.concat(newInfo.appSections);

                        systemInfo.commandSections =
                            systemInfo.commandSections.concat(newInfo.commandSections);

                        systemInfo.moduleSections =
                            systemInfo.moduleSections.concat(newInfo.moduleSections);
                    }
                }

                this.jsonDocument = mkResponse;
                this.activeModel = systemInfo;
                this.errorMessage = "";
            }

            this.watchFiles(watchPaths);
        }
        else
        {
            this.activeModel = undefined;
            // Looks like we couldn't properly load the current model.  So search this directory for
            // all 'interesting' model files.
            console.log(`An error occurred during model load.`);
            console.log(mkResponse);
            console.log('Watching the filesystem for changes.');

            let rootDirList: string[] = this.workspaceFolders.map(
                (nextDir: lsp.WorkspaceFolder): string =>
                {
                    return lsp.Files.uriToFilePath(nextDir.uri)
                });

            let dirList: string[] = [];

            for (let nextDir of rootDirList)
            {
                console.log(`-- ${nextDir}`);
                let newDirs = fsUtil.recursiveDirFind(nextDir);
                dirList = dirList.concat(newDirs);
            }

            let fileList = this.getAvailableSystems(dirList)
                .concat(this.getAvailableApps(dirList))
                .concat(this.getAvailableComponents(dirList))
                .concat(this.getAvailableInterfaces(dirList));


            for (let filePath of fileList)
            {
                console.log(`## ${filePath}`);
            }

            this.watchFiles(fileList);

            // Attempt to fix up the fix up the model, which should trigger a reload event if
            // successful.

            if (this.attemptModelFixup(mkResponse, fileList))
            {
                this.reloadActiveModel();
            }
            else
            {
                // Adef file is not supported for Legato system view. So the warning message will not be displayed for it.
                if (this.activeDefFile.slice(-5) === ".sdef")
                {
                    this.client.connection.window.showWarningMessage("An error occurred when loading Legato system view: " + mkResponse);
                    this.errorMessage = mkResponse;
                }
            }
        }
    }

    private attemptModelFixup(mkResponse: string, fileList: string[]): boolean
    {
        interface Extractor
        {
            section: tooling.SearchPath;
            regex: RegExp;
            filterFunc: (extractedStr: string) => string;
        }

        let matchers: Extractor[] =
            [
                {
                    section: tooling.SearchPath.AppSearch,
                    regex: /Can\'t find definition file \((.*)\) or binary app/,
                    filterFunc: function (newName: string): string
                        {
                            console.log(`Test 2: ${newName}`);
                            return newName;
                        }
                },

                {
                    section: tooling.SearchPath.ComponentSearch,
                    regex: /Couldn\'t find component \'(.*)\'/,
                    filterFunc: function (newName: string): string
                        {
                            console.log(`Test 1: ${newName}`);
                            return path.join(newName, 'Component.cdef');
                        }
                },

                {
                    section: tooling.SearchPath.InterfaceSearch,
                    regex: /Couldn't find file \'(.*)\'/,
                    filterFunc: function (newName: string): string
                        {
                            let ext = path.extname(newName);

                            if (ext !== '.api')
                            {
                                return newName + '.api';
                            }

                            return newName;
                        }
                }
            ];

        let name: string = undefined;
        let section: tooling.SearchPath = undefined;

        for (let matcher of matchers)
        {
            let result = mkResponse.match(matcher.regex);

            if (result !== null)
            {
                name = matcher.filterFunc(result[1]);
                section = matcher.section;

                console.log(`### Matching ${name}: ${section}`);
                break;
            }
        }

        if (name === undefined)
        {
            return false;
        }

        let foundDir: string = undefined;

        for (let filePath of fileList)
        {
            console.log(`### ${filePath}`);
            if (filePath.endsWith(name))
            {
                console.log('### FOUND');
                foundDir = path.dirname(filePath);

                if (section === tooling.SearchPath.ComponentSearch)
                {
                    foundDir = path.dirname(foundDir);
                }

                break;
            }
        }

        console.log(`### Add search path ${section} --> ${foundDir}`);

        if (foundDir === undefined)
        {
            return false;
        }

        if (tooling.mkEditAddSearchPath(this, section, foundDir))
        {
            console.log('Error adding the search path!!');
        }
        return true;
    }

    private notifyModelUpdate()
    {
        if (this.clientReceiveModelUpdates)
        {
            if (this.activeModel !== undefined)
            {
                let logicalView = conversion.SystemAsSymbol(this.activeModel as model.System);

                this.client.connection.sendNotification('le_UpdateLogicalView', logicalView);
            }

            this.client.connection.sendNotification('le_SendErrorMessage', this.errorMessage);
        }
    }

    private watchFiles(watchPaths: string[])
    {
        this.clearModifyTimeout();

        if (watchPaths.length > 0)
        {
            let theThis = this;

            this.fileWatcher = fs_watcher.watch(watchPaths);

            this.fileWatcher.on('change',
                function (_path, _stats)
                {
                    theThis.clearModifyTimeout();
                    theThis.reloadTimer = setTimeout(
                        function ()
                        {
                            theThis.reloadActiveModel();
                            theThis.notifyModelUpdate();
                            theThis.reloadTimer = undefined;
                        },
                        400);
                });
        }
    }

    private clearFileWatch()
    {
        if (this.fileWatcher !== undefined)
        {
            this.fileWatcher.close();
            this.fileWatcher = undefined;
        }
    }

    private clearModifyTimeout()
    {
        if (this.reloadTimer !== undefined)
        {
            clearTimeout(this.reloadTimer);
            this.reloadTimer = undefined;
        }
    }

    /** Used to set wether or not the client is receiving update events. */
    public set receiveModelUpdates(newValue: boolean)
    {
        this.clientReceiveModelUpdates = newValue;
        this.notifyModelUpdate();
    }

    public get receiveModelUpdates(): boolean
    {
        return this.clientReceiveModelUpdates;
    }

    /**
     * Check the given URI string and see if it is a definition file that has contributed to the
     * currently loaded active Legato model.
     *
     * @param uriStr The URI to examine.
     */
    public uriInActiveModel(uriStr: string): boolean
    {
        if (this.jsonDocument === undefined)
        {
            return false;
        }

        let filePath = path.normalize(Uri.parse(uriStr).fsPath);
        return (this.jsonDocument.tokenMap[filePath] !== undefined);
    }

    public getDocTokens(uriStr: string): jdoc.Token[] | undefined
    {
        let filePath = path.normalize(Uri.parse(uriStr).fsPath);
        return this.jsonDocument.tokenMap[filePath];
    }

    /** Where is our current 'active' version of Legato located? */
    public get legatoRoot(): string
    {
        return path.normalize(this.getEnvString('LEGATO_ROOT'));
    }

    /** Path to the legato bin directory. */
    public get legatoBin(): string
    {
        console.log('Test 2');
        return path.join(this.legatoRoot, 'bin');
    }

    /** The name of the device target we are working with. */
    public get legatoTarget(): model.LegatoTarget
    {
        return this.getEnvString('LEGATO_TARGET') as model.LegatoTarget;
    }

    public get path(): string[]
    {
        return this.getEnvString('PATH').split(':');
    }

    /** What definition file is our model built off of? */
    public get activeDefFile(): string
    {
        return path.normalize(this.getEnvString('LEGATO_DEF_FILE'));
    }

    /** List of available system definition files. */
    public getAvailableSystems(additionalDirs: string[] = []): string[]
    {
        let searchDirs = this.workspaceFolders.map(
            (nextRootDir: lsp.WorkspaceFolder): string =>
            {
                return lsp.Files.uriToFilePath(nextRootDir.uri);
            })
            .concat(additionalDirs);

        return this.fileList(searchDirs, '.sdef');
    }

    public getAvailableApps(additionalDirs: string[] = []): string[]
    {
        let searchPaths =
            ((this.jsonDocument !== undefined) ? this.jsonDocument.buildParams.search.appDirs : [])
            .concat(additionalDirs);

        return this.fileList(searchPaths, model.DefType.applicationDef)
            .concat(this.fileList(searchPaths, '.app'));
    }

    public getAvailableComponents(additionalDirs: string[] = []): string[]
    {
        let searchPaths =
            (  (this.jsonDocument !== undefined)
             ? this.jsonDocument.buildParams.search.componentDirs : [])
            .concat(additionalDirs);

        return this.fileList(searchPaths, model.DefType.componentDef);
    }

    public getAvailableInterfaces(additionalDirs: string[] = []): string[]
    {
        let searchPaths =
            ( (this.jsonDocument !== undefined)
             ? this.jsonDocument.buildParams.search.interfaceDirs : [])
            .concat(additionalDirs);

        return this.fileList(searchPaths, '.api');
    }

    /**
     * Internal function that will read a value from the environment, if the value is undefined an
     * empty string is returned.
     */
    private getEnvString(name: string): string
    {
        let resultStr: string | undefined = this.clientProps.env[name];

        if (resultStr === undefined)
        {
            return '';
        }

        return resultStr;
    }

    /**
     * Generate a list of files of a given extension.
     *
     * @param directories A list of directories to search under.  The resulting list of files can
     *                    include files from all of these directories.
     * @param extension We are looking for files with this extension.
     */
    private fileList(directories: string[], extension: string): string[]
    {
        function getFiles(directory: string, extension: string, foundFiles: string[])
        {
            let items = fs.readdirSync(directory);

            items.forEach(
                function (nextItem: string)
                {
                    if (   (nextItem !== '.')
                        && (nextItem !== '..'))
                    {
                        let fullPath = path.join(directory, nextItem);

                        if (fs.statSync(fullPath).isDirectory())
                        {
                            if (!fsUtil.ignoreDir(directory, fullPath))
                            {
                                getFiles(fullPath, extension, foundFiles);
                            }
                            else
                            {
                                console.log('  Ignored.');
                            }
                        }
                        else if (path.extname(fullPath) === extension)
                        {
                            foundFiles.push(fullPath);
                        }
                    }
                });
        }

        let fileList: string[] = [];

        for (let directory of directories)
        {
            if (!fs.existsSync(directory))
            {
                continue;
            }

            getFiles(directory, extension, fileList);
        }

        return fileList.sort();
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Flags for the features that the client has informed that they support.
 */
//--------------------------------------------------------------------------------------------------
interface ClientCapabilities
{
    hasWorkspaces: boolean;
    hasConfiguration: boolean;
    hasHierarchicalDocumentSymbol: boolean;
}



//--------------------------------------------------------------------------------------------------
/**
 * Here we represent the client of our active LSP connection.
 */
//--------------------------------------------------------------------------------------------------
export class LspClient
{
    /** The actual connection itself. */
    connection: lsp.Connection<lsp._, lsp._, lsp._, lsp._, lsp._, lsp._>;

    /** What is this client capable of? */
    capabilities: ClientCapabilities;

    /** The active build environment that the user is working against. */
    profile: Profile;


    /**
     * Simply construct with default values, we expect things to be properly setup when the client
     * callbacks start happening.
     */
    constructor()
    {
        this.connection = lsp.createConnection(lsp.ProposedFeatures.all),
        this.capabilities =
            {
                hasWorkspaces: false,
                hasConfiguration: false,
                hasHierarchicalDocumentSymbol: false
            };

        this.profile = new Profile(this);
    }
}
