#
# C code generator functions
#
# Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
#

import os
import sys
import time
import cStringIO
import collections
import re

import codeTypes

# The new template system
import jinja2


#---------------------------------------------------------------------------------------------------
# Globals
#---------------------------------------------------------------------------------------------------

# Contains the text for the client and handler files.
# These will be written out to actual files
ClientFileText = cStringIO.StringIO()
ServerFileText = cStringIO.StringIO()
ServerIncludeFileText = cStringIO.StringIO()
LocalHeaderFileText = cStringIO.StringIO()
InterfaceHeaderFileText = cStringIO.StringIO()


#---------------------------------------------------------------------------------------------------
# Template related definitions and functions
#---------------------------------------------------------------------------------------------------

# This environment is shared by all templates
# Note that '$' is used as a line statement prefix, instead of '#', since some templates have
# C macro definitions, which start with '#'.
Environment = jinja2.Environment(trim_blocks=True, line_statement_prefix='$')
TemplateCache = {}

def FormatCode(templateString, **dataArgs):
    # Get the template from the cache, or create and cache it, if not in the cache.
    # This provides a noticeable improvement in processing speed, especially with large files.
    if templateString in TemplateCache:
        template = TemplateCache[templateString]
    else:
        template = Environment.from_string(templateString)
        TemplateCache[templateString] = template

    # Apply the template
    result = template.render(dataArgs)

    # Add back the trailing newline that gets stripped by render()
    result += '\n'

    # Remove trailing spaces
    result = re.sub(' +$', '', result, flags=re.M)

    return result


#
# Define and register the filter for processing a parameter list with the given parameter template
#
def PrintParmList(parmList, templateName, sep=',\n', leadSep=False):
    resultList = [ getattr(p, templateName).format(parm=p)
                       for p in parmList if not isinstance(p, codeTypes.VoidData) ]
    resultString = sep.join( resultList )

    # If there are parameters to be printed, and if requested, then prepend the separator.
    if resultString and leadSep:
        resultString = sep + resultString

    return resultString

Environment.filters["printParmList"] = PrintParmList


#
# Register the filter for adding the common prefix to names.  This may be needed in cases of
# auto-generated types/names that are part of the public interface.
#
Environment.filters["addNamePrefix"] = codeTypes.AddNamePrefix



#---------------------------------------------------------------------------------------------------
# Templates, canned code, etc
#---------------------------------------------------------------------------------------------------

DefaultPackerUnpacker = """
//--------------------------------------------------------------------------------------------------
// Generic Pack/Unpack Functions
//--------------------------------------------------------------------------------------------------

// todo: These functions could be moved to a separate library, to reduce overall code size and RAM
//       usage because they are common to each client and server.  However, they would then likely
//       need to be more generic, and provide better parameter checking and return results.  With
//       the way they are now, they can be customized to the specific needs of the generated code,
//       so for now, they will be kept with the generated code.  This may need revisiting later.

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* PackData(void* msgBufPtr, const void* dataPtr, size_t dataSize)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    memcpy( msgBufPtr, dataPtr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackData(void* msgBufPtr, void* dataPtr, size_t dataSize)
{
    memcpy( dataPtr, msgBufPtr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* PackString(void* msgBufPtr, const char* dataStr)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    // Add one for the null character
    size_t dataSize = strlen(dataStr) + 1;

    memcpy( msgBufPtr, dataStr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackString(void* msgBufPtr, const char** dataStrPtr)
{
    // Add one for the null character
    size_t dataSize = strlen(msgBufPtr) + 1;

    // Strings do not have to be word aligned, so just return a pointer
    // into the message buffer
    *dataStrPtr = msgBufPtr;
    return ( msgBufPtr + dataSize );
}

// todo: This function may eventually replace all usage of UnpackString() above.
//       Maybe there should also be a PackDataString() function as well?
// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackDataString(void* msgBufPtr, void* dataPtr, size_t dataSize)
{
    // Number of bytes copied from msg buffer, not including null terminator
    size_t numBytes;

    // todo: For now, assume the string will always fit in the buffer. This may not always be true.
    le_utf8_Copy( dataPtr, msgBufPtr, dataSize, &numBytes );
    return ( msgBufPtr + (numBytes + 1) );
}
"""


#---------------------------------------------------------------------------------------------------


HeaderCommentTemplate = """
/**
 * {{comment}}
 */
"""

#
# Puts the comment into a doxygen-style function comment header and returns the string
#
def FormatHeaderComment(comment):
    comment = '\n * '.join( comment.splitlines() )
    headerComment = FormatCode(HeaderCommentTemplate, comment=comment)

    # Strip any leading/trailing white space
    return headerComment.strip()


#---------------------------------------------------------------------------------------------------


HandlerTypeTemplate = """
//--------------------------------------------------------------------------------------------------
{{headerComment}}
//--------------------------------------------------------------------------------------------------
typedef void (*{{handler.name}})
(
    {{ handler.parmList | printParmList("clientParmList") | indent }}
);
"""


def AddParamComments(header, parmList):
    # If there is no header comment, then create a stub
    if not header:
        header = FormatHeaderComment("This handler ...")

    headerLines = header.splitlines()
    commentLines = []
    for p in parmList:
        commentLines.append( ' * @param %s' % p.name )

        # The comment lines for each parameter start with '///<' which needs to be stripped off,
        # in addition to any leading whitespace after that.
        commentLines.extend( ' *        %s' % c[4:].lstrip() for c in p.comment.splitlines() )

    newHeader = '\n'.join( headerLines[:-1] + [' *'] + commentLines + headerLines[-1:] )
    #print newHeader
    return newHeader


def WriteHandlerTypeDef(fp, handler):
    headerComment = AddParamComments(handler.comment, handler.parmList)
    handlerStr = FormatCode(HandlerTypeTemplate, handler=handler, headerComment=headerComment)
    print >>fp, handlerStr


#---------------------------------------------------------------------------------------------------


FuncPrototypeTemplate = """
//--------------------------------------------------------------------------------------------------
{{func.comment}}
//--------------------------------------------------------------------------------------------------
{{func.type}} {{func.name}}
(
    {{ func.parmList | printParmListWithComments | indent }}
)
"""


#
# Generates a properly formatted string for the parameter, along with any associated comments.
#
def FormatParameter(parm, lastParm):
    # Format the parameter
    s = parm.clientParmList.format(parm=parm)
    if not lastParm:
        s += ','

    # If the parm is not void, then the comment will contain at least the direction.
    if parm.type != "void":
        commentLines = [ "///< [%s]" % parm.direction ]
        if parm.comment:
            commentLines += parm.comment.splitlines()

        # Indent the comments, relative to the parameter type/name
        commentStr = '\n'.join( ' '*4+c for c in commentLines )

        # Add the comments on the following line, and also add a trailing blank line
        s = '\n'.join( ( s, commentStr, '' ) )
        #print s

    return s


#
# Define and register the filter for processing parameters with comments
#
def PrintParmListWithComments(parmList):
    resultList = [ FormatParameter(p, False) for p in parmList[:-1] ]
    resultList += [ FormatParameter(parmList[-1], True) ]
    return '\n'.join( resultList )

Environment.filters["printParmListWithComments"] = PrintParmListWithComments


#
# Create a string for the function prototype/declaration.  This is a separate function, since
# this string is used in more than one place, i.e. in the header file and also the client file.
#
def GetFuncPrototypeStr(func):
    funcStr = FormatCode(FuncPrototypeTemplate, func=func)

    # Remove any leading or trailing whitespace on the return string, such as newlines, so that
    # it doesn't add extra, unintended, spaces in the generated code output.
    return funcStr.strip()


#---------------------------------------------------------------------------------------------------


RespondFuncPrototypeTemplate = """
//--------------------------------------------------------------------------------------------------
/**
 * Server-side respond function for {{func.name}}
 */
//--------------------------------------------------------------------------------------------------
void {{func.name}}Respond
(
    {{ func | printRespondParmList | indent }}
)
"""


#
# Define and register the filter for processing the respond parameter list
#
def PrintRespondParmList(func):
    # This parameter always exists
    resultList = [ "%s %s" % (codeTypes.AddNamePrefix("ServerCmdRef_t"), "_cmdRef") ]

    # Add return parameter, if there is one
    if func.type != "void":
        resultList.append( "%s _result" % func.type )

    # Add OUT parameters, if there are any
    resultList += [ p.asyncServerParmList.format(parm=p) for p in func.parmListOut ]

    # Return everything as a string
    return ',\n'.join( resultList )

Environment.filters["printRespondParmList"] = PrintRespondParmList


#
# Create a string for the respond function prototype/declaration that is used by async-style
# server functions.  This string is generated in a separate function, since it is used in more
# than one place, i.e. in the header file and also the server file.
#
def GetRespondFuncPrototypeStr(func):
    funcStr = FormatCode(RespondFuncPrototypeTemplate, func=func)

    # Remove any leading or trailing whitespace on the return string, such as newlines, so that
    # it doesn't add extra, unintended, spaces in the generated code output.
    return funcStr.strip()


#---------------------------------------------------------------------------------------------------


ServerAsyncFuncPrototypeTemplate = """
//--------------------------------------------------------------------------------------------------
/**
 * Prototype for server-side async interface function
 */
//--------------------------------------------------------------------------------------------------
void {{func.name}}
(
    {{ "ServerCmdRef_t" | addNamePrefix }} _cmdRef
    {{- func.parmListInCall | printParmList("asyncServerParmList", leadSep=True) | indent }}
)
"""


#
# Create a string for the function prototype/declaration that is used by async-style
# server functions.  This string is generated in a separate function, as it may eventually
# be used in more than one place.
#
def GetServerAsyncFuncPrototypeStr(func):
    funcStr = FormatCode(ServerAsyncFuncPrototypeTemplate, func=func)

    # Remove any leading or trailing whitespace on the return string, such as newlines, so that
    # it doesn't add extra, unintended, spaces in the generated code output.
    return funcStr.strip()



#---------------------------------------------------------------------------------------------------
# Type defintion related functions and code
#---------------------------------------------------------------------------------------------------


TypeDefinitionTemplate = """
//--------------------------------------------------------------------------------------------------
{{typeDef.comment}}
//--------------------------------------------------------------------------------------------------
{{typeDef.definition}}
"""


def WriteTypeDefinition(fp, typeDef):
    typeDefStr = FormatCode(TypeDefinitionTemplate, typeDef=typeDef)
    print >>fp, typeDefStr


#---------------------------------------------------------------------------------------------------


EnumDefinitionTemplate = """
//--------------------------------------------------------------------------------------------------
{{enumDef.comment}}
//--------------------------------------------------------------------------------------------------
typedef enum
{
    {{ enumDef.memberList | printEnumListWithComments | indent }}
}
{{enumDef.typeName}};
"""


#
# Generates a properly formatted string for the enum value, along with any associated comments.
#
def FormatEnumMember(member, lastMember):
    # Format the enum
    s = member.name[:]

    if hasattr(member, 'value'):
        #print member.value
        s += ' = %s' % member.value

    if not lastMember:
        s += ','

    if member.comment:
        commentLines = member.comment.splitlines()

        # Indent the comments, relative to the parameter type/name
        commentStr = '\n'.join( ' '*4+c for c in commentLines )

        # Add the comments on the following line, and also add a trailing blank line
        s = '\n'.join( ( s, commentStr, '' ) )
        #print s

    return s


#
# Define and register the filter for processing enum members with comments
#
def PrintEnumListWithComments(memberList):
    resultList = [ FormatEnumMember(v, False) for v in memberList[:-1] ]
    resultList += [ FormatEnumMember(memberList[-1], True) ]
    return '\n'.join( resultList )

Environment.filters["printEnumListWithComments"] = PrintEnumListWithComments


def WriteEnumDefinition(fp, enumDef):
    enumDefStr = FormatCode(EnumDefinitionTemplate, enumDef=enumDef)
    print >>fp, enumDefStr


#---------------------------------------------------------------------------------------------------
# Client templates/code
#---------------------------------------------------------------------------------------------------

FuncImplTemplate = dict(

    function = """
{{prototype}}
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    {{func.resultStorage}}

    // Range check values, if appropriate
    $ for p in func.parmListIn
    $ if p.maxValue:
    {{ p.maxValueCheck.format( parm=p ) }}
    $ endif
    $ endfor
    {{""}}

    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_{{func.name}};
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    {{ func.parmListIn | printParmList("clientPack", sep="\n") | indent }}

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response");
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    {% if func.type != "void" -%}
    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );

    $ if func.addHandlerName:
    // Put the handler reference result into the client data object, and
    // then return a safe reference to the client data object as the reference.
    _clientDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK
    $ endif
    {% endif %}

    // Unpack any "out" parameters
    {{ func.parmListOut | printParmList("clientUnpack", sep="\n") | indent }}

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

    $ if func.type != "void"
    {{""}}
    return _result;
    $ endif
}
"""
)


def WriteFuncCode(func, template):
    funcStr = FormatCode(template['function'], func=func, prototype=GetFuncPrototypeStr(func))
    print >>ClientFileText, funcStr


#---------------------------------------------------------------------------------------------------

ClientHandlerTemplate = dict(

    handler = """
// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_{{func.name}}
(
    void* _reportPtr,
    void* _notUsed
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // Pull out additional data from the context pointer
    _ClientData_t* _clientDataPtr = _clientContextPtr;
    {{handler.name}} _handlerRef_{{func.name}} = ({{handler.name}})_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    {{ handler.parmList | printParmList("handlerUnpack", sep="\n\n") | indent }}

    // Call the registered handler
    if ( _handlerRef_{{func.name}} != NULL )
    {
        _handlerRef_{{func.name}}( {{ handler.parmList | printParmList("unpackCallName", sep=", ") }} );
    }
    else
    {
        LE_ERROR("ERROR in client _Handle_{{func.name}}: no registered handler");
    }

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}
"""

)

def WriteClientHandler(func, handler, template):
    funcStr = FormatCode(template["handler"], func=func, handler=handler)
    print >>ClientFileText, funcStr


#---------------------------------------------------------------------------------------------------

AsyncHandlerTemplate = dict(

    msgHandler = """
static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
)
{
    // Get the message payload
    _Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    uint8_t* _msgBufPtr = msgPtr->buffer;

    // Have to partially unpack the received message in order to know which thread
    // the queued function should actually go to.
    void* clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &clientContextPtr, sizeof(void*) );

    // Pull out the callers thread
    _ClientData_t* clientDataPtr = clientContextPtr;
    le_thread_Ref_t callersThreadRef = clientDataPtr->callersThreadRef;

    // Trigger the appropriate event
    switch (msgPtr->id)
    {
        $ for func in funcList
        case _MSGID_{{func.name}} :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_{{func.name}}, msgRef, NULL);
            break;
            {{""}}
        $ endfor

        default: LE_ERROR("Unknowm msg id = %i for client thread = %p", msgPtr->id, callersThreadRef);
    }
}
""",

)


def WriteAsyncHandler(flist, template):
    print >>ClientFileText, FormatCode(template["msgHandler"], funcList=flist)


#---------------------------------------------------------------------------------------------------
# Server templates/code
#---------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------
# This function is the handler that is registered with the server-side AddHandler function.
# It will take the parameters and package them up for sending back to the client.
#---------------------------------------------------------------------------------------------------

FuncAsyncTemplate = dict(

    function = """
static void AsyncResponse_{{func.name}}
(
    {{ handler.parmList | printParmList("clientParmList") | indent }}
)
{
    le_msg_MessageRef_t _msgRef;
    _Message_t* _msgPtr;
    _ServerData_t* serverDataPtr = (_ServerData_t*)contextPtr;

    // Will not be used if no data is sent back to client
    __attribute__((unused)) uint8_t* _msgBufPtr;

    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(serverDataPtr->clientSessionRef);
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_{{func.name}};
    _msgBufPtr = _msgPtr->buffer;

    // Always pack the client context pointer first
    _msgBufPtr = PackData( _msgBufPtr, &(serverDataPtr->contextPtr), sizeof(void*) );

    // Pack the input parameters
    {{ handler.parmList | printParmList("clientPack", sep="\n") | indent }}

    // Send the async response to the client
    LE_DEBUG("Sending message to client session %p", serverDataPtr->clientSessionRef);
    le_msg_Send(_msgRef);
}
"""

)

def WriteAsyncFuncCode(func, handler, template):
    handlerStr = FormatCode(template['function'], func=func, handler=handler)
    print >>ServerFileText, handlerStr



#---------------------------------------------------------------------------------------------------
# This function takes the message from the client, unpacks it, and calls the real server-side
# function, and then packs any return value or out parameters.
#---------------------------------------------------------------------------------------------------

FuncHandlerTemplate = dict(

    handler = """
static void Handle_{{func.name}}
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    {{ func.parmListIn | printParmList("handlerUnpack", sep="\n\n") | indent }}

    {% if func.addHandlerName -%}
    // Create a new server data object and fill it in
    _ServerData_t* serverDataPtr = le_mem_ForceAlloc(_ServerDataPool);
    serverDataPtr->clientSessionRef = le_msg_GetSession(_msgRef);
    serverDataPtr->contextPtr = contextPtr;
    contextPtr = serverDataPtr;
    {% endif %}

    // Define storage for output parameters
    {{ func.parmListOut | printParmList("handlerParmList", sep="\n") | indent }}

    // Call the function
    {% if func.type == "void" -%}

    {{func.name}} ( {{ func.parmList | printParmList("unpackCallName", sep=", ") }} );

    {% else -%}

    {{func.type}} _result;
    _result = {{func.name}} ( {{ func.parmList | printParmList("unpackCallName", sep=", ") }} );

    {% if func.addHandlerName -%}
    // Put the handler reference result and a pointer to the associated remove function
    // into the server data object.  This function pointer is needed in case the client
    // is closed and the handlers need to be removed.
    serverDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    serverDataPtr->removeHandlerFunc = (RemoveHandlerFunc_t){{func.name | replace("Add", "Remove", 1)}};

    // Return a safe reference to the server data object as the reference.
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, serverDataPtr);
    _UNLOCK
    {% endif %}

    {% endif %}

    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;

    {% if func.type != "void" -%}
    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );
    {% endif %}

    // Pack any "out" parameters
    {{ func.parmListOut | printParmList("handlerPack", sep="\n") | indent }}

    // Return the response
    LE_DEBUG("Sending response to client session %p", le_msg_GetSession(_msgRef));
    le_msg_Respond(_msgRef);
}
"""

)

AsyncFuncHandlerTemplate = dict(

    handler = """
{{respondProto}}
{
    LE_ASSERT(_cmdRef != NULL);

    // Get the message related data
    le_msg_MessageRef_t _msgRef = (le_msg_MessageRef_t)_cmdRef;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    __attribute__((unused)) uint8_t* _msgBufPtr = _msgPtr->buffer;

    // Ensure the passed in msgRef is for the correct message
    LE_ASSERT(_msgPtr->id == _MSGID_{{func.name}});

    // Ensure that this Respond function has not already been called
    LE_FATAL_IF( !le_msg_NeedsResponse(_msgRef), "Response has already been sent");

    {% if func.type != "void" -%}
    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );
    {% endif %}

    // Pack any "out" parameters
    {{ func.parmListOut | printParmList("asyncServerPack", sep="\n") | indent }}

    // Return the response
    LE_DEBUG("Sending response to client session %p", le_msg_GetSession(_msgRef));
    le_msg_Respond(_msgRef);
}

static void Handle_{{func.name}}
(
    le_msg_MessageRef_t _msgRef
)
{
    // Get the message buffer pointer
    __attribute__((unused)) uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Unpack the input parameters from the message
    {{ func.parmListIn | printParmList("handlerUnpack", sep="\n\n") | indent }}

    // Call the function
    {{func.name}} ( ({{ "ServerCmdRef_t" | addNamePrefix }})_msgRef
                    {{- func.parmListInCall | printParmList("asyncServerCallName", sep=", ", leadSep=True) }} );
}
"""
)



def WriteHandlerCode(func, template):
    # The prototype parameter is only needed for the AsyncFuncHandlerTemplate, but it does no
    # harm to always include it.  It will be ignored for the other template(s).
    funcStr = FormatCode(template["handler"],
                         func=func,
                         respondProto=GetRespondFuncPrototypeStr(func))
    print >>ServerFileText, funcStr


#---------------------------------------------------------------------------------------------------
# This function take the message from the client, and then based on the message ID, calls
# the appropriate handler.
#---------------------------------------------------------------------------------------------------

MsgHandlerTemplate = dict(

    msgHandler = """
static void ServerMsgRecvHandler
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
)
{
    // Get the message payload so that we can get the message "id"
    _Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    // Get the client session ref for the current message.  This ref is used by the server to
    // get info about the client process, such as user id.  If there are multiple clients, then
    // the session ref may be different for each message, hence it has to be queried each time.
    _ClientSessionRef = le_msg_GetSession(msgRef);

    // Dispatch to appropriate message handler and get response
    switch (msgPtr->id)
    {
        $ for func in funcList
        case _MSGID_{{func.name}} : Handle_{{func.name}}(msgRef); break;
        $ endfor
        {{""}}
        default: LE_ERROR("Unknowm msg id = %i", msgPtr->id);
    }

    // Clear the client session ref associated with the current message, since the message
    // has now been processed.
    _ClientSessionRef = 0;
}
"""
)


def WriteMsgHandler(flist, template):
    print >>ServerFileText, FormatCode(template["msgHandler"], funcList=flist)


#---------------------------------------------------------------------------------------------------
# Output file templates/code
#---------------------------------------------------------------------------------------------------


WarningNotice = """\
/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */
"""

def WriteWarning(fp):
    print >>fp, WarningNotice


#---------------------------------------------------------------------------------------------------


IncludeGuardBeginTemplate = """
#ifndef {{fileName}}_H_INCLUDE_GUARD
#define {{fileName}}_H_INCLUDE_GUARD
"""

IncludeGuardEndTemplate = """
#endif // {{fileName}}_H_INCLUDE_GUARD
"""


def FormatFileName(fileName):
    return os.path.splitext( os.path.basename(fileName) )[0].upper()

def WriteIncludeGuardBegin(fp, fileName):
    print >>fp, FormatCode(IncludeGuardBeginTemplate, fileName=FormatFileName(fileName))

def WriteIncludeGuardEnd(fp, fileName):
    print >>fp, FormatCode(IncludeGuardEndTemplate, fileName=FormatFileName(fileName))


#---------------------------------------------------------------------------------------------------

def WriteCommonInterface(fp,
                         startTemplate,
                         pf,
                         ph,
                         pt,
                         importList,
                         fileName,
                         genericFunctions,
                         headerComments,
                         genAsync):

    WriteWarning(fp)

    # Write the user-supplied doxygen-style header comments
    for comments in headerComments:
        print >>fp, comments

    WriteIncludeGuardBegin(fp, fileName)
    print >>fp, FormatCode(startTemplate)

    # Write the imported include files.
    if importList:
        print >>fp, '// Interface specific includes'
        for i in importList:
            print >>fp, '#include "%s"' % i
        print >>fp, '\n'

    # Write out the prototypes for the generic functions
    for s in genericFunctions.values():
        print >>fp, "%s;\n" % GetFuncPrototypeStr(s)

    # Write out the type definitions
    for t in pt.values():
        # Types that have a simple definition will have the appropriate attribute; otherwise call an
        # explicit function to write out the definition.
        if hasattr(t, "definition"):
            WriteTypeDefinition(fp, t)
        elif isinstance( t, codeTypes.EnumData ):
            WriteEnumDefinition(fp, t)
        else:
            # todo: Should I print an error or something here?
            pass

    # Write out the handler type definitions
    for h in ph.values():
        WriteHandlerTypeDef(fp, h)

    # Write out the function prototypes, and if required, the respond function prototypes
    for f in pf.values():
        #print f

        # The Add and Remove handler functions are never asynchronous.
        if genAsync and not f.addHandlerName and not f.isRemoveHandler :
            print >>fp, "%s;\n" % GetRespondFuncPrototypeStr(f)
            print >>fp, "%s;\n" % GetServerAsyncFuncPrototypeStr(f)
        else:
            print >>fp, "%s;\n" % GetFuncPrototypeStr(f)

    WriteIncludeGuardEnd(fp, fileName)


#---------------------------------------------------------------------------------------------------


InterfaceHeaderStartTemplate = """
#include "legato.h"
"""

def WriteInterfaceHeaderFile(pf,
                             ph,
                             pt,
                             importList,
                             genericFunctions,
                             fileName,
                             headerComments):

    WriteCommonInterface(InterfaceHeaderFileText,
                         InterfaceHeaderStartTemplate,
                         pf,
                         ph,
                         pt,
                         importList,
                         fileName,
                         genericFunctions,
                         headerComments,
                         False)


#---------------------------------------------------------------------------------------------------


LocalHeaderStartTemplate = """
#include "legato.h"

#define PROTOCOL_ID_STR "{{idString}}"

#define SERVICE_INSTANCE_NAME "{{serviceName}}"


// todo: This will need to depend on the particular protocol, but the exact size is not easy to
//       calculate right now, so in the meantime, pick a reasonably large size.  Once interface
//       type support has been added, this will be replaced by a more appropriate size.
#define _MAX_MSG_SIZE 1000

// Define the message type for communicating between client and server
typedef struct
{
    uint32_t id;
    uint8_t buffer[_MAX_MSG_SIZE];
}
_Message_t;
"""

def WriteLocalHeaderFile(pf, ph, hashValue, fileName, serviceName):
    WriteWarning(LocalHeaderFileText)
    WriteIncludeGuardBegin(LocalHeaderFileText, fileName)

    print >>LocalHeaderFileText, FormatCode(LocalHeaderStartTemplate,
                                            idString=hashValue,
                                            serviceName=serviceName)

    # Write out the message IDs for the functions
    for i, name in enumerate(pf):
        print >>LocalHeaderFileText, "#define _MSGID_%s %i" % (name, i)
    print >>LocalHeaderFileText

    WriteIncludeGuardEnd(LocalHeaderFileText, fileName)


#---------------------------------------------------------------------------------------------------

ClientGenericCode = """
//--------------------------------------------------------------------------------------------------
// Generic Client Types, Variables and Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Client Data Objects
 *
 * This object is used for each registered handler.  This is needed since we are not using
 * events, but are instead queueing functions directly with the event loop.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_HandlerFunc_t handlerPtr;          ///< Registered handler function
    void*                  contextPtr;          ///< ContextPtr registered with handler
    le_event_HandlerRef_t  handlerRef;          ///< HandlerRef for the registered handler
    le_thread_Ref_t        callersThreadRef;    ///< Caller's thread.
}
_ClientData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for client data objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ClientDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Client Thread Objects
 *
 * This object is used to contain thread specific data for each IPC client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;     ///< Client Session Reference
    int                 clientCount;    ///< Number of clients sharing this thread
}
_ClientThreadData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for client thread objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ClientThreadDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Key under which the pointer to the Thread Object (_ClientThreadData_t) will be kept in
 * thread-local storage.  This allows a thread to quickly get a pointer to its own Thread Object.
 */
//--------------------------------------------------------------------------------------------------
static pthread_key_t _ThreadDataKey;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for use with Add/Remove handler references
 *
 * @warning Use _Mutex, defined below, to protect accesses to this data.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t _HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex and associated macros for use with the above HandlerRefMap.
 *
 * Unused attribute is needed because this variable may not always get used.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static pthread_mutex_t _Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define _LOCK    LE_ASSERT(pthread_mutex_lock(&_Mutex) == 0);

/// Unlocks the mutex.
#define _UNLOCK  LE_ASSERT(pthread_mutex_unlock(&_Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/* This global flag is shared by all client threads, and is used to indicate whether the common
 * data has been initialized.  It is only initialized once by the main thread, and is only read by
 * the other threads.  Thus, a mutex is not needed for accesses to this variable.
 */
//--------------------------------------------------------------------------------------------------
static bool CommonDataInitialized = false;


//--------------------------------------------------------------------------------------------------
/**
 * Forward declaration needed by InitClientThreadData
 */
//--------------------------------------------------------------------------------------------------
static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Init thread specific data
 */
//--------------------------------------------------------------------------------------------------
static void InitClientThreadData
(
    void
)
{
    // Open a session.
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;

    protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID_STR, sizeof(_Message_t));
    sessionRef = le_msg_CreateSession(protocolRef, SERVICE_INSTANCE_NAME);
    le_msg_SetSessionRecvHandler(sessionRef, ClientIndicationRecvHandler, NULL);
    le_msg_OpenSessionSync(sessionRef);

    // Store the client sessionRef in thread-local storage, since each thread requires
    // its own sessionRef.
    _ClientThreadData_t* clientThreadPtr = le_mem_ForceAlloc(_ClientThreadDataPool);
    clientThreadPtr->sessionRef = sessionRef;
    if (pthread_setspecific(_ThreadDataKey, clientThreadPtr) != 0)
    {
        LE_FATAL("pthread_setspecific() failed!");
    }

    // This is the first client for the current thread
    clientThreadPtr->clientCount = 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the client thread data for the current thread.
 *
 * If the current thread does not have client data, then NULL is returned
 */
//--------------------------------------------------------------------------------------------------
static _ClientThreadData_t* GetClientThreadDataPtr
(
    void
)
{
    return pthread_getspecific(_ThreadDataKey);
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the sessionRef for the current thread.
 *
 * If the current thread does not have a session ref, then this is a fatal error.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static le_msg_SessionRef_t GetCurrentSessionRef
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then the session ref has not been created.
    LE_FATAL_IF(clientThreadPtr==NULL, "No client session for current thread");

    return clientThreadPtr->sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init data that is common across all threads.
 */
//--------------------------------------------------------------------------------------------------
static void InitCommonData(void)
{
    // Allocate the client data pool
    _ClientDataPool = le_mem_CreatePool("ClientData", sizeof(_ClientData_t));

    // Allocate the client thread pool
    _ClientThreadDataPool = le_mem_CreatePool("ClientThreadData", sizeof(_ClientThreadData_t));

    // Create the thread-local data key to be used to store a pointer to each thread object.
    LE_ASSERT(pthread_key_create(&_ThreadDataKey, NULL) == 0);

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined multiplied by
    // the number of client threads.  Since this number can't be completely determined at
    // build time, just make a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("ClientHandlers", 5);
}
"""

ClientStartFuncCode = """
{{ proto['startClientFunc'] }}
{
    // If this is the first time the function is called, init the client common data.
    if ( ! CommonDataInitialized )
    {
        InitCommonData();
        CommonDataInitialized = true;
    }

    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        InitClientThreadData();
        LE_DEBUG("======= Starting client for '%s' service ========", SERVICE_INSTANCE_NAME);
    }
    else
    {
        // Keep track of the number of clients for the current thread.  There is only one
        // connection per thread, and it is shared by all clients.
        clientThreadPtr->clientCount++;
        LE_DEBUG("======= Starting another client for '%s' service ========", SERVICE_INSTANCE_NAME);
    }
}

{{ proto['stopClientFunc'] }}
{
    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        LE_CRIT("Trying to stop non-existent client session for '%s' service",
                SERVICE_INSTANCE_NAME);
    }
    else
    {
        // This is the last client for this thread, so close the session.
        if ( clientThreadPtr->clientCount == 1 )
        {
            le_msg_CloseSession( clientThreadPtr->sessionRef );

            // Need to delete the thread specific data, since it is no longer valid.  If a new
            // client session is started, new thread specific data will be allocated.
            le_mem_Release(clientThreadPtr);
            if (pthread_setspecific(_ThreadDataKey, NULL) != 0)
            {
                LE_FATAL("pthread_setspecific() failed!");
            }

            LE_DEBUG("======= Stopping client for '%s' service ========", SERVICE_INSTANCE_NAME);
        }
        else
        {
            // There is one less client sharing this thread's connection.
            clientThreadPtr->clientCount--;

            LE_DEBUG("======= Stopping another client for '%s' service ========",
                     SERVICE_INSTANCE_NAME);
        }
    }
}
"""

ClientStartClientCode = """
//--------------------------------------------------------------------------------------------------
// Client Specific Client Code
//--------------------------------------------------------------------------------------------------
"""


def WriteClientFile(headerFiles, pf, ph, genericFunctions):
    WriteWarning(ClientFileText)

    print >>ClientFileText, '\n' + '\n'.join('#include "%s"'%h for h in headerFiles) + '\n'
    print >>ClientFileText, DefaultPackerUnpacker
    print >>ClientFileText, ClientGenericCode

    # Note that this does not need to be an ordered dictionary, unlike genericFunctions
    protoDict = { n: GetFuncPrototypeStr(f) for n,f in genericFunctions.items() }
    print >>ClientFileText, FormatCode(ClientStartFuncCode, proto=protoDict)

    print >>ClientFileText, ClientStartClientCode

    for f in pf.values():
        #print f
        if f.addHandlerName:
            # Write out the handler first; there should only be one
            # todo: How can this be enforced
            for h in ph.values():
                if h.name == f.addHandlerName:
                    WriteClientHandler(f, h, ClientHandlerTemplate)
                    break

        # Write out the functions next
        WriteFuncCode(f, FuncImplTemplate)

    addHandlerFuncs = [ f for f in pf.values() if f.addHandlerName ]
    WriteAsyncHandler(addHandlerFuncs, AsyncHandlerTemplate)



#---------------------------------------------------------------------------------------------------

ServerGenericCode = """
//--------------------------------------------------------------------------------------------------
// Generic Server Types, Variables and Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for generic function to remove a handler, given the handler ref.
 */
//--------------------------------------------------------------------------------------------------
typedef void(* RemoveHandlerFunc_t)(void *handlerRef);


//--------------------------------------------------------------------------------------------------
/**
 * Server Data Objects
 *
 * This object is used to store additional context info for each request
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t   clientSessionRef;     ///< The client to send the response to
    void*                 contextPtr;           ///< ContextPtr registered with handler
    le_event_HandlerRef_t handlerRef;           ///< HandlerRef for the registered handler
    RemoveHandlerFunc_t   removeHandlerFunc;    ///< Function to remove the registered handler
}
_ServerData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for server data objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ServerDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for use with Add/Remove handler references
 *
 * @warning Use _Mutex, defined below, to protect accesses to this data.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t _HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex and associated macros for use with the above HandlerRefMap.
 *
 * Unused attribute is needed because this variable may not always get used.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static pthread_mutex_t _Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define _LOCK    LE_ASSERT(pthread_mutex_lock(&_Mutex) == 0);

/// Unlocks the mutex.
#define _UNLOCK  LE_ASSERT(pthread_mutex_unlock(&_Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * Forward declaration needed by StartServer
 */
//--------------------------------------------------------------------------------------------------
static void ServerMsgRecvHandler
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef;


//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef;


//--------------------------------------------------------------------------------------------------
/**
 * Cleanup client data if the client is no longer connected
 */
//--------------------------------------------------------------------------------------------------
static void CleanupClientData
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("Client %p is closed !!!", sessionRef);

    // Iterate over the server data reference map and remove anything that matches
    // the client session.
    _LOCK

    le_ref_IterRef_t iterRef = le_ref_GetIterator(_HandlerRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    _ServerData_t const* serverDataPtr;

    while ( result == LE_OK )
    {
        serverDataPtr =  le_ref_GetValue(iterRef);

        if ( sessionRef != serverDataPtr->clientSessionRef )
        {
            LE_DEBUG("Found session ref %p; does not match",
                     serverDataPtr->clientSessionRef);
        }
        else
        {
            LE_DEBUG("Found session ref %p; match found, so needs cleanup",
                     serverDataPtr->clientSessionRef);

            // Remove the handler
            serverDataPtr->removeHandlerFunc( serverDataPtr->handlerRef );

            // Release the server data block
            le_mem_Release((void*)serverDataPtr);

            // Delete the associated safeRef
            le_ref_DeleteRef( _HandlerRefMap, (void*)le_ref_GetSafeRef(iterRef) );

            // Since the reference map was modified, the iterator is no longer valid and
            // so has to be re-initalized.  This means that some values may get revisited,
            // but eventually this will iterate over the whole reference map.
            // todo: Is there an easier way?
            iterRef = le_ref_GetIterator(_HandlerRefMap);
        }

        // Get the next value in the reference mpa
        result = le_ref_NextNode(iterRef);
    }

    _UNLOCK
}
"""

ServerStartFuncCode = """
{{ proto['getServiceRef'] }}
{
    return _ServerServiceRef;
}


{{ proto['getSessionRef'] }}
{
    return _ClientSessionRef;
}


{{ proto['startServerFunc'] }}
{
    LE_DEBUG("======= Starting Server %s ========", SERVICE_INSTANCE_NAME);

    le_msg_ProtocolRef_t protocolRef;

    // Create the server data pool
    _ServerDataPool = le_mem_CreatePool("ServerData", sizeof(_ServerData_t));

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined for the server.
    // Don't expect that to be more than 2-3, so use 3 as a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("ServerHandlers", 3);

    // Start the server side of the service
    protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID_STR, sizeof(_Message_t));
    _ServerServiceRef = le_msg_CreateService(protocolRef, SERVICE_INSTANCE_NAME);
    le_msg_SetServiceRecvHandler(_ServerServiceRef, ServerMsgRecvHandler, NULL);
    le_msg_AdvertiseService(_ServerServiceRef);

    // Register for client sessions being closed
    le_msg_AddServiceCloseHandler(_ServerServiceRef, CleanupClientData, NULL);
}
"""

ServerStartClientCode = """
//--------------------------------------------------------------------------------------------------
// Client Specific Server Code
//--------------------------------------------------------------------------------------------------
"""

def WriteServerFile(headerFiles, pf, ph, genericFunctions, genAsync):
    WriteWarning(ServerFileText)

    print >>ServerFileText, '\n' + '\n'.join('#include "%s"'%h for h in headerFiles) + '\n'
    print >>ServerFileText, DefaultPackerUnpacker
    print >>ServerFileText, ServerGenericCode

    # Note that this does not need to be an ordered dictionary, unlike genericFunctions
    protoDict = { n: GetFuncPrototypeStr(f) for n,f in genericFunctions.items() }
    print >>ServerFileText, FormatCode(ServerStartFuncCode, proto=protoDict)

    print >>ServerFileText, ServerStartClientCode

    for f in pf.values():
        #print f
        if f.addHandlerName:
            # Write out the handler first; there should only be one per function
            for h in ph.values():
                if h.name == f.addHandlerName:
                    WriteAsyncFuncCode(f, h, FuncAsyncTemplate)
                    break

        # Write out the functions next.
        # The Add and Remove handler functions are never asynchronous.
        if genAsync and not f.addHandlerName and not f.isRemoveHandler :
            WriteHandlerCode(f, AsyncFuncHandlerTemplate)
        else:
            WriteHandlerCode(f, FuncHandlerTemplate)

    WriteMsgHandler(pf.values(), MsgHandlerTemplate)


#---------------------------------------------------------------------------------------------------


ServerHeaderStartTemplate = """
#include "legato.h"
"""


AsyncServerHeaderStartTemplate = """
#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Command reference for async server-side function support.  The interface function receives the
 * reference, and must pass it to the corresponding respond function.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {{ "ServerCmd*" | addNamePrefix }} {{ "ServerCmdRef_t" | addNamePrefix }};
"""



def WriteServerHeaderFile(pf,
                          ph,
                          pt,
                          importList,
                          genericFunctions,
                          fileName,
                          genAsync):

    if genAsync:
        headerTemplate = AsyncServerHeaderStartTemplate
    else:
        headerTemplate = ServerHeaderStartTemplate

    WriteCommonInterface(ServerIncludeFileText,
                         headerTemplate,
                         pf,
                         ph,
                         pt,
                         importList,
                         fileName,
                         genericFunctions,
                         [],
                         genAsync)


#---------------------------------------------------------------------------------------------------
# Main codegen functions
#---------------------------------------------------------------------------------------------------

def GetOutputFileNames(prefix):
    # Define the default names
    fileNames = [ "interface.h", "local.h", "client.c", "server.c", "server.h" ]

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



def PostProcessAddHandler(f, flist, tlist):
    """
    Define the AddHandler and RemoveHandler functions and add them to the function list.

    todo:
     - Can the RemoveHandler function take additional parameters? If so, this could get complicated
       or maybe not so complicated.  There would have to be a REMOVE_HANDLER keyword, similar to
       how there is an ADD_HANDLER keyword.
       UPDATE: no, the RemoveHandler should not need additional parameters
     - Does it make any sense for the AddHandler to support more than one additional parameter. Based
       on current usage, the only time one extra parameter is used is for the MDC, where the user
       can select which profile they are registering against for state changes.  In all other cases,
       the AddHandler does not take any additional parameters, beyond the function pointer and in
       some cases the context pointer.  Reviewed the MCC and Positioning APIs.  MCC is same as MDC
       and uses a profile ref.  Positioning uses two additional parameters horizontal and vertical
       magnitude.  This could make things complicated.
    """

    #print f
    #print f.parmList


    #
    # Create the AddHandler/RemoveHandler ref type, which is returned by the AddHandler and used
    # by the RemoveHandler.
    #
    handlerRefType = codeTypes.ReferenceData(
                        f.baseName,
                        FormatHeaderComment("Reference type for %s handler ADD/REMOVE functions" % f.name))
    tlist[f.name] = handlerRefType
    #print handlerRefType
    #print handlerRefDefn

    #
    # Create the AddHandler and add it to the function list
    #

    # todo: update codeTypes.HandlerParmData and use it here
    addParm = codeTypes.SimpleData( "handlerPtr", f.type )
    #print addParm

    # always add a contextPtr parameter
    # todo: There might be a better way to handle this rather than using SimpleData(), but in this
    #       case, we actually want to pass the pointer value, rather than dereferencing it.
    contextParm = codeTypes.SimpleData( "contextPtr", "void*" )

    # Create the parameter list for the new AddHandler function.  If the current function does
    # not have any parameters, the parameter list will only contain VoidData.
    #print f.name, f.parmList, len(f.parmList)
    if (len(f.parmList) == 1) and isinstance(f.parmList[0], codeTypes.VoidData):
        addParmList = [ addParm, contextParm ]
    else:
        addParmList = list(f.parmList) + [ addParm, contextParm ]
    addHandler = codeTypes.FunctionData(
        "Add" + f.baseName,
        handlerRefType.refName,
        addParmList,
        FormatHeaderComment("%s handler ADD function" % f.name)
    )
    addHandler.addHandlerName = f.type

    # The parameter needs to know the name of the enclosing function.  Also, the function itself
    # needs to know that it is an AddHandler function, and the name/type of the handler that it
    # is adding.

    # todo: update codeTypes.HandlerParmData and use addParm.setFuncName here.
    # As a temporary solution, directly modify the templates for the parameter

    # These two definitions could be directly substituted into the templates below,
    # but for now, define them explicitly.
    addParm.funcName = addHandler.name
    addParm.funcType = addHandler.type

    addParm.clientPack = """\
// The input parameters are stored in the client data object, and it is
// a pointer to this object that is passed down.
// Create a new client data object and fill it in
_ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
_clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)handlerPtr;
_clientDataPtr->contextPtr = contextPtr;
_clientDataPtr->callersThreadRef = le_thread_GetCurrent();
contextPtr = _clientDataPtr;\
"""

    addParm.handlerUnpack = ""
    addParm.unpackCallName = "AsyncResponse_%s" % addParm.funcName
    # end of todo from above

    # Add the function to the function list
    #print addHandler
    flist[addHandler.name] = addHandler

    #
    # Create the RemoveHandler and add it to the function list
    #

    removeParm = codeTypes.SimpleData( "addHandlerRef", handlerRefType.refName )

    # todo: This needs to be implemented better
    # As a temporary solution, directly modify the templates for the parameter.
    # Note that the original clientPack is still needed, so prepend to it.
    removeParm.clientPack = """\
// The passed in handlerRef is a safe reference for the client data object.  Need to get the
// real handlerRef from the client data object and then delete both the safe reference and
// the object since they are no longer needed.
_LOCK
_ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
_UNLOCK
addHandlerRef = ({parm.parmType})clientDataPtr->handlerRef;
le_mem_Release(clientDataPtr);
""" + removeParm.clientPack

    # After we have unpacked the parameter, need to use it to look up data, so append the actions
    # to the original handlerUnpack
    removeParm.handlerUnpack += """
// The passed in handlerRef is a safe reference for the server data object.  Need to get the
// real handlerRef from the server data object and then delete both the safe reference and
// the object since they are no longer needed.
_LOCK
_ServerData_t* serverDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
_UNLOCK
addHandlerRef = ({func.type})serverDataPtr->handlerRef;
le_mem_Release(serverDataPtr);
""".format(func=addHandler)

    removeHandler = codeTypes.FunctionData(
        "Remove" + f.baseName,
        "void",
        [ removeParm ],
        FormatHeaderComment("%s handler REMOVE function" % f.name)
    )
    removeHandler.isRemoveHandler = True

    # Add the function to the function list
    flist[removeHandler.name] = removeHandler




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
        if isinstance( f, codeTypes.HandlerFunctionData ):
            parsedHandlers[f.name] = f
        elif isinstance( f, codeTypes.AddHandlerFunctionData ):
            PostProcessAddHandler( f, parsedFunctions, parsedTypes )
        elif isinstance( f, codeTypes.BaseTypeData ):
            parsedTypes[f.name] = f
        else:
            parsedFunctions[f.name] = f

    # Create the generic client functions
    startClientFunc = codeTypes.FunctionData(
        "ConnectService",
        "void",
        [ codeTypes.VoidData() ],
        FormatHeaderComment("Connect the client to the service")
    )

    stopClientFunc = codeTypes.FunctionData(
        "DisconnectService",
        "void",
        [ codeTypes.VoidData() ],
        FormatHeaderComment("Disconnect the client from the service")
    )

    genericInterfaceFunctions = collections.OrderedDict( ( ('startClientFunc', startClientFunc),
                                                           ('stopClientFunc',  stopClientFunc) ) )

    # Create the generic server functions
    startServerFunc = codeTypes.FunctionData(
        "AdvertiseService",
        "void",
        [ codeTypes.VoidData() ],
        FormatHeaderComment("Initialize the server and advertise the service.")
    )

    getServiceRef = codeTypes.FunctionData(
        "GetServiceRef",
        "le_msg_ServiceRef_t",
        [ codeTypes.VoidData() ],
        FormatHeaderComment("Get the server service reference")
    )

    getSessionRef = codeTypes.FunctionData(
        "GetClientSessionRef",
        "le_msg_SessionRef_t",
        [ codeTypes.VoidData() ],
        FormatHeaderComment("Get the client session reference for the current message")
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

        WriteInterfaceHeaderFile(parsedFunctions,
                                 parsedHandlers,
                                 parsedTypes,
                                 clientImportList,
                                 genericFunctions,
                                 os.path.splitext( os.path.basename(commandArgs.interfaceFile) )[0],
                                 # interfaceFname,
                                 headerComments)
        open(interfaceFpath, 'w').write( InterfaceHeaderFileText.getvalue() )

    if commandArgs.genLocal:
        WriteLocalHeaderFile(parsedFunctions,
                             parsedHandlers,
                             hashValue,
                             localFname,
                             commandArgs.serviceName)
        open(localFpath, 'w').write( LocalHeaderFileText.getvalue() )

    if commandArgs.genClient:
        WriteClientFile([localFname, interfaceFname],
                        parsedFunctions,
                        parsedHandlers,
                        genericInterfaceFunctions)
        open(clientFpath, 'w').write( ClientFileText.getvalue() )

    if commandArgs.genServerInterface:
        # If there are no functions or handlers defined, then don't put the generic server
        # function prototypes in the server interface header file.
        if not parsedFunctions and not parsedHandlers:
            genericFunctions = {}
        else:
            genericFunctions = genericServerFunctions

        WriteServerHeaderFile(parsedFunctions,
                              parsedHandlers,
                              parsedTypes,
                              serverImportList,
                              genericFunctions,
                              os.path.splitext( os.path.basename(commandArgs.interfaceFile) )[0],
                              # serverIncludeFname,
                              commandArgs.async)
        open(serverIncludeFpath, 'w').write( ServerIncludeFileText.getvalue() )

    if commandArgs.genServer:
        WriteServerFile([localFname, serverIncludeFname],
                        parsedFunctions,
                        parsedHandlers,
                        genericServerFunctions,
                        commandArgs.async)
        open(serverFpath, 'w').write( ServerFileText.getvalue() )


