
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
            Client.log("Language server onInitialize.");

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

            Client.log("LEGATO_ROOT:" + Client.profile.legatoRoot);
        }
        catch (e)
        {
            Client.log('exception: ' + e);
        }

        return {
            capabilities:
            {
                // codeActionProvider: true,
                // documentFormattingProvider: true,
                definitionProvider: true,
                documentHighlightProvider: true,
                documentSymbolProvider: true,
                foldingRangeProvider: true,

                textDocumentSync: lsp.TextDocumentSyncKind.Full,

                completionProvider:
                {
                    resolveProvider: true
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
        Client.log("Language server initialized.");

        if (Client.capabilities.hasWorkspaces)
        {
            // Get the current list of workspace folders from the client.
            Client.connection.workspace.getWorkspaceFolders().then(
                // On a successful request...
                (folders: lsp.WorkspaceFolder[]) =>
                {
                    Client.log('Received workspace folders.');
                    Client.profile.workspaceFolders = folders;

                    for (let folder of Client.profile.workspaceFolders)
                    {
                        Client.log('  ' + folder.name + ": " + folder.uri);
                    }
                },

                // On a failed request...
                (reason: any) =>
                {
                    Client.log('The folder request was rejected.');
                    Client.log('Reason: ' + reason);
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
        Client.log('Client.connection.onDidChangeConfiguration');
        Client.profile.environment = settings;
    });



Client.connection.onShutdown(() =>
    {
    });



Client.connection.onExit(() =>
    {
        //
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
        Client.log('Client.connection.onDidOpenTextDocument');
        Client.log('  params.textDocument.languageId: ' + params.textDocument.languageId);
        Client.log('  params.textDocument.uri:        ' + params.textDocument.uri);
        Client.log('  params.textDocument.version:    ' + params.textDocument.version.toString());
        //Client.log('  params.textDocument.text:       ' + params.textDocument.text);
    });

Client.connection.onDidChangeTextDocument((params: lsp.DidChangeTextDocumentParams) =>
    {
        Client.log('Client.connection.onDidChangeTextDocument');
    });

Client.connection.onWillSaveTextDocument((params: lsp.WillSaveTextDocumentParams) =>
    {
        Client.log('Client.connection.onWillSaveTextDocument');
    });

Client.connection.onWillSaveTextDocumentWaitUntil((params: lsp.WillSaveTextDocumentParams): lsp.TextEdit[] =>
    {
        Client.log('Client.connection.onWillSaveTextDocumentWaitUntil');
        return null;
    });

Client.connection.onDidSaveTextDocument((params: lsp.DidSaveTextDocumentParams) =>
    {
        Client.log('Client.connection.onDidSaveTextDocument');

        //if (Client.profile.uriInActiveModel(params.textDocument.uri))
        //{
        //    Client.log('The model has been modified, reloading now...');
        //    Client.profile.reloadActiveModel();
        //    Client.log('Load complete.');
        //}
    });

Client.connection.onDidCloseTextDocument((params: lsp.DidCloseTextDocumentParams) =>
    {
        Client.log('Client.connection.onDidCloseTextDocument');
        Client.log('  params.textDocument.uri: ' + params.textDocument.uri);
    });



// -------------------------------------------------------------------------------------------------
//   Language feature event handlers.
// -------------------------------------------------------------------------------------------------

Client.connection.onCompletion((params: lsp.CompletionParams): lsp.CompletionList =>
    {
        return null;
    });

 Client.connection.onCompletionResolve((params: lsp.CompletionItem): lsp.CompletionItem =>
    {
        return null;
    });

// Client.connection.onHover(() => {});
// Client.connection.onSignatureHelp(() => {});
// Client.connection.onDefinition(() => {});
// Client.connection.onTypeDefinition(() => {});
// Client.connection.onImplementation(() => {});
// Client.connection.onReferences(() => {});
// Client.connection.onDocumentHighlight(() => {});

Client.connection.onDocumentSymbol(
    (params: lsp.DocumentSymbolParams): lsp.SymbolInformation[] | lsp.DocumentSymbol[] =>
    {
        Client.log('Client.connection.onDocumentSymbol');
        Client.log('  params:                  ' + params);
        Client.log('  params.textDocument:     ' + params.textDocument);
        Client.log('  params.textDocument.uri: ' + params.textDocument.uri);

        if (!(Client.profile.activeModel instanceof model.System))
        {
            return [];
        }

        if (Client.capabilities.hasHierarchicalDocumentSymbol)
        {
            //return [conversion.SystemAsSymbol(Client.profile.activeModel)] as lsp.DocumentSymbol[];
        }

        return [];
    });

//Client.connection.onCodeAction((params: lsp.CodeActionParams): (lsp.Command | lsp.CodeAction)[] =>
//    {
//        Client.log('Client.connection.onCodeAction');
//        return [];
//    });

// Client.connection.onCodeLens(() => {});
// Client.connection.onCodeLensResolve(() => {});
// Client.connection.onDocumentLinks(() => {});
// Client.connection.onDocumentLinkResolve(() => {});
// Client.connection.onDocumentColor(() => {});
// Client.connection.onColorPresentation(() => {});
// Client.connection.onDocumentFormatting(() => {});
// Client.connection.onDocumentRangeFormatting(() => {});
// Client.connection.onDocumentOnTypeFormatting(() => {});
// Client.connection.onRenameRequest(() => {});
// Client.connection.onPrepareRename(() => {});

Client.connection.onFoldingRanges((params: lsp.FoldingRangeRequestParam): lsp.FoldingRange[] =>
    {
        Client.log('Client.connection.onFoldingRanges');

        let filePath = lsp.Files.uriToFilePath(params.textDocument.uri);
        Client.log('   path: ' + filePath);

        return undefined;
    });



// -------------------------------------------------------------------------------------------------
//   Core extension protocol handlers.
// -------------------------------------------------------------------------------------------------

const ExtensionVersion = "0.2";


Client.connection.onRequest("le_GetExtensionProtocolVersion",
    (): string =>
    {
        Client.log("Warning, client using deprecated API.");
        return ExtensionVersion;
    });

Client.connection.onRequest("le_SetClientExtensionProtocolVersion",
    (requestedVersion: string): boolean =>
    {
        Client.log("Warning, client using deprecated API.");
        return true;
    });


// -------------------------------------------------------------------------------------------------
//   Legato system extension protocol handlers.
// -------------------------------------------------------------------------------------------------

Client.connection.onRequest("le_GetLogicalView",
    (): any =>
    {
        Client.log("Warning, client using deprecated API.");
        let flags: conversion.ConversionFlags = { supportsDefaultCollapsed: false };
        return conversion.SystemAsSymbol(flags, Client.profile.activeModel as model.System);
    });

Client.connection.onRequest("le_registerModelUpdates",
    () =>
    {
        if (Client.profile.clientProperties.supportsModelUpdates)
        {
            Client.profile.receiveModelUpdates = true;
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
