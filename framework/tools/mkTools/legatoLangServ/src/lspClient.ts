
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



//--------------------------------------------------------------------------------------------------
/**
 * This class represents the client's build environment and active system/application model.
 */
//--------------------------------------------------------------------------------------------------
export class Profile
{
    /** The build environment we are to run/build under. */
    private envVars: NodeJS.ProcessEnv;

    /** The directories in the user's editing workspace. */
    public workspaceFolders: lsp.WorkspaceFolder[];

    /** The Json model as loaded from mkparse. */
    private jsonDocument: jdoc.Document;

    /** The active Legato definition file that provides the build model that we're working with. */
    //activeDefFile: string;

    /** A loaded version of that definition file. */
    public activeModel: model.System | model.Application;

    /** Watch the filesystem for any changes to any of the files that make up the active model. */
    //private fileWatchers: fs.FSWatcher[];
    private fileWatcher: fs_watcher.FSWatcher;

    /** Keep track of if the client is receiving model load updates. */
    private clientReceiveModelUpdates: boolean;

    private clientProps: ext.le_ExtensionClientCapabilities;

    /**
     * Allow for multiple file change events to come in quickly.  (They often come in multiples,
     * even for the same file.)  So, we simply wait for a given number of miliseconds without a
     * file change event before we attempt to reload the model. */
    private reloadTimer?: NodeJS.Timeout;

    private client: LspClient;

    /** Simply construct with default values. */
    public constructor(client: LspClient)
    {
        this.workspaceFolders = [];

        this.activeModel = undefined;
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
        return this.clientProps.env;
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

    /** Reload the active module from definition files. */
    public reloadActiveModel()
    {
        let watchPaths: string[] = [];

        this.clearFileWatch();

        this.jsonDocument = undefined;
        this.activeModel = undefined;

        if (   (this.activeDefFile !== undefined)
            && (!fs.existsSync(this.activeDefFile)))
        {
            console.log("No definition file has been set, can not load a model.");
            return;
        }

        let jsonDocument = tooling.mkInfo(this, this.activeDefFile);

        /*if (model.isApplicationDef(this.activeDefFile))
        {
        }
        else */ if (model.DefFile.isSystemDef(this.activeDefFile))
        {
            let systemInfo = loader.parseSystem(jsonDocument, this.activeDefFile);

            for (let def in jsonDocument.tokenMap)
            {
                watchPaths.push(def);

                if (   (model.DefFile.isSystemDef(def))
                    && (def !== this.activeDefFile))
                {
                    let newInfo = loader.parseSystem(jsonDocument, def);

                    function splice<T>(listA: T[], listB: T[])
                    {
                        listA.splice(listA.length - 1, 0, ...listB);
                    }

                    splice(systemInfo.searchPathSections, newInfo.searchPathSections);
                    splice(systemInfo.appSections, newInfo.appSections);
                    splice(systemInfo.commandSections, newInfo.commandSections);
                    splice(systemInfo.moduleSections, newInfo.moduleSections);
                }
            }

            this.jsonDocument = jsonDocument;
            this.activeModel = systemInfo;
        }

        this.watchFiles(watchPaths);
    }

    private notifyModelUpdate()
    {
        if (   (this.activeModel !== undefined)
            && (this.clientReceiveModelUpdates))
        {
            let flags: conversion.ConversionFlags = { supportsDefaultCollapsed: true };
            let logicalView = conversion.SystemAsSymbol(flags, this.activeModel as model.System);

            this.client.connection.sendNotification('le_UpdateLogicalView', logicalView);
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
                function (path, stats)
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
        let filePath = Uri.parse(uriStr).fsPath;
        return (this.jsonDocument.tokenMap[filePath] !== undefined);
    }

    /** Where is our current 'active' version of Legato located? */
    public get legatoRoot(): string
    {
        return path.normalize(this.getEnvString('LEGATO_ROOT'));
    }

    /** Path to the legato bin directory. */
    public get legatoBin(): string
    {
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
    public get availableSystems(): string[]
    {
        return this.fileList(
                this.workspaceFolders.map(
                    (rootFolder: lsp.WorkspaceFolder): string =>
                    {
                        return lsp.Files.uriToFilePath(rootFolder.uri);
                    }),
                '.sdef');
    }

    public get availableApps(): string[]
    {
        let searchPaths = this.jsonDocument.buildParams.search.appDirs;
        let def = model.DefType.applicationDef;

        return this.fileList(searchPaths, def);
    }

//    public get availableComponents(): string[]
//    {
//        let searchPaths = this.jsonDocument.buildParams.search.componentDirs;
//        let def = model.DefType.componentDef;
//
//        return this.fileList(searchPaths, def);
//    }

//    public get availableInterfaces(): string[]
//    {
//        return this.fileList(apiFolders, '.api');
//    }

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
        let foundFiles: string[] = [];

        for (let directory of directories)
        {
            fs.readdirSync(directory).forEach((found: string) =>
                {
                    found = path.resolve(found);

                    if (path.extname(found) == extension)
                    {
                        foundFiles.push(found);
                    }
                });
        }

        return foundFiles.sort();
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

    /** Shortcut to log messages in the client console. */
    log(message: string)
    {
        this.connection.console.log(message);
    }
}
