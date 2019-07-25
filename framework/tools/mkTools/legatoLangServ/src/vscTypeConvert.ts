
//--------------------------------------------------------------------------------------------------
/**
 * Convert internal language server model objects into something that the language client expects.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as lsp from 'vscode-languageserver';
import Uri from 'vscode-uri';
import * as fsUtil from './fsUtil';
import * as path from 'path';

import * as model from './model/annotatedModel';
import * as loader from './model/loader';
import * as ext from './lspExtensionDefs';
import * as lspCli from './lspClient';
import * as nav from './model/dataNavigation';
import * as jdoc from './model/jsonDocument';



//--------------------------------------------------------------------------------------------------
/**
 * Extract the definition name from it's path.
 *
 * For most def files, the file name is the name of the definition.  With components, the definition
 * name is the enclosing directory name.  So, extract the definition name based on the definition
 * file type.
 *
 * @param defPath The path we're extracting a name from.
 */
//--------------------------------------------------------------------------------------------------
function GetLegatoDefName(defPath: string): string
{
    let name: string;

    if (path.extname(defPath) === model.DefType.componentDef)
    {
        name = path.basename(path.dirname(defPath));
    }
    else
    {
        name = path.basename(defPath, path.extname(defPath));
    }

    return name;
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert a list of Legato file paths into symbol objects for the client plug-in.
 *
 * @param pathList The list of paths to convert.
 */
//--------------------------------------------------------------------------------------------------
export function FilePathsToSymbolInformation(pathList: string[]): lsp.SymbolInformation[]
{
    return pathList.map(
        (defPath: string): lsp.SymbolInformation =>
        {
            return {
                    "name": GetLegatoDefName(defPath),
                    "kind": lsp.SymbolKind.File,

                    "location": {
                            "uri": Uri.file(defPath).toString(),
                            "range": {
                                    "start": { "line": 0, "character": 0 },
                                    "end":   { "line": 0, "character": 0 }
                                }
                        },

                };
        });
}



//--------------------------------------------------------------------------------------------------
/**
 * A set of flags to indicate what features to enable during object conversion.
 */
//--------------------------------------------------------------------------------------------------
export interface ConversionFlags
{
    /**
     * If true, then generate fields that indicate specific entries should be collapsed by default
     * upon first viewing,.
     */
    supportsDefaultCollapsed: boolean;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class DefinitionObject implements ext.le_DefinitionObject
{
    public name: string;
    public kind: ext.le_DefinitionObjectType;
    public detail?: string;

    public defaultCollapsed: boolean;

    public defPath: string;
    public children?: ext.le_DefinitionObject[];

    public range: lsp.Range;
    public selectionRange: lsp.Range;

    public constructor(type: ext.le_DefinitionObjectType, name: string, path: string,
                       start: loader.Location, end: loader.Location,
                       defaultCollapsed: boolean = undefined)
    {
        let newRange = lsp.Range.create(start.line - 1, start.column, end.line - 1, end.column + 1);

        this.name = name;
        this.kind = type as lsp.SymbolKind;
        this.defPath = Uri.file(path).toString();

        this.range = newRange;
        this.selectionRange = newRange;

        if (defaultCollapsed === undefined)
        {
            this.defaultCollapsed = true;
        }
        else
        {
            this.defaultCollapsed = defaultCollapsed;
        }

        this.children = [];
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert component source file names into symbol objects for the language client.
 */
//--------------------------------------------------------------------------------------------------
function SourcesAsSymbols(component: model.Component): ext.le_DefinitionObject[]
{
    let sources: ext.le_DefinitionObject[] = [];

    for (let section of component.sourcesSections)
    {
        for (let source of section.sources)
        {
            let location = new loader.Location(source.path, 0, 0);
            let sourceRef = new DefinitionObject(ext.le_DefinitionObjectType.Source,
                                                 path.basename(source.name),
                                                 source.path,
                                                 location,
                                                 location);

            sourceRef.children = undefined;

            sources.push(sourceRef);
        }
    }

    return sources;
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert component names into source symbols for the language client.  Then append any source
 * files as child symbols.
 */
//--------------------------------------------------------------------------------------------------
function ComponentsAsSymbols(componentSections: model.ComponentSection[]): DefinitionObject[]
{
    let componentList: DefinitionObject[] = [];

    for (let componentSection of componentSections)
    {
        for (let component of componentSection.components)
        {
            let location = new loader.Location(component.target.path, 0, 0);
            let comp = new DefinitionObject(ext.le_DefinitionObjectType.ComponentRef,
                                            component.name,
                                            component.target.path,
                                            location,
                                            location,
                                            true);
            comp.children = SourcesAsSymbols(component.target);

            if (   (comp.children !== undefined)
                && (comp.children.length === 0))
            {
                comp.children = undefined;
            }

            componentList.push(comp);
        }
    }

    return componentList;
}



function AddExternalInterfaces
(
    appSym: DefinitionObject,
    appPath: string,
    clients: jdoc.ClientInterface[],
    servers: jdoc.ServerInterface[]
)
{
    let start = new loader.Location(appPath, 0, 0);

    let extSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                      "Interface",
                                      appPath,
                                      start,
                                      start,
                                      true);

    let clientSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                         "Clients",
                                         appPath,
                                         start,
                                         start,
                                         true);

    let serverSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                         "Services",
                                         appPath,
                                         start,
                                         start,
                                         true);

    for (let clientApi of clients)
    {
        let apiName = `${clientApi.name} (${clientApi.interface.name}.api)`;

        let ifSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
            apiName,
            clientApi.interface.path,
            start,
            start,
            true);

        clientSym.children.push(ifSym);
    }

    for (let serverApi of servers)
    {
        let apiName = `${serverApi.name} (${serverApi.interface.name}.api)`;

        let ifSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
            apiName,
            serverApi.interface.path,
            start,
            start,
            true);

        serverSym.children.push(ifSym);
    }

    if (clientSym.children.length > 0)
    {
        extSym.children.push(clientSym);
    }

    if (serverSym.children.length > 0)
    {
        extSym.children.push(serverSym);
    }

    if (extSym.children.length > 0)
    {
        appSym.children.push(extSym);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert a collection of application references into symbol objects.  The apps are grouped by
 * which system definition they came from.
 */
//--------------------------------------------------------------------------------------------------
function AppsAsSymbols(systemPath: string,
                       appSections: model.SystemAppsSection[]): DefinitionObject[]
{
    // Keep track of the app subsections that needed to be created.
    let sectionMap: { [index: string]: DefinitionObject } = {};
    let mainSection: DefinitionObject = undefined;

    // Go through all of the system's subsections, and convert them over to the symbol model
    for (let appSection of appSections)
    {
        let newName = 'apps [' + path.basename(appSection.location.file) + ']';
        let section = sectionMap[newName];

        if (section === undefined)
        {
            let defaultCollapsed: boolean = false;

            if (appSection.location.file !== systemPath)
            {
                defaultCollapsed = true;
            }

            section = new DefinitionObject(ext.le_DefinitionObjectType.DefSection,
                                           newName,
                                           appSection.location.file,
                                           appSection.location,
                                           appSection.location,
                                           defaultCollapsed);

            sectionMap[newName] = section;

            if (section.defaultCollapsed === false)
            {
                mainSection = section;
            }
        }

        for (let appRef of appSection.apps)
        {
            if (appRef.target === undefined)
            {
                console.log('Skipping: ' + appRef.name);
                continue;
            }

            let start = new loader.Location(appRef.refString, 0, 0);
            let sym = new DefinitionObject(ext.le_DefinitionObjectType.ApplicationRef,
                                           path.basename(appRef.name, path.extname(appRef.name)),
                                           appRef.target.path,
                                           start,
                                           start);

            if (appRef.target !== undefined)
            {
                console.log(`## Application ${appRef.name}: isIncluded: ${appRef.target.isIncluded}.`);

                let clients: jdoc.ClientInterface[] = [];
                let servers: jdoc.ServerInterface[] = [];

                console.log(`interfaces:\n${appRef.target.interfaces.extern}`);

                for (let x in appRef.target.interfaces.extern)
                {
                    console.log(`## ${x}`);
                }

                if (appRef.target.isIncluded === false)
                {
                    let components = ComponentsAsSymbols(appRef.target.componentSections);

                    if (components.length > 0)
                    {
                        sym.children = sym.children.concat(components);
                    }

                    clients = appRef.target.interfaces.extern.clients;
                    console.log(`Clinets:\n${clients}`);
                }

                servers = appRef.target.interfaces.extern.servers;
                console.log(`Servers:\n${servers}`);

                AddExternalInterfaces(sym, appRef.target.path, clients, servers);
            }

            section.children.push(sym);
        }
    }

    let includedApps = Object.keys(sectionMap).map(
        (key): DefinitionObject =>
        {
            return sectionMap[key];
        })
        .filter(
            (defObject: DefinitionObject): boolean =>
            {
                // Filter out off if the includes with children.  We also filter out the main def
                // file as well.
                if (   (defObject.children.length === 0)
                    || (defObject.defaultCollapsed === false))
                {
                    return false;
                }

                return true;
            });

    let incSection = new DefinitionObject(ext.le_DefinitionObjectType.IncDefs,
                                         'includes',
                                         systemPath,
                                         new loader.Location(),
                                         new loader.Location(),
                                         true);

    incSection.children = includedApps;

    return [ mainSection, incSection ];
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct a symbol object from a Legato logical system object.  The constituent pieces of the
 * logical model will be represented by this object's children collection.
 */
//--------------------------------------------------------------------------------------------------
export function SystemAsSymbol(system: model.System): DefinitionObject
{
    let start = new loader.Location(system.path, 0, 0);
    let symbol = new DefinitionObject(ext.le_DefinitionObjectType.DefinitionFile,
                                      system.name,
                                      system.path,
                                      start,
                                      start,
                                      false);

    symbol.children = AppsAsSymbols(system.path, system.appSections);

    return symbol;
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct an environment that is local to a given directory.
 *
 * @param localDir Include this directory in the user environment.
 */
//--------------------------------------------------------------------------------------------------
export function getClientLocalEnvironment
(
    client: lspCli.LspClient,
    localDir: string
)
//--------------------------------------------------------------------------------------------------
: NodeJS.ProcessEnv
//--------------------------------------------------------------------------------------------------
{
    let env = client.profile.environment;
    env['CURDIR'] = localDir;
    return env;
}



//--------------------------------------------------------------------------------------------------
/**
 * Turn the workspace's environment variables into language server completion items.
 *
 * @param env The environment we're walking to create completions.
 */
//--------------------------------------------------------------------------------------------------
export function envVarsToCompletion(env: NodeJS.ProcessEnv): lsp.CompletionList
{
    function createItem(name: string, trueName: string): lsp.CompletionItem
    {
        let newItem = lsp.CompletionItem.create(name);

        newItem.documentation = 'Build environment variable.';
        newItem.filterText = trueName;
        newItem.sortText = trueName;

        return newItem;
    }

    let list: lsp.CompletionItem[] = [];

    for (let variable in env)
    {
        list.push(createItem(variable, variable));
        list.push(createItem('{' + variable + '}', variable));
    }

    return lsp.CompletionList.create(list);
}



//--------------------------------------------------------------------------------------------------
/**
 * CHeck the section name, and if it has a specific file extension associated with it's items,
 * return that extension.  Otherwise return undefined.
 *
 * @param name The section name to check.
 */
//--------------------------------------------------------------------------------------------------
function sectionNameToExtension(name: string): string | undefined
{
    let result: string = undefined;

    switch (name)
    {
        case 'components':    result = '.cdef'; break;
        case 'apps':          result = '.adef'; break;
        case 'kernelModules': result = '.mdef'; break;
        case 'api':           result = '.api';  break;
        case 'sources':       result = '.c';    break;
        case 'file':          result = '*';     break;
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Check the given section name and see if it is supposed to contain directory paths.
 * @param name The section name to check.
 */
//--------------------------------------------------------------------------------------------------
function sectionHasDirectoryItems(name: string): boolean
{
    let result = false;

    switch (name)
    {
        case 'interfaceSearch':
        case 'appSearch':
        case 'componentSearch':
        case 'moduleSearch':
        case 'dir':
            result = true;
            break;
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Append completion items to the results collection based on the type of section that the user is
 * editing.
 *
 * @param results Append the completion items to this collection.
 * @param section Determine the type of completion required from the section type.
 * @param workspace The root directory we're working out of.
 */
//--------------------------------------------------------------------------------------------------
export function appendSectionCompletions
(
    results: lsp.CompletionList,
    section: nav.Section,
    workspace: string
)
//--------------------------------------------------------------------------------------------------
{
    function appendCompletionStrings(regex: RegExp, results: lsp.CompletionList, strings: string[])
    {
        let newItems = strings.map(
            function (item: string): lsp.CompletionItem
            {
                return lsp.CompletionItem.create('.' + item.replace(regex, ''));
            });

        results.items = results.items.concat(newItems);
    }

    console.log('Completion Test: ' + section.name);

    let regex = new RegExp("^" + workspace);
    let extension = sectionNameToExtension(section.name);

    if (extension !== undefined)
    {
        console.log('File completion: ' + section.name + ': ' + extension);
        appendCompletionStrings(regex, results, fsUtil.recursiveFileFind(workspace, extension));
    }
    else if (sectionHasDirectoryItems(section.name))
    {
        appendCompletionStrings(regex, results, fsUtil.recursiveDirFind(workspace));
    }
}
