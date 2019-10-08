
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
import * as model from './model/annotatedModel';
import * as conversion from './vscTypeConvert';



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
                definitionProvider: false,
                documentHighlightProvider: false,
                documentSymbolProvider: false,
                foldingRangeProvider: false,

                textDocumentSync: lsp.TextDocumentSyncKind.Full,

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
        Client.profile.environment = settings;
    });



Client.connection.onShutdown(() =>
    {
    });



Client.connection.onExit(() =>
    {
    });



// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
//   Workspace folder event handlers.
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------



// -------------------------------------------------------------------------------------------------
//   Text sync event handlers.
// -------------------------------------------------------------------------------------------------

Client.connection.onDidOpenTextDocument((params: lsp.DidOpenTextDocumentParams) =>
    {
    });

Client.connection.onDidChangeTextDocument((params: lsp.DidChangeTextDocumentParams) =>
    {
    });

Client.connection.onWillSaveTextDocument((params: lsp.WillSaveTextDocumentParams) =>
    {
    });

Client.connection.onWillSaveTextDocumentWaitUntil((params: lsp.WillSaveTextDocumentParams): lsp.TextEdit[] =>
    {
        return undefined;
    });

Client.connection.onDidSaveTextDocument((params: lsp.DidSaveTextDocumentParams) =>
    {
    });

Client.connection.onDidCloseTextDocument((params: lsp.DidCloseTextDocumentParams) =>
    {
    });



// -------------------------------------------------------------------------------------------------
//   Language feature event handlers.
// -------------------------------------------------------------------------------------------------

Client.connection.onCompletion((params: lsp.CompletionParams): lsp.CompletionList =>
    {
        return undefined;
    });

 Client.connection.onCompletionResolve((params: lsp.CompletionItem): lsp.CompletionItem =>
    {
        return undefined;
    });

Client.connection.onDocumentSymbol(
    (params: lsp.DocumentSymbolParams): lsp.SymbolInformation[] | lsp.DocumentSymbol[] =>
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

Client.connection.onFoldingRanges((params: lsp.FoldingRangeRequestParam): lsp.FoldingRange[] =>
    {
        return undefined;
    });



// -------------------------------------------------------------------------------------------------
//   Workspace event handlers.
// -------------------------------------------------------------------------------------------------

Client.connection.onWorkspaceSymbol((params: lsp.WorkspaceSymbolParams): lsp.SymbolInformation[] =>
    {
        let filePaths: string[] = [];

        switch ('.' + params.query)
        {
            case model.DefType.systemDef:
                filePaths = Client.profile.availableSystems;
                break;

            case model.DefType.applicationDef:
                filePaths = Client.profile.availableApps;
                break;

            case model.DefType.componentDef:
                filePaths = Client.profile.availableComponents;
                break;

            case model.DefType.apiDef:
                filePaths = Client.profile.availableInterfaces;
                break;
        }

        return conversion.FilePathsToSymbolInformation(filePaths);
    });



// -------------------------------------------------------------------------------------------------
//   Core extension protocol handlers.
// -------------------------------------------------------------------------------------------------

const ExtensionVersion = "0.2";


Client.connection.onRequest("le_GetExtensionProtocolVersion",
    (): string =>
    {
        return ExtensionVersion;
    });

Client.connection.onRequest("le_SetClientExtensionProtocolVersion",
    (requestedVersion: string): boolean =>
    {
        return true;
    });


// -------------------------------------------------------------------------------------------------
//   Legato system extension protocol handlers.
// -------------------------------------------------------------------------------------------------

Client.connection.onRequest("le_GetLogicalView",
    (): any =>
    {
        let flags: conversion.ConversionFlags = { supportsDefaultCollapsed: false };
        return conversion.SystemAsSymbol(flags, Client.profile.activeModel as model.System, []);
    });

Client.connection.onRequest("le_registerModelUpdates",
    () =>
    {
        try
        {
            if (Client.profile.clientProperties.supportsModelUpdates)
            {
                Client.profile.receiveModelUpdates = true;
            }
        }
        catch (e)
        {
            console.log('  le_registerModelUpdates Exception: ' + e);
            console.error(e.stack);
        }
    });

Client.connection.onRequest("le_unregisterModelUpdates",
    () =>
    {
        if (Client.profile.clientProperties.supportsModelUpdates)
        {
            Client.profile.receiveModelUpdates = false;
        }
    });

Client.connection.onRequest("le_listDefinitions",
    (params: ext.le_ListAvailableDefsRequest): ext.le_ListAvailableDefsResponse =>
    {
        let response: ext.le_ListAvailableDefsResponse = { definitions: [] };

        switch (params.defType)
        {
            case ext.le_DefinitionType.sdef:
                response.definitions = Client.profile.availableSystems.map(
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
                response.definitions = Client.profile.availableApps.map(
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
