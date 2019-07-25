
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as path from 'path';

import * as model from './annotatedModel';
import * as jdoc from "./jsonDocument";
import * as parser from './parser';



//--------------------------------------------------------------------------------------------------
/**
 * Represent the original location a token was found within a def file's source code.
 */
//--------------------------------------------------------------------------------------------------
export class Location extends parser.Location
{
    public file: string;
    public line: number;
    public column: number;

    public constructor(file?: string, line?: number, column?: number)
    {
        let newFile   = file   === undefined ? "" : file;
        let newLine   = line   === undefined ? 1  : line;
        let newColumn = column === undefined ? 1  : column;;

        super(newFile, newLine, newColumn);
    }

    public toString(): string
    {
        return path.basename(this.file)  + '(' + this.line + ', ' + this.column + ')';
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export interface BlockMatchLocation extends parser.BlockMatchLocation
{
    startLocation: Location;
    endLocation: Location;
}



//--------------------------------------------------------------------------------------------------
/**
 * A token as generated from a Legato def file's source code.
 */
//--------------------------------------------------------------------------------------------------
export class Token extends parser.Token
{
    public type: jdoc.TokenType;
    public location: Location;

    public constructor(file?: string, jToken?: jdoc.Token)
    {
        let newFile = file === undefined ? "" : file;

        let newType = jdoc.TokenType.EndOfFile;
        let newText = "";
        let newLine = 1;
        let newColumn = 1;

        if (jToken !== undefined)
        {
            newText   = jToken.text;
            newLine   = jToken.line;
            newColumn = jToken.column;
            newType   = jToken.type;
        }

        super(new Location(newFile, newLine, newColumn), newText);
        this.type = newType;
    }

    public toString(): string
    {
        return this.location.toString() + ": " + this.type + ": '" + this.text + "'";
    }

    public isMatch(other: parser.Token): boolean
    {
        if (   (other instanceof Token)
            || (other instanceof DataToken))
        {
            return this.type === other.type;
        }

        return false;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export abstract class DataToken extends parser.DataToken implements Token
{
    public type: jdoc.TokenType;
    public location: Location;

    public constructor(type: jdoc.TokenType)
    {
        super();

        this.type = type;
        this.location = new Location();
    }

    public toString(): string
    {
        return this.location.toString() + ": " + this.type + ": '" + this.text + "'";
    }

    public isMatch(other: parser.Token): boolean
    {
        if (other instanceof Token)
        {
            return this.type === other.type;
        }

        return false;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function newToken(type: jdoc.TokenType): parser.Token
{
    let token = new Token();
    token.type = type;

    return token;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function newStringToken(type: jdoc.TokenType): parser.DataToken
{
    class StringToken extends DataToken
    {
        public constructor(type: jdoc.TokenType)
        {
            super(type);
        }

        public convert(text: string): any
        {
            return text;
        }
    }

    return new StringToken(type);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function newIntToken(type: jdoc.TokenType): parser.DataToken
{
    class IntToken extends DataToken
    {
        public constructor(type: jdoc.TokenType)
        {
            super(type);
        }

        public convert(text: string): any
        {
            return Number.parseInt(text);
        }
    }

    return new IntToken(type);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function newFpToken(): parser.DataToken
{
    class FpToken extends DataToken
    {
        public constructor(type: jdoc.TokenType)
        {
            super(type);
        }

        public convert(text: string): any
        {
            return parseFloat(text);
        }
    }

    return new FpToken(jdoc.TokenType.Float);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function newBoolToken(): parser.DataToken
{
    class BoolToken extends DataToken
    {
        public constructor(type: jdoc.TokenType)
        {
            super(type);
        }

        public convert(text: string): any
        {

            if (text === 'false')
            {
                return false;
            }

            return true;
        }
    }

    return new BoolToken(jdoc.TokenType.Boolean);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export const defTokens =
    {
        EndOfFile:           newToken(jdoc.TokenType.EndOfFile),
        OpenCurly:           newToken(jdoc.TokenType.OpenCurly),
        CloseCurly:          newToken(jdoc.TokenType.CloseCurly),
        OpenParenthesis:     newToken(jdoc.TokenType.OpenParenthesis),
        CloseParenthesis:    newToken(jdoc.TokenType.CloseParenthesis),
        Colon:               newToken(jdoc.TokenType.Colon),
        Equals:              newToken(jdoc.TokenType.Equals),
        Dot:                 newToken(jdoc.TokenType.Dot),
        Star:                newToken(jdoc.TokenType.Star),
        Arrow:               newToken(jdoc.TokenType.Arrow),
        Whitespace:          newToken(jdoc.TokenType.Whitespace),
        Comment:             newToken(jdoc.TokenType.Comment),
        FilePermissions:     newStringToken(jdoc.TokenType.FilePermissions),
        ServerIpcOption:     newStringToken(jdoc.TokenType.ServerIpcOption),
        ClientIpcOption:     newStringToken(jdoc.TokenType.ClientIpcOption),
        Arg:                 newStringToken(jdoc.TokenType.Arg),
        FilePath:            newStringToken(jdoc.TokenType.FilePath),
        FileName:            newStringToken(jdoc.TokenType.FileName),
        Name:                newStringToken(jdoc.TokenType.Name),
        DottedName:          newStringToken(jdoc.TokenType.DottedName),
        GroupName:           newStringToken(jdoc.TokenType.GroupName),
        IpcAgent:            newStringToken(jdoc.TokenType.IpcAgent),
        Integer:             newIntToken(jdoc.TokenType.Integer),
        SignedInteger:       newIntToken(jdoc.TokenType.SignedInteger),
        Boolean:             newBoolToken(),
        String:              newStringToken(jdoc.TokenType.String),
        Float:               newFpToken(),
        Md5Hash:             newStringToken(jdoc.TokenType.Md5Hash),
        Directive:           newStringToken(jdoc.TokenType.Directive),
        OptionalOpenSquare:  newToken(jdoc.TokenType.OptionalOpenSquare),
        ProvideHeaderOption: newToken(jdoc.TokenType.ProvideHeaderOption)
    };



export const legatoCollectionDelimiters: parser.BlockDelimiterTokens =
    {
        begin: defTokens.OpenParenthesis,
        end: defTokens.CloseParenthesis
    };



    export const legatoBlockDelimiters: parser.BlockDelimiterTokens =
    {
        begin: defTokens.OpenCurly,
        end: defTokens.CloseCurly
    };



export function expectNamedBlockOf(name: string,
                                   contents: parser.Matcher,
                                   action?: parser.BlockMatchHandler): parser.Matcher
{
    return parser.expectExpression
        (
            parser.expectValue(defTokens.Name, name),
            parser.expectToken(defTokens.Colon),
            parser.expectBlockListOf
            (
                contents,
                legatoBlockDelimiters,
                undefined,
                action
            ),
        );
}



export function expectNamedCollection<dt, vt>
(
    dataCollection: (name: string, location: parser.BlockMatchLocation, values: vt[]) => dt,
    dataObject: (name: string, location: parser.Location) => vt,
    name: parser.DataToken,
    item: parser.DataToken
)
: parser.Matcher
{
    return parser.expectExpression
        (
            parser.optionalExpression
            (
                parser.expectData(name),
                parser.expectToken(defTokens.Equals),
            ),

            parser.expectBlockListOf
            (
                parser.expectExpression
                (
                    parser.expectData(item),

                    (location: parser.Location, data: any[]): any =>
                    {
                        return dataObject(data[0], location);
                    }
                ),

                legatoCollectionDelimiters,

                undefined,

                (location: parser.BlockMatchLocation, data: any[]): any =>
                {
                    return dataCollection('', location, data as vt[]);
                }
            ),

            (_location: parser.Location, data: any[]): any =>
            {
                let collection = data[1] as dt;

                if (collection === undefined)
                {
                    return collection;
                }

                if (   ('name' in collection)
                    && (data.length >= 1))
                {
                    (collection as unknown as { name: string }).name = data[0];
                }

                return collection;
            }
        );
}



export function expectStringSection(type: model.SectionType,
                                    itemType: parser.DataToken): parser.Matcher
{
    return expectNamedBlockOf
        (
            type,
            parser.expectData(itemType),

            (location: BlockMatchLocation, data: any[]): any =>
            {
                return new model.SearchPathSection(location, type, data);
            }
        );
}



export const bindingNameTriple =
    parser.expectExpression
    (
        parser.expectData(defTokens.IpcAgent),
        defTokens.Dot,
        parser.expectData(defTokens.Name)
    );



export const bindingsSection =
        parser.expectExpression
        (
            parser.expectData(defTokens.IpcAgent),
            defTokens.Dot,

            parser.expectOneOf
            (
                parser.expectExpression
                (
                    defTokens.Star,
                    defTokens.Dot,
                    parser.expectData(defTokens.Name)
                ),

                parser.expectExpression
                (
                    defTokens.Dot,
                    parser.expectData(defTokens.Name),
                    defTokens.Dot,
                    parser.expectData(defTokens.Name)
                )
            ),

            defTokens.Arrow,

            parser.expectData(defTokens.IpcAgent),
            defTokens.Dot,
            parser.expectData(defTokens.Name)
        );



export function expectStrProperty
(
    name: string,
    dataType: parser.DataToken
)
: parser.Matcher
{
    return parser.expectExpression
        (
            parser.expectValue(defTokens.Name, name),
            parser.expectToken(defTokens.Colon),
            parser.expectData(dataType),

            (location: Location, data: any[]): any =>
            {
                let prop: model.StrProperty =
                    {
                        name: name,
                        location: location,
                        value: data[0],
                    };

                return prop;
            }
        );
}



export function expectIntProperty
(
    name: string,
    dataType: parser.DataToken
)
: parser.Matcher
{
    return parser.expectExpression
        (
            parser.expectValue(defTokens.Name, name),
            parser.expectToken(defTokens.Colon),
            parser.expectData(dataType),

            (location: Location, data: any[]): any =>
            {
                let prop: model.IntProperty =
                    {
                        name: name,
                        location: location,
                        value: data[0],
                    };

                return prop;

            }
        );
}



export function expectBoolProperty
(
    name: string,
    dataType: parser.DataToken
)
: parser.Matcher
{
    return parser.expectExpression
        (
            parser.expectValue(defTokens.Name, name),
            parser.expectToken(defTokens.Colon),
            parser.expectData(dataType),

            (location: Location, data: any[]): any =>
            {
                let prop: model.BoolProperty =
                    {
                        name: name,
                        location: location,
                        value: data[0],
                    };

                return prop;
            }
        );
}



export function expectEnumProperty
(
    name: string,
    dataType: parser.DataToken,
    ...values: string[]
)
: parser.Matcher
{
    return parser.expectExpression
        (
            parser.expectValue(defTokens.Name, name),
            parser.expectToken(defTokens.Colon),

            parser.expectOneOf
            (
                ...values.map((value: string): parser.Matcher =>
                    {
                        return parser.expectValue(dataType, value);
                    })
            )
        );
}



export const watchdogProperties =
    parser.expectOneOf
    (
        expectIntProperty('maxWatchdogTimeout', defTokens.Integer),

        expectEnumProperty
        (
            'watchdogAction', defTokens.Name,
            "ignore", "restart", "restartApp", "stop", "stopApp", "reboot"
        ),

        parser.expectExpression
        (
            parser.expectValue(defTokens.Name, 'watchdogTimeout'),
            parser.expectToken(defTokens.Colon),

            parser.expectExpression
            (
                parser.expectValue(defTokens.Name, 'never'),

                (location: Location, _data: any[]): any =>
                {
                    return new model.StrProperty('watchdogTimeout', location, 'never')
                }
            ),

            parser.expectExpression
            (
                parser.expectData(defTokens.Integer),

                (location: Location, data: any[]): any =>
                {
                    return new model.StrProperty('watchdogTimeout', location, data[0])
                }
            )
        )
    );



export const componentsSection =
    expectNamedBlockOf
    (
        "components",
        parser.expectExpression
        (
            parser.expectData(defTokens.FilePath),

            (location: parser.Location, data: any[]) =>
            {
                return new model.ComponentRef(data[0], location);
            }
        ),

        (location: parser.BlockMatchLocation, data: any[]): any =>
        {
            return new model.ComponentSection(location, data as model.ComponentRef[]);
        }
    );



export const bundledFileSection =
    expectNamedBlockOf
    (
        'file',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),

            parser.expectData(defTokens.FilePath),
            parser.expectData(defTokens.FilePath)
        )
    );



export const bundledBinarySection =
    expectNamedBlockOf
    (
        'binary',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),

            parser.expectData(defTokens.FilePath),
            parser.expectData(defTokens.FilePath)
        )
    );



export const bundledDirSection =
    expectNamedBlockOf
    (
        'dir',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),

            parser.expectData(defTokens.FilePath),
            parser.expectData(defTokens.FilePath)
        )
    );



export const bundlesSection =
    expectNamedBlockOf
    (
        'bundles',

        parser.expectMultipleOf
        (
            parser.expectOneOf
            (
                bundledFileSection,
                bundledBinarySection,
                bundledDirSection
            )
        )
    );



export const requiredDirSection =
    expectNamedBlockOf
    (
        'dir',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),

            parser.expectData(defTokens.FilePath),
            parser.expectData(defTokens.FilePath)
        )
    );



export const requiredFileSection =
    expectNamedBlockOf
    (
        'file',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),

            parser.expectData(defTokens.FilePath),
            parser.expectData(defTokens.FilePath)
        )
    );



export const requiredDeviceSection =
    expectNamedBlockOf
    (
        'device',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),

            parser.expectData(defTokens.FilePath),
            parser.expectData(defTokens.FilePath)
        )
    );



/** Parse a module ref section.  These sections can be found in systems, apps, and components. */
export const requiredKernelModulesSection =
    expectNamedBlockOf
    (
        'kernelModules',

        parser.expectExpression
        (
            parser.expectData(defTokens.FilePath),
            parser.optionalExpression(parser.expectToken(defTokens.OptionalOpenSquare))
        )
    );



const appReference =
    parser.expectExpression
    (
        parser.expectData(defTokens.FilePath),
        //parser.optionalBlockListOf
        //(
        //    appOverrides,
        //    legatoBlockDelimiters
        //),

        (location: Location, data: any[]): any =>
        {
            let appRef = new model.ApplicationRef(data as any, location);

            return appRef;
        }
    );



const appsSection =
    expectNamedBlockOf
    (
        "apps",
        appReference,

        (location: BlockMatchLocation, data: any[]): any =>
        {
            let appSection = new model.SystemAppsSection(location);

            if (   (data !== undefined)
                && (data.length > 0))
            {
                appSection.apps = data as model.ApplicationRef[];
            }

            return appSection;
        }
    );



const systemCommand =
    parser.expectExpression
    (
        parser.expectData(defTokens.Name),
        parser.expectToken(defTokens.Equals),
        parser.expectData(defTokens.Name),
        parser.expectToken(defTokens.Colon),
        parser.expectData(defTokens.FilePath),

        (location: Location, data: any[]): any =>
        {
            return new model.Command(location,
                                    data[0] as string,
                                    data[1] as string,
                                    data[2] as string);
        }
    );



const commandsSection =
    expectNamedBlockOf
    (
        "commands",
        systemCommand,

        (location: BlockMatchLocation, data: any[]): any =>
        {
            return new model.CommandSection(location, data as model.Command[]);
        }
    );



const buildVars =
    expectNamedBlockOf
    (
        "buildVars",
        parser.expectExpression
        (
            parser.expectData(defTokens.Name),
            defTokens.Equals,
            parser.expectData(defTokens.FilePath),

            (location: Location, data: any[]): any =>
            {
                return new model.BuildVar(location, data[0] as string, data[1] as string);
            }
        ),

        (location: BlockMatchLocation, data: any[]): any =>
        {
            return new model.BuildVarSection(location, data as model.BuildVar[]);
        }
    );



const extWdKickSection =
    parser.expectExpression
    (
        parser.expectValue(defTokens.Name, 'externalWatchdogKick'),
        defTokens.Colon,
        parser.expectData(defTokens.Integer)
    );



export const systemDefinition =
    parser.expectExpression
    (
        parser.expectMultipleOf
        (
            parser.expectOneOf
            (
                appsSection,

                commandsSection,

                expectStringSection(model.SectionType.interfaceSearch, defTokens.FilePath),
                expectStringSection(model.SectionType.appSearch, defTokens.FilePath),
                expectStringSection(model.SectionType.componentSearch, defTokens.FilePath),
                expectStringSection(model.SectionType.moduleSearch, defTokens.FilePath),

                buildVars,

                bindingsSection,

                expectStringSection(model.SectionType.cFlags, defTokens.FilePath),
                expectStringSection(model.SectionType.cxxFlags, defTokens.FilePath),
                expectStringSection(model.SectionType.ldFlags, defTokens.FilePath),

                requiredKernelModulesSection,

                extWdKickSection
            ),
        ),

        (location: Location, data: any[]): any =>
        {
            let systemDef = new model.System(location.file);

            for (let item of data)
            {
                systemDef.appendSection(item as model.DefSection);
            }

            return systemDef;
        }
    );



const executablesSection =
    expectNamedBlockOf
    (
        'executables',

        parser.expectExpression
        (
            parser.expectData(defTokens.Name),
            parser.expectToken(defTokens.Equals),

            parser.expectBlockListOf
            (
                parser.expectData(defTokens.FilePath),
                legatoCollectionDelimiters
            ),

            (location: parser.Location, data: any[]): any =>
            {
                if (typeof data === 'string')
                {
                    return data;
                }

                let name = data[0] as string;
                data.shift();


                return new model.Executable(name, location, data);
            }
        ),

        (location: parser.BlockMatchLocation, data: any[]): any =>
        {
            return new model.ExecutableSection(location, data);
        }
    );



const procEnvVarSection =
    expectNamedBlockOf
    (
        "envVars",
        parser.expectExpression
        (
            parser.expectData(defTokens.Name),
            parser.expectToken(defTokens.Equals),
            parser.expectData(defTokens.FilePath)
        ),

        (location: parser.BlockMatchLocation, data: any[]): any =>
        {
            return new model.EnvVarSection(location, data as model.EnvVar[]);
        }
    );



const procPropertiesSection =
    parser.expectOneOf
    (
        expectEnumProperty
        (
            'faultAction', defTokens.Name,
            'ignore', 'restart', 'restartApp', 'stopApp', 'reboot'
        ),

        expectEnumProperty
        (
            'priority', defTokens.Name,
            'idle', 'low', 'medium', 'high'
        ),

        expectIntProperty('maxCoreDumpFileBytes', defTokens.Integer),
        expectIntProperty('maxFileBytes', defTokens.Integer),
        expectIntProperty('maxFileDescriptors', defTokens.Integer),
        expectIntProperty('maxLockedMemoryBytes', defTokens.Integer),

        watchdogProperties
    );



const processSection =
    expectNamedBlockOf
    (
        "processes",
        parser.expectOneOf
        (
            //runSection,
            procEnvVarSection,
            procPropertiesSection
        ),

        (_location: parser.BlockMatchLocation, data: any[]): any =>
        {
            return data;
        }
    );



const requiredConfigTreeSection =
    expectNamedBlockOf
    (
        'configTree',

        parser.expectExpression
        (
            parser.optionalExpression(parser.expectData(defTokens.FilePermissions)),
            parser.expectData(defTokens.FilePath),
        )
    );



const appRequiresSection =
    expectNamedBlockOf
    (
        'requires',

        parser.expectOneOf
        (
            requiredConfigTreeSection,
            requiredDirSection,
            requiredFileSection,
            requiredDeviceSection,
            requiredKernelModulesSection
        )
    );



const externSection =
    expectNamedBlockOf
    (
        'extern',

        parser.expectOneOf
        (
            parser.expectExpression
            (
                parser.expectData(defTokens.Name),
                parser.expectToken(defTokens.Equals),
                bindingNameTriple
            ),

            parser.expectExpression
            (
                bindingNameTriple
            )
        )
    );




/** Rule to match the top level of an application definition file. */
export const appDefinition =
        parser.expectExpression
        (
            parser.expectMultipleOf
            (
                parser.expectOneOf
                (
                    //appPropertiesSection,
                    processSection,
                    executablesSection,
                    componentsSection,
                    bundlesSection,
                    appRequiresSection,
                    bindingsSection,
                    externSection
                )
            ),

            (location: Location, data: any[]): any =>
            {
                let filePath: string = '';

                if (location !== undefined)
                {
                    filePath = location.file;
                }

                let appDef = new model.Application(filePath);

                for (let value of data)
                {
                    appDef.appendSection(value);
                }

                return appDef;
            }
        );



//--------------------------------------------------------------------------------------------------
/**
 * Called to filter out any preprocessor directives from the supplied token list.
 */
//--------------------------------------------------------------------------------------------------
function preProcess(tokens: Token[]): Token[]
{
    let discard = 0;

    return tokens.filter(
        (token: Token): boolean =>
        {
            // Check to see if we are keeping or dropping the current token...

            if (discard > 0)
            {
                // We are in the middle of dropping paramter tokens, so drop this one.
                discard = discard - 1;
                return false;
            }
            else if (token.type === jdoc.TokenType.Directive)
            {
                if (   (token.text === "#if")
                    || (token.text === "#elif"))
                {
                    // We will also need to remove the whitespace and parameter tokens for the "if"
                    // statement.
                    discard = 6;
                }

                // Make sure that this directive token is removed, no matter what type of directive
                // it is, like #endif.
                return false;
            }

            // This token is good, keep it.
            return true;
        });
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert from the json tokens to our internal token type.
 */
//--------------------------------------------------------------------------------------------------
export function convertTokensFromJson(testFile: string, jsonTokens: any[]): Token[]
{
    let newTokens: Token[] = [];

    for (let jToken of jsonTokens)
    {
        newTokens.push(new Token(testFile, jToken));
    }

    return preProcess(newTokens);
}



//--------------------------------------------------------------------------------------------------
/**
 * Find the given file path from the given document's token map.
 */
//--------------------------------------------------------------------------------------------------
function findFile(jsonDoc: jdoc.Document, fileName: string)
{
    if (path.extname(fileName) === '')
    {
        fileName += '.adef';
    }

    for (let foundPath in jsonDoc.tokenMap)
    {
        let nextName = path.basename(foundPath);

        if (nextName === fileName)
        {
            return foundPath;
        }
    }

    return undefined;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
export function parseApp(jsonDoc: jdoc.Document, defFilePath: string): model.Application
{
    let tokenList = convertTokensFromJson(defFilePath, jsonDoc.tokenMap[defFilePath]);
    let buffer = new parser.TokenBuffer(tokenList,
                                        defTokens.EndOfFile,
                                        [
                                            defTokens.Comment,
                                            defTokens.Whitespace,
                                            defTokens.Directive
                                        ]);

    let result = appDefinition.pull(buffer);

    if (result.isMatch)
    {
        let application = result.data.value as model.Application;

        if (application.path === '')
        {
            application.setPath(defFilePath);
        }

        return application;
    }

    return undefined;
}



function convertComponent(jsonDoc: jdoc.Document, appRef: string, name: string): model.ComponentRef
{
    let foundComponent: jdoc.Component;

    for (let component of jsonDoc.model.components)
    {
        if (component.name === name)
        {
            foundComponent = component;
            break;
        }
    }

    if (foundComponent === undefined)
    {
        return undefined;
    }

    let refLoc: Location = new Location(appRef, 1, 1);
    let componentRef = new model.ComponentRef(name, refLoc);

    let newComponent = new model.Component(foundComponent.path, foundComponent.name);
    let newComponentLoc = new Location(foundComponent.path, 1, 1);
    let newBlockLoc: BlockMatchLocation =
        {
            startLocation: new Location(foundComponent.path, 1, 1),
            endLocation: new Location(foundComponent.path, 1, 1)
        };

    newComponent.sourcesSections.push(new model.SourceSection(newBlockLoc, []));

    foundComponent.sources.c.forEach(cSource =>
        {
            let source = new model.Source(newComponentLoc, cSource, model.SourceType.C);
            newComponent.sourcesSections[0].sources.push(source);
        });

    foundComponent.sources.cxx.forEach(cxxSource =>
        {
            let source = new model.Source(newComponentLoc, cxxSource, model.SourceType.Cpp);
            newComponent.sourcesSections[0].sources.push(source);
        });

    componentRef.target = newComponent;

    return componentRef;
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert an app object found within the JSON document into a parsed annotated model object.
 */
//--------------------------------------------------------------------------------------------------
function convertApp(jsonDoc: jdoc.Document,
                    appDefPath: string,
                    isIncluded: boolean): model.Application
{
    let foundApp: jdoc.Application = undefined;

    for (let app of jsonDoc.model.apps)
    {
        if (app.path === appDefPath)
        {
            foundApp = app;
            break;
        }
    }

    if (foundApp === undefined)
    {
        return undefined;
    }

    let newApp = new model.Application(foundApp.path, foundApp.name);
    let location: BlockMatchLocation =
        {
            startLocation: new Location(newApp.path, 1, 1),
            endLocation: new Location(newApp.path, 1, 1)
        };

    newApp.interfaces = foundApp.interfaces;
    newApp.isIncluded = isIncluded;

    newApp.componentSections.push(new model.ComponentSection(location, []));
    newApp.executableSections.push(new model.ExecutableSection(location, []));

    foundApp.components.forEach(foundComponent =>
        {
            newApp.componentSections[0].components.push(convertComponent(jsonDoc,
                                                                         appDefPath,
                                                                         foundComponent));
        });

    foundApp.executables.forEach(foundExecutable =>
        {
            let newLoc = new Location(foundApp.path);
            let newExe = new model.Executable(foundExecutable.name, newLoc, []);

            foundExecutable.components.forEach(foundComponent =>
                {
                    newExe.componentRefs.push(convertComponent(jsonDoc,
                                                               appDefPath,
                                                               foundComponent));
                });

            newApp.executableSections[0].executables.push(newExe);
        });

    return newApp;
}



//--------------------------------------------------------------------------------------------------
/**
 * Load the Legato model from the given json document.
 */
//--------------------------------------------------------------------------------------------------
export function parseSystem(jsonDoc: jdoc.Document,
                            sysDefPath: string,
                            isIncluded: boolean): model.System
{
    let tokenList = convertTokensFromJson(sysDefPath, jsonDoc.tokenMap[sysDefPath]);
    let buffer = new parser.TokenBuffer(tokenList,
                                        defTokens.EndOfFile,
                                        [
                                            defTokens.Comment,
                                            defTokens.Whitespace,
                                            defTokens.Directive
                                        ]);

    let result = systemDefinition.pull(buffer);
    let system = result.data.value as model.System;

    for (let appSections of system.appSections)
    {
        for (let appRef of appSections.apps)
        {
            let appDefPath = findFile(jsonDoc, path.basename(appRef.name));

            if (appDefPath !== undefined)
            {
                let app = convertApp(jsonDoc, appDefPath, isIncluded);

                if (app !== undefined)
                {
                    appRef.target = app;
                }
            }
        }
    }

    return system;
}
