
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as path from 'path';
import * as ldr from './loader';



export type Location = ldr.Location;
export type BlockMatchLocation = ldr.BlockMatchLocation;



//--------------------------------------------------------------------------------------------------
/**
 * List of known target IDs.
 */
//--------------------------------------------------------------------------------------------------
export enum LegatoTarget
{
    ar7    = 'ar7',
    ar86   = 'ar86',
    wp85   = 'wp85',
    wp750x = 'wp750x',
    wp76xx = 'wp76xx',
    wp77xx = 'wp77xx',
    ar758x = 'ar758x',
    ar759x = 'ar759x',
    virt   = 'virt'
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class DefObject
{
    location: ldr.Location;

    constructor(location: ldr.Location)
    {
        this.location = location;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class NamedDefObject extends DefObject
{
    name: string;

    constructor(name: string, location: ldr.Location)
    {
        super(location);
        this.name = name;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export enum SectionType
{
    apps = 'apps',
    appSearch = 'appSearch',
    bindingSection = 'bindings',
    buildVars = 'buildVars',
    bundlesSection = 'bundles',
    cFlags = 'cFlags',
    commands = 'commands',
    components = 'components',
    componentSearch = 'componentSearch',
    cxxFlags = 'cxxFlags',
    environmentSection = 'environment',
    executables = 'executables',
    externalBuildSection = 'externalBuild',
    externSection = 'externSection',
    headerDirSection = 'headerDir',
    interfaceSearch = 'interfaceSearch',
    javaPackageSection = 'javaPackage',
    kernelModules = 'kernelModules',
    ldFlags = 'ldFlags',
    libSection = 'lib',
    moduleSearch = 'moduleSearch',
    pythonPackageSection = 'pythonPackage',
    requiresSection = 'requiresSection',
    runSection = 'runSection',
    sourcesSection = 'sources'
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class DefSection extends DefObject
{
    type: SectionType;
    endLocation: ldr.Location;

    constructor(type: SectionType, location: ldr.BlockMatchLocation)
    {
        super(location.startLocation);
        this.type = type;
        this.endLocation = location.endLocation;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class Property extends NamedDefObject
{
    public constructor(name: string, location: Location)
    {
        super(name, location);
    }

    public static isString(arg: any): arg is StrProperty
    {
        return arg instanceof StrProperty;
    }

    public static isNumber(arg: any): arg is IntProperty
    {
        return arg instanceof IntProperty;
    }

    public static isBoolean(arg: any): arg is BoolProperty
    {
        return arg instanceof BoolProperty;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class StrProperty extends Property
{
    public value: string;

    public constructor(name: string, location: Location, value: string)
    {
        super(name, location);
        this.value = value;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class IntProperty extends Property
{
    public value: number;

    public constructor(name: string, location: Location, value: number)
    {
        super(name, location);
        this.value = value;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class BoolProperty extends Property
{
    public value: boolean;

    public constructor(name: string, location: Location, value: boolean)
    {
        super(name, location);
        this.value = value;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export type PropValueType = StrProperty | IntProperty | BoolProperty;



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface PropCollectionType
{
    [name: string]: PropValueType;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class PropertyCollection
{
    private properties: PropCollectionType;

    constructor()
    {
        this.properties = {};
    }

    get props()
    {
        return this.properties;
    }

    public append(property: PropValueType | PropValueType[])
    {
        if (property instanceof Array)
        {
            for (let newProp of property)
            {
                this.properties[newProp.name] = newProp;
            }
        }
        else
        {
            this.properties[property.name] = property;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Represent a reference to another object within the model.  The target object reference may be
 * undefined if the target object hasn't been loaded yet or could not be found.
 */
//--------------------------------------------------------------------------------------------------
export class DefObjectRef<ObjectType> extends NamedDefObject
{
    /** An optional filesystem path to the referenced file. */
    public refString: string;

    /** Points to the base directory of the referenced definition file. */
    public baseDir?: string;

    /** The extension of the referenced definition file. */
    public extension?: string;

    /** Internal reference to the actual definition object. */
    private targetObj?: ObjectType;


    /**
     * Create a new reference to a definition object.
     *
     * @param refString Name of the referenced definition file.  (Full path and extension are
     *                  optional components.)
     * @param location  Location in the parent definition file this reference was found.
     */
    public constructor(refString: string, location: Location)
    {
        super(refString, location);

        let baseDir = path.dirname(refString);
        let extension = path.extname(refString);
        //let name = path.basename(refString, extension);


        this.refString = refString;

        this.baseDir = (baseDir !== '') ? baseDir : undefined;
        this.extension = (extension !== '') ? extension : undefined;

        this.targetObj = undefined;
    }

    /** The definition object at the end of this reference.  */
    public get target(): ObjectType
    {
        return this.targetObj;
    }

    public set target(newObject: ObjectType)
    {
        this.targetObj = newObject;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class ComponentRef extends DefObjectRef<Component>
{
    public constructor(refString: string, location: Location)
    {
        super(refString, location);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class ApplicationRef extends DefObjectRef<Application>
{
    public overrides: PropertyCollection;

    public constructor(refString: string, location: Location)
    {
        super(refString, location);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class KernelModuleRef extends DefObjectRef<KernelModule>
{
    public isOptional: boolean;

    public constructor(refString: string, location: Location, isOptional: boolean)
    {
        super(refString, location);
        this.isOptional = isOptional;
    }
}


// export class ModuleSection extends DefSection {}
// export class ComponentSection extends DefSection {}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class ComponentSection extends DefSection
{
    public components: ComponentRef[];

    constructor(location: BlockMatchLocation, components: ComponentRef[])
    {
        super(SectionType.components, location);
        this.components = components;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class Executable extends NamedDefObject
{
    public componentRefs: ComponentRef[];

    public constructor(name: string, location: Location, componentRefs: ComponentRef[])
    {
        super(name, location);
        this.componentRefs = componentRefs;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class ExecutableSection extends DefSection
{
    public executables: Executable[];

    public constructor(location: BlockMatchLocation, executables: Executable[])
    {
        super(SectionType.executables, location);
        this.executables = executables;
    }
}



export class ProcessParam extends DefObject
{
    public value: string;

    public constructor(location: Location, value: string)
    {
        super(location);
        this.value = value;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class Process extends NamedDefObject
{
    public params: ProcessParam[];

    public constructor(name: string, location: Location, params: ProcessParam[])
    {
        super(name, location);
        this.params = params;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class RunSection extends DefSection
{
    public processes: Process[];

    public constructor(location: BlockMatchLocation, processes: Process[])
    {
        super(SectionType.runSection, location);
        this.processes = processes;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class EnvVar extends NamedDefObject
{
    public value: string;

    public constructor(name: string, location: Location, value: string)
    {
        super(name, location);
        this.value = value;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface EnvVarCollectionType
{
    [name: string]: EnvVar;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class EnvVarSection extends DefSection
{
    public vars: EnvVarCollectionType;

    public constructor(location: BlockMatchLocation, vars: EnvVar[])
    {
        super(SectionType.environmentSection, location);

        for (let envVar of vars)
        {
            this.vars[envVar.name] = envVar;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class ProcessSection extends DefSection
{
    public properties: PropertyCollection;
    public runSections: RunSection[];
    public envVarSections: EnvVarSection[];

    public constructor(location: BlockMatchLocation)
    {
        super(SectionType.executables, location);

        this.properties = new PropertyCollection();
        this.runSections = [];
        this.envVarSections = [];
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class RequiresSection extends DefSection
{
    public constructor(location: BlockMatchLocation)
    {
        super(SectionType.requiresSection, location);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class BundlesSection extends DefSection
{
    public constructor(location: BlockMatchLocation)
    {
        super(SectionType.bundlesSection, location);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class BindingsSection extends DefSection
{
    public constructor(location: BlockMatchLocation)
    {
        super(SectionType.bindingSection, location);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class ExternSection extends DefSection
{
    public constructor(location: BlockMatchLocation)
    {
        super(SectionType.externSection, location);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class SearchPathSection extends DefSection
{
    searchPaths: string[]

    constructor(location: BlockMatchLocation, type: SectionType, paths: string[])
    {
        super(type as SectionType, location);
        this.searchPaths = paths;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class BuildVar extends NamedDefObject
{
    value: string;

    constructor(location: Location, name: string, value: string)
    {
        super(name, location);
        this.value = value;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class BuildVarSection extends DefSection
{
    buildVars: BuildVar[];

    constructor(location: BlockMatchLocation, buildVars: BuildVar[])
    {
        super(SectionType.buildVars, location);
        this.buildVars = buildVars;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class SystemAppsSection extends DefSection
{
    apps: ApplicationRef[];

    constructor(location: BlockMatchLocation)
    {
        super(SectionType.apps, location);
        this.apps = [];
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class Command extends NamedDefObject
{
    appName: string;
    commandPath: string;

    constructor(location: Location, commandName: string, appName: string, commandPath: string)
    {
        super(commandName, location);
        this.appName = appName;
        this.commandPath = commandPath;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class CommandSection extends DefSection
{
    commands: Command[];

    constructor(location: BlockMatchLocation, commands: Command[])
    {
        super(SectionType.commands, location);
        this.commands = commands;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class KernelModuleSection extends DefSection
{
    moduleRefs: KernelModuleRef[]

    constructor(location: BlockMatchLocation, moduleRefs: KernelModuleRef[])
    {
        super(SectionType.kernelModules, location);
        this.moduleRefs = moduleRefs;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export enum DefType
{
    componentDef   = '.cdef',
    componentInc   = '.cinc',
    applicationDef = '.adef',
    applicationInc = '.ainc',
    systemDef      = '.sdef',
    systemInc      = '.sinc',
    moduleDef      = '.mdef',
    moduleInc      = '.minc'
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
function checkExt(defPath: string, expected: string): boolean
{
    let ext = path.extname(defPath);

    if (   (ext === '')
        || (ext.endsWith(expected) == false))
    {
        return false;
    }

    return true;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function isDef(defPath: string): boolean
{
    return checkExt(defPath,  'def');
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function isInc(defPath: string): boolean
{
    return checkExt(defPath, 'inc');
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
interface SectionUpdateMap
{
    [typeName: string]: Array<any>;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class DefFile
{
    public type: DefType;
    public path: string;
    public name: string;

    protected sectionMap: SectionUpdateMap;
    public rawSections: DefSection[];

    public constructor(type: DefType, filePath: string, name?: string)
    {
        this.type = type;
        this.path = filePath;

        if (name === undefined)
        {
            this.name = path.basename(filePath, path.extname(filePath));
        }
        else
        {
            this.name = name;
        }

        this.sectionMap = {};
        this.rawSections = [];
    }

    protected appendSection(newSection: DefSection)
    {
        this.rawSections.push(newSection);

        let collection = this.sectionMap[typeof newSection];

        if (collection !== undefined)
        {
            collection.push(newSection);
        }
    }

    private static isObjectDef(defPath: string, ...checks: DefType[]): boolean
    {
        let ext = path.extname(defPath);

        for (let check of checks)
        {
            if (ext === check)
            {
                return true;
            }
        }

        return false;
    }

    public static isSystemDef(defPath: string): boolean
    {
        return this.isObjectDef(defPath, DefType.systemDef, DefType.systemInc);
    }

    public static isApplicationDef(defPath: string): boolean
    {
        return this.isObjectDef(defPath, DefType.applicationDef, DefType.applicationInc);
    }

    public static isComponentDef(defPath: string): boolean
    {
        return this.isObjectDef(defPath, DefType.componentDef, DefType.componentInc);
    }

    public static isModuleDef(defPath: string): boolean
    {
        return this.isObjectDef(defPath, DefType.componentDef, DefType.componentInc);
    }

    public static asSystem(def: DefFile): def is System
    {
        return def instanceof System;
    }

    public static asApplication(def: DefFile): def is Application
    {
        return def instanceof Application;
    }

    public static asComponent(def: DefFile): def is Component
    {
        return def instanceof Component;
    }

    public static asModule(def: DefFile): def is KernelModule
    {
        return def instanceof KernelModule;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class Component extends DefFile
{
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class Application extends DefFile
{
    public properties: PropertyCollection;
    public componentSections: ComponentSection[];
    public executableSections: ExecutableSection[];
    public processSections: ProcessSection[];
    public requiresSections: RequiresSection[];
    public bundlesSections: BundlesSection[];
    public bindingsSections: BindingsSection[];
    public externSections: ExternSection[];

    public constructor(filePath: string, name?: string)
    {
        if (name === undefined)
        {
            name = path.basename(filePath, path.extname(filePath));
        }

        super(DefType.applicationDef, filePath, name);

        this.properties = new PropertyCollection();

        this.componentSections = [];
        this.executableSections = [];
        this.processSections = [];
        this.requiresSections = [];
        this.bundlesSections = [];
        this.bindingsSections = [];
        this.externSections = [];
    }

    public setPath(filePath: string)
    {
        super.path = filePath;
        super.name = path.basename(filePath, path.extname(filePath));
    }

    public appendSection(newSection: DefSection)
    {
console.log('### Application.appendSection');
        if (newSection instanceof ComponentSection)
        {
console.log('###      ComponentSection');
            this.componentSections.push(newSection);
        }
        else if (newSection instanceof ExecutableSection)
        {
console.log('###      ExecutableSection');
            this.executableSections.push(newSection);
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class KernelModule extends DefFile
{
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export class System extends DefFile
{
    public searchPathSections: SearchPathSection[];
    public buildVarSections: BuildVarSection[];
    public appSections: SystemAppsSection[];
    public commandSections: CommandSection[];
    public moduleSections: KernelModuleSection[];

    public constructor(filePath: string, name?: string)
    {
        if (name === undefined)
        {
            name = path.basename(filePath, DefType.systemDef);
        }

        super(DefType.systemDef, filePath, name);

        this.searchPathSections = [];
        this.buildVarSections = [];
        this.appSections = [];
        this.commandSections = [];
        this.moduleSections = [];
    }

    public appendSection(newSection: DefSection)
    {
        interface AddMap
        {
            [typeName: string]: Array<any>;
        }

        let map: AddMap = {};

        map[typeof SystemAppsSection] = this.appSections;
        map[typeof CommandSection] = this.commandSections;
        map[typeof SearchPathSection] = this.searchPathSections;


        if (newSection instanceof SystemAppsSection)
        {
            this.appSections.push(newSection);
        }
        else if (newSection instanceof CommandSection)
        {
            this.commandSections.push(newSection);
        }
        else if (newSection instanceof SearchPathSection)
        {
            this.searchPathSections.push(newSection);
        }
        else
        {
            // Unexpected object type!!!
        }
    }
}
