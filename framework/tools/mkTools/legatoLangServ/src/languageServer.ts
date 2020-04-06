
//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Legato language server.  We create and manage our connection to the
 * language client here.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

"use strict";

import * as lsp from 'vscode-languageserver';
import * as path from 'path';
import Uri from 'vscode-uri';

import * as ext from './lspExtensionDefs';
import * as lspCli from './lspClient';
import * as fsUtil from './fsUtil';
import * as model from './model/annotatedModel';
import * as conversion from './vscTypeConvert';
import * as nav from './model/dataNavigation';



//--------------------------------------------------------------------------------------------------
/**
 * An object to represent our connection to the client.  This connection is not actively established
 * until all of our event handlers have been registered.
 * let Connection = lsp.createConnection(lsp.ProposedFeatures.all);
 */
//--------------------------------------------------------------------------------------------------
const Client = new lspCli.LspClient();



// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
//   Connection event handlers.
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
/**
 * Handler called when we are attempting to initialize the connection to the client.
 */
//--------------------------------------------------------------------------------------------------
Client.connection.onInitialize((params: lsp.InitializeParams): lsp.InitializeResult =>
    {
        try
        {
            console.log("Language server onInitialize.");

            // Here the client has told us the protocol features that it supports.
            Client.capabilities.hasWorkspaces =    params.capabilities.workspace
                                                && !!params.capabilities.workspace.workspaceFolders;

            Client.capabilities.hasHierarchicalDocumentSymbol =
                params.capabilities
                      .textDocument
                      .documentSymbol
                      .hierarchicalDocumentSymbolSupport;

            // Specify workspace folder
            Client.profile.workspaceFolders = params.workspaceFolders;

            // Hopefully the client has also supplied us their full extension properties set as
            // well.
            if ('env' in params.initializationOptions)
            {
                Client.profile.clientProperties =
                                 params.initializationOptions as ext.le_ExtensionClientCapabilities;
            }
            else
            {
                // Looks like it's an older style init, so assume they don't support model updates
                // and try to populate or environment map.
                Client.profile.clientProperties =
                    {
                        env: params.initializationOptions as NodeJS.ProcessEnv,
                        supportsModelUpdates: false
                    }
            }
        }
        catch (e)
        {
            console.log('  exception: ' + e);
        }

        return {
            capabilities:
            {
                workspaceSymbolProvider: true,
                documentSymbolProvider: true,
                hoverProvider: true,

                textDocumentSync:
                    {
                        openClose: true,
                        change: lsp.TextDocumentSyncKind.Full,
                        willSave: false,
                        willSaveWaitUntil: false,
                        save: { includeText: false }
                    },

                completionProvider:
                    {
                        resolveProvider: false
                    }
            }
        };
    });



//--------------------------------------------------------------------------------------------------
/**
 * Now we are informed that the connection has been fully initialized.
 */
//--------------------------------------------------------------------------------------------------
Client.connection.onInitialized(() =>
    {
        console.log("Language server initialized.");

        if (Client.capabilities.hasWorkspaces)
        {
            // Get the current list of workspace folders from the client.
            Client.connection.workspace.getWorkspaceFolders().then(
                // On a successful request...
                (folders: lsp.WorkspaceFolder[]) =>
                {
                    Client.profile.workspaceFolders = folders;
                },

                // On a failed request...
                (reason: any) =>
                {
                    console.log('The folder request was rejected.');
                    console.log('Reason: ' + reason);
                });

            // Register a handler that will be executed whenever the user adds or removes folders
            // from their workspace.
            Client.connection.workspace.onDidChangeWorkspaceFolders(
                (params: lsp.WorkspaceFoldersChangeEvent) =>
                {
                    for (let removed of params.removed)
                    {
                        const index = Client.profile.workspaceFolders.indexOf(removed);

                        if (index > -1)
                        {
                            Client.profile.workspaceFolders.splice(index, 1);
                        }
                    }

                    for (let added of params.added)
                    {
                        Client.profile.workspaceFolders.push(added);
                    }
                });
            }
    });



Client.connection.onDidChangeConfiguration((settings: any) =>
    {
        if (settings["PATH"])
        {
            Client.profile.environment = settings;
        }
    });



Client.connection.onShutdown(() =>
    {
    });



Client.connection.onExit(() =>
    {
    });



// -------------------------------------------------------------------------------------------------
/**
 * Record an exception that occurred to the language server log.
 *
 * @param error The exception object itself.
 */
// -------------------------------------------------------------------------------------------------
function RecordError(error: any)
{
    console.error(error.message);
    console.error('Call stack:\n' + error.stack);
}



// -------------------------------------------------------------------------------------------------
/**
 * This type represents the functions that actually handle the language server protocol events.
 *
 * All handlers take either no arguments or a single argument that is a structure broken down into
 * all the actual parameters to handle.  They may or may not return a value so they may be functions
 * that return 'undefined.'
 */
// -------------------------------------------------------------------------------------------------
type RegHandlerType<PT, RT> = (arg: PT) => RT;



// -------------------------------------------------------------------------------------------------
/**
 * A function that knows how to register a handler for a given protocol sync point.
 */
// -------------------------------------------------------------------------------------------------
type RegFunctionType<PT, RT> = (handler: RegHandlerType<PT, RT>) => void;



// -------------------------------------------------------------------------------------------------
/**
 * Custom protocol handlers are registered by name.  Standard protocol handlers are registered by
 * function handle.  This type allows our binding functions to take both.
 */
// -------------------------------------------------------------------------------------------------
type RegType<PT, RT> = string | RegFunctionType<PT, RT>;



// -------------------------------------------------------------------------------------------------
/**
 * A type guard to determine if the given argument is either a registration handler or a string.
 *
 * Returns: A true if the argument is a string, false otherwise.
 *
 * @param arg Is this a registration handler or is it a string object?
 */
// -------------------------------------------------------------------------------------------------
function isString<PT, RT>(arg: RegType<PT, RT>): arg is string
{
    return typeof arg === "string";
}



// -------------------------------------------------------------------------------------------------
/**
 * Wrap an event handler in a simple default try catch handler.  This catch handler will simply dump
 * the error information to the VS code console and return the default value that was passed in.
 *
 * @param defaultResult What to return if the call fails.
 * @param registerFn The function that handles the event registration.
 * @param handler The user code that will do the actual work.
 */
// -------------------------------------------------------------------------------------------------
function RegValHandler<PT, RT>(defaultResult: RT,
                               registerFn: RegType<PT, RT>,
                               handler: (arg: PT) => RT)
{
    // Attempt to call the user handler, if any exception is thrown, log it.
    function TryHandler(arg: PT): RT
    {
        let result: RT = defaultResult;

        try
        {
            result = handler(arg);
        }
        catch (e)
        {
            RecordError(e);
        }

        return result;
    }

    // If registerFn is a string, it's a custom protocol handler.  Otherwise it's a standard
    // protocol item.
    if (isString(registerFn))
    {
        Client.connection.onRequest(registerFn, TryHandler);
    }
    else
    {
        registerFn(TryHandler);
    }
}



// -------------------------------------------------------------------------------------------------
/**
 * Wrap an event handler in a simple default try catch handler.  If any exceptions occur, the error
 * information is reported on the console.
 *
 * @param registerFn The function that handles the event registration.
 * @param handler The actual work happens in this function.
 */
// -------------------------------------------------------------------------------------------------
function RegHandler<PT>(registerFn: RegType<PT, void>, handler: (arg: PT) => void)
{
    // Attempt to call the user handler, if any exception is thrown, log it.
    function TryHandler(arg: PT)
    {
        try
        {
            handler(arg);
        }
        catch (e)
        {
            RecordError(e);
        }
    }

    // See if we are registering a builtin handler, or a custom protocol handler.
    if (isString(registerFn))
    {
        Client.connection.onRequest(registerFn, TryHandler);
    }
    else
    {
        registerFn(TryHandler);
    }
}



// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
//   Workspace folder event handlers.
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------



// -------------------------------------------------------------------------------------------------
//   Text sync event handlers.
// -------------------------------------------------------------------------------------------------

RegHandler(Client.connection.onDidOpenTextDocument,
    (_params: lsp.DidOpenTextDocumentParams) =>
    {
    });

RegHandler(Client.connection.onDidChangeTextDocument,
    (_params: lsp.DidChangeTextDocumentParams) =>
    {
    });

RegHandler(Client.connection.onWillSaveTextDocument,
    (_params: lsp.WillSaveTextDocumentParams) =>
    {
    });

RegValHandler([], Client.connection.onWillSaveTextDocumentWaitUntil,
    (_params: lsp.WillSaveTextDocumentParams): lsp.TextEdit[] =>
    {
        return [];
    });

RegHandler(Client.connection.onDidSaveTextDocument,
    (_params: lsp.DidSaveTextDocumentParams) =>
    {
    });

RegHandler(Client.connection.onDidCloseTextDocument,
    (_params: lsp.DidCloseTextDocumentParams) =>
    {
    });



// -------------------------------------------------------------------------------------------------
//   Language feature event handlers.
// -------------------------------------------------------------------------------------------------

RegValHandler(undefined, Client.connection.onCompletion,
    (params: lsp.CompletionParams): lsp.CompletionList =>
    {
        console.log("On completion...");

        let filePathUri = params.textDocument.uri;
        let dirPath = path.dirname(path.normalize(Uri.parse(filePathUri).fsPath));
        let env = conversion.getClientLocalEnvironment(Client, dirPath);

        let results = conversion.envVarsToCompletion(env);

        let line = params.position.line + 1;
        let column = params.position.character;

        let docTokens = Client.profile.getDocTokens(filePathUri);

        if (docTokens === undefined)
        {
            return results;
        }

        let section = nav.getSectionType(dirPath, docTokens, line, column, true);

        if (section === undefined)
        {
            return results;
        }

        let workspace = Uri.parse(Client.profile.workspaceFolders[0].uri).fsPath;
        conversion.appendSectionCompletions(results, section, workspace);

        return results;
    });

RegValHandler(undefined, Client.connection.onCompletionResolve,
    (_params: lsp.CompletionItem): lsp.CompletionItem =>
    {
        return undefined;
    });

RegValHandler(undefined, Client.connection.onHover,
    (params: lsp.TextDocumentPositionParams): lsp.Hover | undefined =>
    {
        if (Client.profile.uriInActiveModel(params.textDocument.uri) === false)
        {
            return undefined;
        }

        let filePathUri = params.textDocument.uri;

        let line = params.position.line + 1;
        let column = params.position.character;

        let docTokens = Client.profile.getDocTokens(filePathUri);
        let dirPath = path.dirname(path.normalize(Uri.parse(filePathUri).fsPath));
        let section = nav.getSectionType(dirPath, docTokens, line, column);
        let env = conversion.getClientLocalEnvironment(Client, dirPath);

        if (section !== undefined)
        {
            if (section.onToken !== undefined)
            {
                let oldText = section.onToken.text;
                let newText = nav.expandText(oldText, env);

                if (oldText !== newText)
                {
                    return {
                            contents: newText,

                            range: {
                                start: {
                                    line: section.onToken.location.line - 1,
                                    character: section.onToken.location.column
                                },

                                end: {
                                    line: section.onToken.location.line - 1,
                                    character:   section.onToken.location.column
                                               + section.onToken.text.length
                                }
                            }
                        };
                }
            }
        }
        else
        {
            console.log("Point not in section.");
        }

        return undefined;
    });

RegValHandler([], Client.connection.onDocumentSymbol,
    (_params: lsp.DocumentSymbolParams): lsp.SymbolInformation[] | lsp.DocumentSymbol[] =>
    {
        if (!(Client.profile.activeModel instanceof model.System))
        {
            return [];
        }

        if (Client.capabilities.hasHierarchicalDocumentSymbol)
        {
        }

        return [];
    });

RegValHandler([], Client.connection.onFoldingRanges,
    (_params: lsp.FoldingRangeRequestParam): lsp.FoldingRange[] =>
    {
        return undefined;
    });



// -------------------------------------------------------------------------------------------------
//   Workspace event handlers.
// -------------------------------------------------------------------------------------------------

RegValHandler([], Client.connection.onWorkspaceSymbol,
    (params: lsp.WorkspaceSymbolParams): lsp.SymbolInformation[] =>
    {
        let filePaths: string[] = [];

        switch ('.' + params.query)
        {
            case model.DefType.systemDef:
                filePaths = Client.profile.getAvailableSystems();
                break;

            case model.DefType.applicationDef:
                let dirPath = path.dirname(
                    path.normalize(Uri.parse(Client.profile.workspaceFolders[0].uri).fsPath));

                filePaths = Client.profile.getAvailableApps();

                filePaths = filePaths.concat(fsUtil.recursiveFileFind(dirPath, '.adef'));
                filePaths = filePaths.concat(fsUtil.recursiveFileFind(dirPath, '.app'));
                break;

            case model.DefType.componentDef:
                filePaths = Client.profile.getAvailableComponents();
                break;

            case model.DefType.apiDef:
                filePaths = Client.profile.getAvailableInterfaces();
                break;
        }

        // Make sure that there are no duplicate paths.
        filePaths = [ ...new Set(filePaths) ];

        return conversion.FilePathsToSymbolInformation(filePaths);
    });



// -------------------------------------------------------------------------------------------------
//   Core extension protocol handlers.
// -------------------------------------------------------------------------------------------------

const ExtensionVersion = "0.2";


RegValHandler("", "le_GetExtensionProtocolVersion",
    (): string =>
    {
        return ExtensionVersion;
    });

RegValHandler(true, "le_SetClientExtensionProtocolVersion",
    (_requestedVersion: string): boolean =>
    {
        return true;
    });


// -------------------------------------------------------------------------------------------------
//   Legato system extension protocol handlers.
// -------------------------------------------------------------------------------------------------

RegValHandler([], "le_GetLogicalView",
    (): any =>
    {
        return conversion.SystemAsSymbol(Client.profile.activeModel as model.System);
    });

RegHandler("le_registerModelUpdates",
    () =>
    {
        if (Client.profile.clientProperties.supportsModelUpdates)
        {
            Client.profile.receiveModelUpdates = true;
        }
    });

RegHandler("le_unregisterModelUpdates",
    () =>
    {
        if (Client.profile.clientProperties.supportsModelUpdates)
        {
            Client.profile.receiveModelUpdates = false;
        }
    });

RegValHandler({ definitions: [] }, "le_listDefinitions",
    (params: ext.le_ListAvailableDefsRequest): ext.le_ListAvailableDefsResponse =>
    {
        let response: ext.le_ListAvailableDefsResponse = { definitions: [] };

        switch (params.defType)
        {
            case ext.le_DefinitionType.sdef:
                response.definitions = Client.profile.getAvailableSystems().map(
                    (value: string): ext.le_DefinitionFile =>
                    {
                        return {
                                "name": path.basename(value, model.DefType.systemDef),
                                "path": Uri.file(value).toString(),
                                "defType": ext.le_DefinitionType.sdef
                            };
                    });
                break;

            case ext.le_DefinitionType.adef:
                response.definitions = Client.profile.getAvailableApps().map(
                    (value: string): ext.le_DefinitionFile =>
                    {
                        return {
                                "name": path.basename(value, model.DefType.applicationDef),
                                "path": Uri.file(value).toString(),
                                "defType": ext.le_DefinitionType.adef
                            };
                    });
                break;
        }

        return response;
    });




// -------------------------------------------------------------------------------------------------
//   Connect to our client.
// -------------------------------------------------------------------------------------------------

Client.connection.listen();
