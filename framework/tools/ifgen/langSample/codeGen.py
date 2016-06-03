#
# Sample code generator functions
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#

import os
import cStringIO
import re

import common
import codeTypes


#---------------------------------------------------------------------------------------------------
# Globals
#---------------------------------------------------------------------------------------------------

# Contains the text for the various data to be printed.
FunctionText = cStringIO.StringIO()
HandlerText = cStringIO.StringIO()
EventText = cStringIO.StringIO()
HeaderText = cStringIO.StringIO()


#---------------------------------------------------------------------------------------------------
# Template related definitions and functions
#---------------------------------------------------------------------------------------------------

# Init the template environment. This is needed in this file for adding filters to the environment.
Environment = common.InitTemplateEnvironment()



#---------------------------------------------------------------------------------------------------
# Templates and associated code for function headers
#---------------------------------------------------------------------------------------------------


FuncPrototypeTemplate = """
//--------------------------------------------------------------------------------------------------
{{func.comment}}
//--------------------------------------------------------------------------------------------------
{{func.funcType}} {{func.funcName}}
(
    {{ func.parmList | printParmListWithComments | indent }}
)
"""


#
# Generates a properly formatted string for the parameter, along with any associated comments.
#
def FormatParameter(parm, lastParm):
    # Format the parameter
    s = "{parm.pType} {parm.pName}".format(parm=parm)
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
        return ''

Environment.filters["printParmListWithComments"] = PrintParmListWithComments


#
# Create a string for the function prototype/declaration.
#
def GetFuncPrototypeStr(func):
    funcStr = common.FormatCode(FuncPrototypeTemplate, func=func)

    # Remove any leading or trailing whitespace on the return string, such as newlines, so that
    # it doesn't add extra, unintended, spaces in the generated code output.
    return funcStr.strip()



#---------------------------------------------------------------------------------------------------
# Write the info to the appropriate buffers
#---------------------------------------------------------------------------------------------------

def WriteFunctions(fList, useLongFormat=False):
    for f in fList:
        if useLongFormat:
            fStr = GetFuncPrototypeStr(f) + '\n\n'
        else:
            fStr = str(f)
        print >>FunctionText, fStr


def WriteHandlers(hList):
    for h in hList:
        print >>HandlerText, h


def WriteEvents(eList):
    for e in eList:
        print >>EventText, e


def WriteHeader(headerList):
    for h in headerList:
        print >>HeaderText, h


#---------------------------------------------------------------------------------------------------
# Public functions
#---------------------------------------------------------------------------------------------------

def WriteAllCode(commandArgs, parsedData, hashValue):

    # Extract the different types of data
    headerComments = parsedData['headerList']
    parsedCode = parsedData['codeList']
    importList = parsedData['importList']

    # Split the parsed code into three lists
    parsedFunctions = []
    parsedHandlers = []
    parsedEvents = []

    for f in parsedCode:
        if isinstance( f, codeTypes.HandlerFuncData ):
            parsedHandlers.append(f)
        elif isinstance( f, codeTypes.EventFuncData ):
            parsedEvents.append(f)
        else:
            parsedFunctions.append(f)


    #
    # If requested, write the contents to the corresponding buffers, and then print out the buffers
    #

    if commandArgs.printFunctions:
        WriteFunctions(parsedFunctions, commandArgs.longFormat)
        print FunctionText.getvalue()

    if commandArgs.printHandlers:
        WriteHandlers(parsedHandlers)
        print HandlerText.getvalue()

    if commandArgs.printEvents:
        WriteEvents(parsedEvents)
        print EventText.getvalue()

    if commandArgs.printHeader:
        WriteHeader(headerComments)
        print HeaderText.getvalue()


