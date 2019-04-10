
//--------------------------------------------------------------------------------------------------
/**
 * Descriptions of the language server protocol extension functions as supported by the Legato
 * language server.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

'use strict';

import * as lsp from 'vscode-languageserver';



/** ------------------------------------------------------------------------------------------------
 *  Core Methods:
 *
 *  method:  le_GetExtensionProtocolVersion (Deprecated)
 *  desc:    Get the version of the extension protocol supported by the language server.
 *  params:  none
 *  returns: The version string, currently always "0.2".
 *
 *  method:  le_SetClientExtensionProtocolVersion
 *  desc:    Set the protocol provided by the language server.  This is used to optionally downgrade
 *           from the latest version of the protocol to something that the client is compatible
 *           with.  Only call this if you need to actually change the version of the protocol being
 *           used by the language server.
 *  params:  Currently only supports the setting of "0.1" or "0.2".
 *  returns: boolean
 *
 *  method:  le_SetExtensionClientCapabilities
 *  desc:    Allow the client to register what capabilities that it supports.
 *  params:  The set of feature flags that the client agrees to support.
 *  returns: none.
 * ---------------------------------------------------------------------------------------------- */

 /**
  * Set of optional flags that the client can fill in to register what features it supports.
  */
export interface le_ExtensionClientCapabilities
{
    /** The variables in the Leaf environment we're running under. */
    env: NodeJS.ProcessEnv;

    /** Does the client support Legato logical load updates? */
    supportsModelUpdates?: boolean;
}



/** ------------------------------------------------------------------------------------------------
 *  System Methods:
 *
 *  method:  le_GetLogicalView
 *  desc:    Return a logical view of the current Legato build model.  What is returned is
 *           influenced by the environment variable LEGATO_DEF_FILE as send from the language
 *           client.
 *  params:  None.
 *  returns: A tree of le_DefinitionObject(s).
 *
 *  method:  le_registerModelUpdates
 *  desc:    Call this function to inform the LSP server that the client wishes to receive logical
 *           model updates.  The default setting is false.
 *  params:  None.
 *  returns: Nothing.  However, the callback le_UpdateLogicalView will begin to be called.
 *
 *  method:  le_unregisterModelUpdates
 *  desc:    Called when the client no longer wishes to receive logical model updates.
 *  params:  None.
 *  returns: Nothing.
 *
 *  method:  le_listDefinitions
 *  desc:    Request a list of available definition files available to the current
 *           system/application.  Like le_GetLogicalView we use  to know what model
 *           to load and search.
 *  params:  le_ListAvailableDefsRequest, specifically we need to know what kind of definition file
 *           we are looking for.
 *  returns: A le_ListAvailableDefsResponse containing a list of the requested definition files.
 * ---------------------------------------------------------------------------------------------- */




 /**
  * Types of data objects that can be represented by a Le_DefinitionObject.
  */
export enum le_DefinitionObjectType
{
    DefinitionFile = lsp.SymbolKind.File,
    DefSection     = lsp.SymbolKind.Namespace,
    ApplicationRef = lsp.SymbolKind.Interface,
    ComponentRef   = lsp.SymbolKind.Class,
    ApiRef         = lsp.SymbolKind.Function
}

/**
  * A simple description of a Legato object that can be found within a loaded Legato build model.
  * */
export interface le_DefinitionObject
{
    name: string;
    kind: le_DefinitionObjectType;
    detail?: string;

    defaultCollapsed?: boolean;

    defPath: string;
    children?: le_DefinitionObject[];

    range: lsp.Range;
    selectionRange: lsp.Range;
}




/**
 * Enumeration that specifies the currently recognized definition file types.
 */
export enum le_DefinitionType
{
    sdef = "sdef",
    adef = "adef",
    cdef = "cdef",
    mdef = "mdef"
}

/**
 * Params for the function le_listDefinitions.
 */
export interface le_ListAvailableDefsRequest
{
    /** The type of definition file to search for. */
    defType: le_DefinitionType;
}

/**
 * Information about a Legato definition file.
 * */
export interface le_DefinitionFile
{
    /** Short name of the file, without extension. */
    name: string;
    /** Full file name and path. */
    path: string;
    /** What type of definition is this? */
    defType: le_DefinitionType;
}

/**
 * Response to the function le_listDefinitions.
 */
export interface le_ListAvailableDefsResponse
{
    /**
     * A list of definition files, or empty if none were found.  Will also return a empty list if
     * LEGATO_DEF_FILE is not set to an existing definition file.
     * */
    definitions: le_DefinitionFile[];
}
