
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
import * as ext from './lspExtensionDefs';



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

    public defaultCollapsed?: boolean;

    public defPath: string;
    public children?: ext.le_DefinitionObject[];

    public range: lsp.Range;
    public selectionRange: lsp.Range;

    public constructor(type: ext.le_DefinitionObjectType, name: string, path: string,
                       start: loader.Location, end: loader.Location,
                       defaultCollapsed?: boolean)
    {
        let newRange = lsp.Range.create(start.line - 1, start.column, end.line - 1, end.column + 1);

        this.name = name;
        this.kind = type as lsp.SymbolKind;
        this.defPath = Uri.file(path).toString();

        this.range = newRange;
        this.selectionRange = newRange;

        if (defaultCollapsed !== undefined)
        {
            this.defaultCollapsed = true;
        }

        this.children = [];
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
function ComponentsAsSymbols(componentSections: model.ComponentSection[]): DefinitionObject[]
{
    let sectionMap: { [index: string]: DefinitionObject } = {};

    for (let componentSection of componentSections)
    {
        let section = sectionMap[componentSection.location.file];

        if (section === undefined)
        {
            section = new DefinitionObject(ext.le_DefinitionObjectType.DefSection,
                                           'components',
                                           componentSection.location.file,
                                           componentSection.location,
                                           componentSection.endLocation);

            sectionMap[componentSection.location.file] = section;
        }

        for (let component of componentSection.components)
        {

            let sym = new DefinitionObject(ext.le_DefinitionObjectType.ComponentRef,
                                           component.name,
                                           component.location.file,
                                           component.location,
                                           new loader.Location(component.name,
                                                               component.location.line,
                                                               component.location.column +
                                                                   component.name.length));
            section.children.push(sym);
        }
    }

    return Object.keys(sectionMap).map(key => sectionMap[key]);
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
            let defaultCollapsed = undefined;

            if (flags.supportsDefaultCollapsed)
            {
                if (appSection.location.file !== systemPath)
                {
                    defaultCollapsed = true;
                }
                else
                {
                    defaultCollapsed = false;
                }
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
            let start = new loader.Location(appRef.target.path, 0, 0);
            let sym = new DefinitionObject(ext.le_DefinitionObjectType.ApplicationRef,
                                           path.basename(appRef.name, path.extname(appRef.name)),
                                           appRef.target.path,
                                           start,
                                           start);

            // Disabled for initial commit, will be re-enabled once the model loading stablity is
            // fixed.

            // if (appRef.target !== undefined)
            // {
            //     let components = ComponentsAsSymbols(appRef.target.componentSections);
            //
            //     if (components.length > 0)
            //     {
            //         sym.children = sym.children.concat(components);
            //     }
            //
            //     let executables = ExecutablesAsSymbols(appRef.target.executableSections);
            //
            //     if (executables.length > 0)
            //     {
            //         sym.children = sym.children.concat(executables);
            //     }
            // }

            section.children.push(sym);
        }
    }

    return Object.keys(sectionMap).map(
        (key):DefinitionObject =>
        {
            return sectionMap[key];
        });

//    return Object.keys(sectionMap).map(key => sectionMap[key]);
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct a symbol object from a Legato logical system object.  The constituent pieces of the
 * logical model will be represented by this object's children collection.
 */
//--------------------------------------------------------------------------------------------------
export function SystemAsSymbol(flags: ConversionFlags, system: model.System): DefinitionObject
{
    let start = new loader.Location(system.path, 0, 0);
    let symbol = new DefinitionObject(ext.le_DefinitionObjectType.DefinitionFile,
                                      system.name,
                                      system.path,
                                      start,
                                      start);

    symbol.children = AppsAsSymbols(flags, system.path, system.appSections);

    return symbol;
}
