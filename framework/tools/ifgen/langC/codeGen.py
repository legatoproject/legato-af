#
# C code generator functions
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#

import os
import cStringIO
import collections
import re

import common
import codeTypes

import codeGenCommon
import codeGenHeader
import codeGenClient
import codeGenServer



#---------------------------------------------------------------------------------------------------
# Main codegen functions
#---------------------------------------------------------------------------------------------------

def GetOutputFileNames(prefix):
    # Define the default names
    fileNames = [ "interface.h", "messages.h", "client.c", "server.c", "server.h" ]

    # If there is a file prefix, then make sure not to overwrite any existing files.
    # todo: Should we still check if the output file is getting overwritten.  This is
    #       causing problems with usage, and so the check has been commented out for now.
    if prefix:
        for i, fn in enumerate(fileNames):
            newfn = "%s_%s" % (prefix, fn)

            #if os.path.exists(newfn):
            #    print "ERROR: %s exists" % newfn
            #    sys.exit(1)

            # Store back the new file name with the prefix
            fileNames[i] = newfn

    return fileNames



def ExtractCommentString(header):
    """Extract the comment string from a doxgen-style function header
    """

    # Split into lines, stripping away any leading or trailing white space.
    lines = [ l.strip() for l in header.strip().splitlines() ]
    if not lines:
        return ''

    # The first line should be the comment opener, the last line should be the comment closer,
    # and everything in between should be the comment lines.  Do some basic sanity checks, and
    # return empty string if they don't pass.
    if not lines[0].startswith('/*') or not lines[-1].endswith('*/'):
        return ''

    newLines = []
    for l in lines[1:-1]:
        # Check for leading '* ' or '*', in that order, and strip them off to get the actual text.
        # If neither is found, then just use the whole line, as is.  Checking for '*' without the
        # trailing ' ' is done in case somebody has a typo and forgot to add the ' '.
        if l[0:2] == '* ':
            newL = l[2:]
        elif l[0:1] == '*':
            newL = l[1:]
        else:
            newL = l
        newLines.append(newL)

    commentString = '\n'.join( newLines )
    return commentString



def PostProcessParmListWithHandler(parmList):
    """
    Convert a HandlerParmData parameter to appropriate handlerPtr and contextPtr parameters.
    """

    newParmList = []

    for p in parmList:
        if not hasattr(p, 'isHandler'):
            # If the previous parameter was an array, then the current parameter will be the
            # length of the array, so skip it.  This is because the length parameter will get
            # generated again when we create a new FunctionData object below.
            if newParmList and isinstance( newParmList[-1], codeTypes.ArrayParmData ):
                continue

            newParmList.append(p)

        else:
            # Create new callback pointer and context pointer parameters.
            #
            # Note that we expect to always get here at least once, because otherwise this function
            # would not have been called.  Also, we should really only get here once, but that is
            # not explicitly handled yet.

            funcParm = codeTypes.HandlerPointerParmData( p.baseName, p.baseType )
            contextParm = codeTypes.SimpleParmData( "contextPtr", codeTypes.RawType("void*") )
            newParmList += [ funcParm, contextParm ]

    return newParmList, funcParm



def PostProcessEventFunc(f, flist, tlist):
    """
    Define the AddHandler and RemoveHandler functions and add them to the function list.

    """

    #print f
    #print f.parmList

    # The baseName for the type and functions defined here is the EVENT name + 'Handler'.
    newBaseName = f.baseName
    if not newBaseName.endswith('Handler'):
        newBaseName += 'Handler'

    #
    # Create the AddHandler/RemoveHandler ref type, which is returned by the AddHandler and used
    # by the RemoveHandler.
    #
    handlerRefType = codeTypes.ReferenceData(
        newBaseName,
        codeGenCommon.FormatHeaderComment(
            "Reference type used by Add/Remove functions for EVENT '%s'" % f.name))
    tlist[f.name] = handlerRefType
    #print handlerRefType
    #print handlerRefDefn

    #
    # Create the AddHandler and add it to the function list
    #

    addParmList, addParm = PostProcessParmListWithHandler(f.parmList)

    # Construct a comment string that combines the EVENT comment, with the canned comment for
    # the AddHandler function.  Hopefully the EVENT comment will be worded in such a way that
    # it is readable when combined.
    newComment = "Add handler function for EVENT '%s'" % f.name
    commentString = ExtractCommentString( f.comment )
    if commentString:
        newComment += ( "\n\n" + commentString )

    addHandler = codeTypes.FunctionData(
        "Add" + newBaseName,
        handlerRefType.baseName,
        addParmList,
        codeGenCommon.FormatHeaderComment(newComment)
    )

    # The parameter needs to know the name of the enclosing function.  Also, the function itself
    # needs to know that it is an AddHandler function, and the name/type of the handler that it
    # is adding.
    addParm.setFuncName(addHandler.name, addHandler.type)
    addHandler.handlerName = addParm.type
    addHandler.isAddHandler = True

    # Add the function to the function list
    #print addHandler
    flist[addHandler.name] = addHandler

    #
    # Create the RemoveHandler and add it to the function list
    #

    # Need to use RawType(refName) here because of a name conflict between handler types and other
    # defined types such as the reference created above.
    #
    # This conflict only occurs with parameters and not return types, because the handler types
    # mapping is only accessed for parameters. This is why baseName can be used with the AddHandler
    # function above.
    #
    # TODO: Is there a better way to handle this conflict. Note that this only happens for internally
    #       generated references.  For types in the API file, this should cause an error when the
    #       file is parsed, although that is not enforced yet.
    #
    removeParm = codeTypes.SimpleParmData( "addHandlerRef", codeTypes.RawType(handlerRefType.refName) )

    # todo: This needs to be implemented better
    # As a temporary solution, directly modify the templates for the parameter.
    # Note that the original clientPack is still needed, so prepend to it.
    removeParm.clientPack = """\
// The passed in handlerRef is a safe reference for the client data object.  Need to get the
// real handlerRef from the client data object and then delete both the safe reference and
// the object since they are no longer needed.
_LOCK
_ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
LE_FATAL_IF(clientDataPtr==NULL, "Invalid reference");
le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
_UNLOCK
addHandlerRef = ({parm.parmType})clientDataPtr->handlerRef;
le_mem_Release(clientDataPtr);
""" + removeParm.clientPack

    # After we have unpacked the parameter, need to use it to look up data, so append the actions
    # to the original serverUnpack.
    # Note: the four sets of braces for the if block are needed because this string is processed
    # twice using str.format()
    removeParm.serverUnpack += """
// The passed in handlerRef is a safe reference for the server data object.  Need to get the
// real handlerRef from the server data object and then delete both the safe reference and
// the object since they are no longer needed.
_LOCK
_ServerData_t* serverDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
if ( serverDataPtr == NULL )
{{{{
    _UNLOCK
    LE_KILL_CLIENT("Invalid reference");
    return;
}}}}
le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
_UNLOCK
addHandlerRef = ({func.type})serverDataPtr->handlerRef;
le_mem_Release(serverDataPtr);
""".format(func=addHandler)

    removeHandler = codeTypes.FunctionData(
        "Remove" + newBaseName,
        "",
        [ removeParm ],
        codeGenCommon.FormatHeaderComment("Remove handler function for EVENT '%s'" % f.name)
    )
    removeHandler.isRemoveHandler = True

    # Add the function to the function list
    flist[removeHandler.name] = removeHandler



def PostProcessFuncWithHandler(f):
    """
    Process a function containing a the handler parameter
    """

    #print f
    #print f.parmList

    # Convert the handler parameter
    newParmList, funcParm = PostProcessParmListWithHandler(f.parmList)

    # Create the new function with newParmList, rather than just over-riding parmList on the old
    # function, to ensure everything gets inited correctly.
    newFunc = codeTypes.FunctionData(
        f.baseName,
        f.baseType,
        newParmList,
        f.comment
    )

    # The parameter needs to know the name of the enclosing function.  Also, the function itself
    # needs to know that it has a handler parameter, as well has the handler name.
    funcParm.setFuncName(newFunc.name, newFunc.type)
    newFunc.handlerName = funcParm.type

    return newFunc



def WriteAllCode(commandArgs, parsedData, hashValue):

    # Extract the different types of data
    headerComments = parsedData['headerList']
    parsedCode = parsedData['codeList']
    importList = parsedData['importList']

    # Split the parsed data into three lists, and insert any AddHandlers/RemoveHandlers as needed
    # todo: I used to need an OrderedDict, so that I could easily look up functions by name.
    #       Not sure if I still need this functionality, so maybe I could just use a list here,
    #       although I would have to update the code to expect a list instead of a dictionary.
    parsedFunctions = collections.OrderedDict()
    parsedHandlers = collections.OrderedDict()
    parsedTypes = collections.OrderedDict()

    for f in parsedCode:
        if isinstance( f, codeTypes.HandlerFuncData ):
            parsedHandlers[f.name] = f
        elif isinstance( f, codeTypes.EventFuncData ):
            PostProcessEventFunc( f, parsedFunctions, parsedTypes )
        elif isinstance( f, codeTypes.BaseTypeData ):
            parsedTypes[f.name] = f
        else:
            # If the function has a handler parameter, then post-process. It is expected that
            # there is only one such parameter.
            # todo: this may need to be revisited later.
            for p in f.parmList:
                if hasattr( p, 'isHandler' ):
                    f = PostProcessFuncWithHandler( f )
                    break
            parsedFunctions[f.name] = f

    # Create the generic client functions
    startClientFunc = codeTypes.FunctionData(
        "ConnectService",
        "",
        [],
        codeGenCommon.FormatHeaderComment("""
Connect the current client thread to the service providing this API. Block until the service is
available.

For each thread that wants to use this API, either ConnectService or TryConnectService must be
called before any other functions in this API.  Normally, ConnectService is automatically called
for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.

This function is created automatically.
""")
    )

    tryStartClientFunc = codeTypes.FunctionData(
        "TryConnectService",
        "le_result_t",
        [],
        codeGenCommon.FormatHeaderComment("""
Try to connect the current client thread to the service providing this API. Return with an error
if the service is not available.

For each thread that wants to use this API, either ConnectService or TryConnectService must be
called before any other functions in this API.  Normally, ConnectService is automatically called
for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.

This function is created automatically.

@return
 - LE_OK if the client connected successfully to the service.
 - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 - LE_COMM_ERROR if the Service Directory cannot be reached.
""")
    )

    stopClientFunc = codeTypes.FunctionData(
        "DisconnectService",
        "",
        [],
        codeGenCommon.FormatHeaderComment("""
Disconnect the current client thread from the service providing this API.

Normally, this function doesn't need to be called. After this function is called, there's no
longer a connection to the service, and the functions in this API can't be used. For details, see
@ref apiFilesC_client.

This function is created automatically.
""")
    )

    genericInterfaceFunctions = collections.OrderedDict( ( ('startClientFunc',    startClientFunc),
                                                           ('tryStartClientFunc', tryStartClientFunc),
                                                           ('stopClientFunc',     stopClientFunc) ) )

    # Create the generic server functions
    startServerFunc = codeTypes.FunctionData(
        "AdvertiseService",
        "",
        [],
        codeGenCommon.FormatHeaderComment("Initialize the server and advertise the service.")
    )

    getServiceRef = codeTypes.FunctionData(
        "GetServiceRef",
        codeTypes.RawType("le_msg_ServiceRef_t"),
        [],
        codeGenCommon.FormatHeaderComment("Get the server service reference")
    )

    getSessionRef = codeTypes.FunctionData(
        "GetClientSessionRef",
        codeTypes.RawType("le_msg_SessionRef_t"),
        [],
        codeGenCommon.FormatHeaderComment("Get the client session reference for the current message")
    )

    genericServerFunctions = collections.OrderedDict( ( ('getServiceRef',   getServiceRef),
                                                        ('getSessionRef',   getSessionRef),
                                                        ('startServerFunc', startServerFunc) ) )
    # Get the output file names
    outputFileList = GetOutputFileNames(commandArgs.filePrefix)
    interfaceFname, localFname, clientFname, serverFname, serverIncludeFname = outputFileList

    # Map the imported files to the appropriate include file names.  Use the client version for
    # the client header file, and server version for the server header file.
    clientImportList = [ GetOutputFileNames(i.name)[0] for i in importList ]
    serverImportList = [ GetOutputFileNames(i.name)[4] for i in importList ]

    # If the output directory option was specified, then prepend to the output files names
    if commandArgs.outputDir:
        if not os.path.exists(commandArgs.outputDir):
            os.makedirs(commandArgs.outputDir)
        outputFileList = [ os.path.join( commandArgs.outputDir, fn ) for fn in outputFileList ]
    interfaceFpath, localFpath, clientFpath, serverFpath, serverIncludeFpath = outputFileList

    # If requested, write the contents to the corresponding buffers, and then write the buffers to
    # the real files, overwriting any existing files
    if commandArgs.genInterface:
        # If there are no functions or handlers defined, then don't put the generic interface
        # function prototypes in the interface header file.
        if not parsedFunctions and not parsedHandlers:
            genericFunctions = {}
        else:
            genericFunctions = genericInterfaceFunctions

        interfaceHeaderFileText = codeGenHeader.WriteInterfaceHeaderFile(
                                     parsedFunctions,
                                     parsedHandlers,
                                     parsedTypes,
                                     clientImportList,
                                     genericFunctions,
                                     interfaceFname,
                                     headerComments)
        open(interfaceFpath, 'w').write( interfaceHeaderFileText.getvalue() )

    if commandArgs.genLocal:
        localHeaderFileText = codeGenHeader.WriteLocalHeaderFile(
                                     parsedFunctions,
                                     parsedHandlers,
                                     hashValue,
                                     localFname,
                                     commandArgs.serviceName)
        open(localFpath, 'w').write( localHeaderFileText.getvalue() )

    if commandArgs.genClient:
        clientFileText = codeGenClient.WriteClientFile(
                                     [localFname, interfaceFname],
                                     parsedFunctions,
                                     parsedHandlers,
                                     genericInterfaceFunctions)
        open(clientFpath, 'w').write( clientFileText.getvalue() )

    if commandArgs.genServerInterface:
        # If there are no functions or handlers defined, then don't put the generic server
        # function prototypes in the server interface header file.
        if not parsedFunctions and not parsedHandlers:
            genericFunctions = {}
        else:
            genericFunctions = genericServerFunctions

        serverIncludeFileText = codeGenHeader.WriteServerHeaderFile(
                                     parsedFunctions,
                                     parsedHandlers,
                                     parsedTypes,
                                     serverImportList,
                                     genericFunctions,
                                     # TODO: temporary workaround for naming conflicts.
                                     interfaceFname,
                                     # serverIncludeFname,
                                     commandArgs.async)
        open(serverIncludeFpath, 'w').write( serverIncludeFileText.getvalue() )

    if commandArgs.genServer:
        serverFileText = codeGenServer.WriteServerFile(
                                     [localFname, serverIncludeFname],
                                     parsedFunctions,
                                     parsedHandlers,
                                     genericServerFunctions,
                                     commandArgs.async)
        open(serverFpath, 'w').write( serverFileText.getvalue() )


