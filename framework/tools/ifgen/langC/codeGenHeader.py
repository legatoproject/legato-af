#
# C code generator functions for header files
#
# Copyright (C) Sierra Wireless Inc.
#

import os
import cStringIO
import collections
import re

import common
import codeTypes

import codeGenCommon


#---------------------------------------------------------------------------------------------------
# Globals
#---------------------------------------------------------------------------------------------------

# Contains the text for the client and handler files.
# These will be written out to actual files
ServerIncludeFileText = cStringIO.StringIO()
LocalHeaderFileText = cStringIO.StringIO()
InterfaceHeaderFileText = cStringIO.StringIO()


#---------------------------------------------------------------------------------------------------
# Type definition related functions and code
#---------------------------------------------------------------------------------------------------


TypeDefinitionTemplate = """
//--------------------------------------------------------------------------------------------------
{{typeDef.comment}}
//--------------------------------------------------------------------------------------------------
{{typeDef.definition}}
"""


def WriteTypeDefinition(fp, typeDef):
    typeDefStr = common.FormatCode(TypeDefinitionTemplate, typeDef=typeDef)
    print >>fp, typeDefStr


#---------------------------------------------------------------------------------------------------


EnumDefinitionTemplate = """
//--------------------------------------------------------------------------------------------------
{{enumDef.comment}}
//--------------------------------------------------------------------------------------------------
typedef enum
{
    {{ enumDef.memberList | printEnumListWithComments(enumDef.valueFormatFunc) | indent }}
}
{{enumDef.name}};
"""


#
# Generates a properly formatted string for the enum value, along with any associated comments.
#
def FormatEnumMember(member, lastMember, valueFormatFunc=None):
    # Format the enum
    s = member.name[:]

    if hasattr(member, 'value'):
        #print member.value
        value = member.value
        if valueFormatFunc:
            value = valueFormatFunc(value)
        s += ' = %s' % value

    if not lastMember:
        s += ','

    if member.commentLines:
        # Indent the comments, relative to the parameter type/name
        commentStr = '\n'.join( '    ///< '+c for c in member.commentLines )

        # Add the comments on the following line, and also add a trailing blank line
        s = '\n'.join( ( s, commentStr, '' ) )
        #print s

    return s


#
# Define and register the filter for processing enum members with comments
#
def PrintEnumListWithComments(memberList, valueFormatFunc=None):
    resultList = [ FormatEnumMember(v, False, valueFormatFunc) for v in memberList[:-1] ]
    resultList += [ FormatEnumMember(memberList[-1], True, valueFormatFunc) ]
    return '\n'.join( resultList )

codeGenCommon.Environment.filters["printEnumListWithComments"] = PrintEnumListWithComments


def WriteEnumDefinition(fp, enumDef):
    enumDefStr = common.FormatCode(EnumDefinitionTemplate, enumDef=enumDef)
    print >>fp, enumDefStr


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


def AddHandlerParamComments(header, parmList):
    # If there is no header comment, then create a stub
    if not header:
        header = codeGenCommon.FormatHeaderComment("This handler ...")

    headerLines = header.splitlines()
    commentLines = []
    for p in parmList:
        # Output the parameter and associated comments.  Pad the comment lines with spaces after
        # the '*' so that the comment lines up with the start of the parameter name above.
        commentLines.append( ' * @param %s' % p.name )
        commentLines.extend( ' *        %s' % c for c in p.commentLines )

    newHeader = '\n'.join( headerLines[:-1] + [' *'] + commentLines + headerLines[-1:] )
    #print newHeader
    return newHeader


def WriteHandlerTypeDef(fp, handler):
    headerComment = AddHandlerParamComments(handler.comment, handler.parmList)
    handlerStr = common.FormatCode(HandlerTypeTemplate, handler=handler, headerComment=headerComment)
    print >>fp, handlerStr



#---------------------------------------------------------------------------------------------------
# Write header files
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
    print >>fp, common.FormatCode(IncludeGuardBeginTemplate, fileName=FormatFileName(fileName))

def WriteIncludeGuardEnd(fp, fileName):
    print >>fp, common.FormatCode(IncludeGuardEndTemplate, fileName=FormatFileName(fileName))


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

    codeGenCommon.WriteWarning(fp)

    # Write the user-supplied doxygen-style header comments
    for comments in headerComments:
        print >>fp, comments

    WriteIncludeGuardBegin(fp, fileName)
    print >>fp, common.FormatCode(startTemplate)

    # Write the imported include files.
    if importList:
        print >>fp, '// Interface specific includes'
        for i in importList:
            print >>fp, '#include "%s"' % i
        print >>fp, '\n'

    # Write out the prototypes for the generic functions
    for s in genericFunctions.values():
        print >>fp, "%s;\n" % codeGenCommon.GetFuncPrototypeStr(s)

    # Write out the type definitions
    for t in pt.values():
        # Types that have a simple definition will have the appropriate attribute; otherwise call an
        # explicit function to write out the definition.
        if hasattr(t, "definition"):
            WriteTypeDefinition(fp, t)
        elif isinstance( t, codeTypes.EnumData ):
            WriteEnumDefinition(fp, t)
        elif isinstance( t, codeTypes.BitMaskData ):
            # TODO: Bitmask may get it's own function later
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

        # Functions that have handler parameters, or are Add and Remove handler functions are
        # never asynchronous.
        if genAsync and not f.handlerName and not f.isRemoveHandler :
            print >>fp, "%s;\n" % codeGenCommon.GetRespondFuncPrototypeStr(f)
            print >>fp, "%s;\n" % codeGenCommon.GetServerAsyncFuncPrototypeStr(f)
        else:
            print >>fp, "%s;\n" % codeGenCommon.GetFuncPrototypeStr(f)

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

    return InterfaceHeaderFileText


#---------------------------------------------------------------------------------------------------


LocalHeaderStartTemplate = """
#include "legato.h"

#define PROTOCOL_ID_STR "{{idString}}"

#ifdef MK_TOOLS_BUILD
    extern const char** {{ "ServiceInstanceNamePtr" | addNamePrefix }};
    #define SERVICE_INSTANCE_NAME (*{{ "ServiceInstanceNamePtr" | addNamePrefix }})
#else
    #define SERVICE_INSTANCE_NAME "{{serviceName}}"
#endif


// todo: This will need to depend on the particular protocol, but the exact size is not easy to
//       calculate right now, so in the meantime, pick a reasonably large size.  Once interface
//       type support has been added, this will be replaced by a more appropriate size.
#define _MAX_MSG_SIZE {{maxMsgSize}}

// Define the message type for communicating between client and server
typedef struct
{
    uint32_t id;
    uint8_t buffer[_MAX_MSG_SIZE];
}
_Message_t;
"""

def WriteLocalHeaderFile(pf, ph, hashValue, fileName, serviceName):

    # TODO: This is a temporary workaround. Some API files require a larger message size, so
    #       hand-code the required size.  The size is not increased for all API files, because
    #       this could significantly increase memory usage, although it is bumped up a little
    #       bit from 1000 to 1100.
    #
    #       In the future, this size will be automatically calculated.
    #
    if fileName.endswith( ("le_secStore_messages.h",
                           "secStoreGlobal_messages.h",
                           "secStoreAdmin_messages.h",
                           "le_fs_messages.h") ):
        maxMsgSize = 8500
    elif fileName.endswith("le_cfg_messages.h"):
        maxMsgSize = 1600
    else:
        maxMsgSize = 1100

    codeGenCommon.WriteWarning(LocalHeaderFileText)
    WriteIncludeGuardBegin(LocalHeaderFileText, fileName)

    print >>LocalHeaderFileText, common.FormatCode(LocalHeaderStartTemplate,
                                                   idString=hashValue,
                                                   serviceName=serviceName,
                                                   maxMsgSize=maxMsgSize)

    # Write out the message IDs for the functions
    for i, name in enumerate(pf):
        print >>LocalHeaderFileText, "#define _MSGID_%s %i" % (name, i)
    print >>LocalHeaderFileText

    WriteIncludeGuardEnd(LocalHeaderFileText, fileName)

    return LocalHeaderFileText


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

    return ServerIncludeFileText

