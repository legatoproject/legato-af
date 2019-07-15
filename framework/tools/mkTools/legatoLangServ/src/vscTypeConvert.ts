
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

import * as model from './model/annotatedModel';
import * as loader from './model/loader';
import * as jdoc from "./model/jsonDocument";
import * as ext from './lspExtensionDefs';



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
 * .
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
 * .
 */
//--------------------------------------------------------------------------------------------------
function ExecutablesAsSymbols(executableSections: model.ExecutableSection[]): DefinitionObject[]
{
    let sectionMap: { [index: string]: DefinitionObject } = {};

    for (let executableSection of executableSections)
    {
        let section = sectionMap[executableSection.location.file];

        if (section === undefined)
        {
            section = new DefinitionObject(ext.le_DefinitionObjectType.DefSection,
                                           'executables',
                                           executableSection.location.file,
                                           executableSection.location,
                                           executableSection.endLocation);

            sectionMap[executableSection.location.file] = section;
        }

        for (let executable of executableSection.executables)
        {
            let sym = new DefinitionObject(ext.le_DefinitionObjectType.ComponentRef,
                                           executable.name,
                                           executable.location.file,
                                           executable.location,
                                           new loader.Location(executable.name,
                                                               executable.location.line,
                                                               executable.location.column +
                                                                   executable.name.length));
            section.children.push(sym);
        }
    }

    return Object.keys(sectionMap).map(key => sectionMap[key]);
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert a collection of application references into symbol objects.  The apps are grouped by
 * which system definition they came from.
 */
//--------------------------------------------------------------------------------------------------
function AppsAsSymbols(flags: ConversionFlags,
                       systemPath: string,
                       appSections: model.SystemAppsSection[]): DefinitionObject[]
{
    // Keep track of the app subsections that needed to be created.
    let sectionMap: { [index: string]: DefinitionObject } = {};

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
                let components = ComponentsAsSymbols(appRef.target.componentSections);

                if (components.length > 0)
                {
                    sym.children = sym.children.concat(components);
                }
            }

            section.children.push(sym);
        }
    }

    return Object.keys(sectionMap).map(
        (key):DefinitionObject =>
        {
            return sectionMap[key];
        })
        .filter(
            (defObject: DefinitionObject): boolean =>
            {
                if (defObject.children.length === 0)
                {
                    return false;
                }

                return true;
            });
}



function ModulesAsSymbols(systemPath: string, modules: jdoc.Module[]): DefinitionObject
{
    let newModules = modules.map((module: jdoc.Module): DefinitionObject =>
        {
            let location = new loader.Location(module.path, 0, 0);

            return new DefinitionObject(ext.le_DefinitionObjectType.KernelRef,
                                        module.name,
                                        module.path,
                                        location,
                                        location);
        });

    let location = new loader.Location(systemPath, 0, 0);
    let moduleCollection = new DefinitionObject(ext.le_DefinitionObjectType.KernelRef,
                                                "Kernel Modules",
                                                systemPath,
                                                location,
                                                location);

    moduleCollection.children = newModules;

    return moduleCollection;
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct a symbol object from a Legato logical system object.  The constituent pieces of the
 * logical model will be represented by this object's children collection.
 */
//--------------------------------------------------------------------------------------------------
export function SystemAsSymbol(flags: ConversionFlags, system: model.System,
                               modules: jdoc.Module[]): DefinitionObject
{
    let start = new loader.Location(system.path, 0, 0);
    let symbol = new DefinitionObject(ext.le_DefinitionObjectType.DefinitionFile,
                                      system.name,
                                      system.path,
                                      start,
                                      start,
                                      false);

    symbol.children = AppsAsSymbols(flags, system.path, system.appSections);
    symbol.children.push(ModulesAsSymbols(system.path, modules));

    return symbol;
}
