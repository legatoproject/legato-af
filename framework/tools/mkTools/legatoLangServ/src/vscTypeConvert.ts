
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
 * The system view in the IDE is a tree of these definition objects.
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



//--------------------------------------------------------------------------------------------------
/**
 * Extract an app's client APIs from it's external interface.
 *
 * @param appRef The application we generating for.
 */
//--------------------------------------------------------------------------------------------------
function InterfaceClientsAsSymbols(appRef: model.ApplicationRef): DefinitionObject[]
{
    let start = new loader.Location(appRef.target.path, 0, 0);
    let clientSymbols: DefinitionObject[] = [];

    for (let clientApi of appRef.target.interfaces.extern.clients)
    {
        let apiName = `${clientApi.name} (${clientApi.interface.name}.api)`;

        let ifSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
            apiName,
            clientApi.interface.path,
            start,
            start,
            true);

        clientSymbols.push(ifSym);
    }

    return clientSymbols;
}



//--------------------------------------------------------------------------------------------------
/**
 * Extract an app's service APIs from it's external interface.
 *
 * @param appRef The application we generating for.
 */
//--------------------------------------------------------------------------------------------------
function InterfaceServersAsSymbols(appRef: model.ApplicationRef): DefinitionObject[]
{
    let start = new loader.Location(appRef.target.path, 0, 0);
    let serverSymbols: DefinitionObject[] = [];

    for (let serverApi of appRef.target.interfaces.extern.servers)
    {
        let apiName = `${serverApi.name} (${serverApi.interface.name}.api)`;
        let ifSym = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                         apiName,
                                         serverApi.interface.path,
                                         start,
                                         start,
                                         true);

        serverSymbols.push(ifSym);
    }

    return serverSymbols;
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct a view of an app's external interfaces.
 *
 * @param appRef The application we generating for.
 */
//--------------------------------------------------------------------------------------------------
function ExternalInterfaces
(
    appRef: model.ApplicationRef
)
//--------------------------------------------------------------------------------------------------
: DefinitionObject
//--------------------------------------------------------------------------------------------------
{
    let appPath = appRef.target.path;
    let start = new loader.Location(appPath, 0, 0);


    // Collect all the app's external client interfaces.
    let clientCollection = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                               "Clients",
                                               appPath,
                                               start,
                                               start,
                                               true);

    clientCollection.children = InterfaceClientsAsSymbols(appRef);


    // Now collect all the server interfaces.
    let serverCollection = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                               "Services",
                                               appPath,
                                               start,
                                               start,
                                               true);

    serverCollection.children = InterfaceServersAsSymbols(appRef);


    // Bundle all of these interfaces under app's external interface.
    let interfaceSymbol = new DefinitionObject(ext.le_DefinitionObjectType.ExternalIf,
                                              "Interface",
                                              appPath,
                                              start,
                                              start,
                                              true);

    interfaceSymbol.children = [];

    if (clientCollection.children.length > 0)
    {
        interfaceSymbol.children.push(clientCollection);
    }

    if (serverCollection.children.length > 0)
    {
        interfaceSymbol.children.push(serverCollection);
    }

    return interfaceSymbol;
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert an application ref into a system view symbol.
 */
//--------------------------------------------------------------------------------------------------
function AppAsSymbol(appRef: model.ApplicationRef): DefinitionObject
{
    let sym: DefinitionObject = undefined;

    // Extract and provide any extra information we can find for this app.
    if (appRef.target.isIncluded === false)
    {
        let start = new loader.Location(appRef.refString, 0, 0);
        sym = new DefinitionObject(ext.le_DefinitionObjectType.ApplicationRef,
                                   path.basename(appRef.name, path.extname(appRef.name)),
                                   appRef.target.path,
                                   start,
                                   start);

        let externalInterfaces = ExternalInterfaces(appRef);
        let components = ComponentsAsSymbols(appRef.target.componentSections);

        if (externalInterfaces.children.length > 0)
        {
            sym.children.push(externalInterfaces);
        }

        if (components.length > 0)
        {
            sym.children = sym.children.concat(components);
        }
    }
    else
    {
        let start = new loader.Location(appRef.location.file, appRef.location.line, -1);
        sym = new DefinitionObject(ext.le_DefinitionObjectType.ApplicationRef,
                                   path.basename(appRef.name,
                                                 path.extname(appRef.name)),
                                   appRef.location.file,
                                   start,
                                   start);

        // If this is an included app, we simplify what we add to the view.  In this case we just
        // add the external interfaces that the included app provides for binding.
        sym.children = InterfaceServersAsSymbols(appRef);
    }

    return sym;
}



//--------------------------------------------------------------------------------------------------
/**
 * A map of definition sections, collected by their names.
 */
//--------------------------------------------------------------------------------------------------
type SectionMapType = { [index: string]: DefinitionObject };



//--------------------------------------------------------------------------------------------------
/**
 * Define a logical view of application objects for the system view.
 */
//--------------------------------------------------------------------------------------------------
interface SystemAppView
{
    /** Map of system app includes. */
    includedAppSections: SectionMapType;

    /** Section that represents the apps of the main .sdef. */
    mainSection: DefinitionObject;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create the display sections for the apps found within the system model, broken down into where
 * the apps were found.
 */
//--------------------------------------------------------------------------------------------------
function CreateSections(systemPath: string, appSections: model.SystemAppsSection[]): SystemAppView
{
    let viewSections: SystemAppView = { includedAppSections: {}, mainSection: undefined };

    // Go through all of the system's subsections, and convert them over to the symbol model
    for (let appSection of appSections)
    {
        let newName = 'apps [' + path.basename(appSection.location.file) + ']';
        let section = viewSections.includedAppSections[newName];

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

            viewSections.includedAppSections[newName] = section;

            if (section.defaultCollapsed === false)
            {
                viewSections.mainSection = section;
            }
        }

        // Now, lets go through and sort all of our apps into the correct sections.
        for (let appRef of appSection.apps)
        {
            if (appRef.target !== undefined)
            {
                section.children.push(AppAsSymbol(appRef));
            }
        }
    }

    return viewSections;
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert our section map to an array of system view objects.
 */
//--------------------------------------------------------------------------------------------------
function SectionMapToDefinitionObjects(sectionMap: SectionMapType): DefinitionObject[]
{
    return Object.keys(sectionMap).map(
        (key): DefinitionObject =>
        {
            return sectionMap[key];
        })
        .filter(
            (defObject: DefinitionObject): boolean =>
            {
                // Filter out any sections that don't have any children.  If the collection's
                // default collapsed flag is false, that means it's the main section, and we don't
                // want that in here either.
                if (   (defObject.children.length === 0)
                    || (defObject.defaultCollapsed === false))
                {
                    return false;
                }

                return true;
            });
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert a collection of application references into symbol objects.  The apps are grouped by
 * which system definition they came from.
 */
//--------------------------------------------------------------------------------------------------
function AppsAsSymbols
(
    systemPath: string,
    appSections: model.SystemAppsSection[]
)
: DefinitionObject[]
//--------------------------------------------------------------------------------------------------
{
    // Keep track of the app subsections that needed to be created.
    let viewSections = CreateSections(systemPath, appSections);

    // Convert or app map to the include node for the system view.
    let incSection = new DefinitionObject(ext.le_DefinitionObjectType.IncDefs,
                                          'included apps',
                                          systemPath,
                                          new loader.Location(),
                                          new loader.Location(),
                                          true);

    incSection.children = SectionMapToDefinitionObjects(viewSections.includedAppSections);

    // Return the main app nodes as well as the included apps.
    return [ viewSections.mainSection, incSection ];
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
