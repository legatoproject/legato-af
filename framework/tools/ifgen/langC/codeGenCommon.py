#
# C code generator common functions and definitions
#
# Copyright (C) Sierra Wireless Inc.
#

import os
import cStringIO
import collections
import re

import common
import codeTypes




#---------------------------------------------------------------------------------------------------
# Template related definitions and functions
#---------------------------------------------------------------------------------------------------

# Init the template environment. This is needed in this file for adding filters to the environment.
Environment = common.InitTemplateEnvironment()


#
# Define and register the filter for processing a parameter list with the given parameter template
#
def PrintParmList(parmList, templateName, sep=',\n', leadSep=False):
    resultList = [ getattr(p, templateName).format(parm=p) for p in parmList ]
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

    // Get the sizes
    uint32_t strSize = strlen(dataStr);
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Always pack the string size first, and then the string itself
    memcpy( msgBufPtr, &strSize, sizeOfStrSize );
    msgBufPtr += sizeOfStrSize;
    memcpy( msgBufPtr, dataStr, strSize );

    // Return pointer to next free byte; msgBufPtr was adjusted above for string size value.
    return ( msgBufPtr + strSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackString(void* msgBufPtr, char* dataStr, size_t dataSize)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    uint32_t strSize;
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Get the string size first, and then the actual string
    memcpy( &strSize, msgBufPtr, sizeOfStrSize );
    msgBufPtr += sizeOfStrSize;

    // Copy the string, and make sure it is null-terminated
    memcpy( dataStr, msgBufPtr, strSize );
    dataStr[strSize] = 0;

    // Return pointer to next free byte; msgBufPtr was adjusted above for string size value.
    return ( msgBufPtr + strSize );
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
    headerComment = common.FormatCode(HeaderCommentTemplate, comment=comment)

    # Strip any leading/trailing white space
    return headerComment.strip()


#---------------------------------------------------------------------------------------------------


FuncPrototypeTemplate = """
//--------------------------------------------------------------------------------------------------
{{func.comment}}
//--------------------------------------------------------------------------------------------------
{{ func.type if func.type else 'void' }} {{func.name}}
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

    # The comment will contain at least the direction.
    directionStr = "[%s]" % parm.direction
    if not parm.commentLines:
        commentLines = [ directionStr ]
    else:
        # First line contains the direction
        commentLines = [ "%s %s" % ( directionStr, parm.commentLines[0] ) ]

        # Remaining lines are padded so that the comments line up
        commentLines += [ "%s %s" % (" "*len(directionStr), c) for c in parm.commentLines[1:] ]

    # Indent the comments, relative to the parameter type/name
    commentStr = '\n'.join( '    ///< '+c for c in commentLines )

    # Add the comments on the following line, and also add a trailing blank line
    s = '\n'.join( ( s, commentStr, '' ) )
    #print s

    return s


#
# Define and register the filter for processing parameters with comments
#
def PrintParmListWithComments(parmList):
    if parmList:
        resultList = [ FormatParameter(p, False) for p in parmList[:-1] ]
        resultList += [ FormatParameter(parmList[-1], True) ]
        return '\n'.join( resultList )
    else:
        return 'void'

Environment.filters["printParmListWithComments"] = PrintParmListWithComments


#
# Create a string for the function prototype/declaration.  This is a separate function, since
# this string is used in more than one place, i.e. in the header file and also the client file.
#
def GetFuncPrototypeStr(func):
    funcStr = common.FormatCode(FuncPrototypeTemplate, func=func)

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
    if func.type:
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
    funcStr = common.FormatCode(RespondFuncPrototypeTemplate, func=func)

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
    funcStr = common.FormatCode(ServerAsyncFuncPrototypeTemplate, func=func)

    # Remove any leading or trailing whitespace on the return string, such as newlines, so that
    # it doesn't add extra, unintended, spaces in the generated code output.
    return funcStr.strip()



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


