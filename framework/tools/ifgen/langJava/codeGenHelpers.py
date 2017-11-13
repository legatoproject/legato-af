#
# Functions for easing writing templates.
#
# These are:
#  - filters format single elements or small lists which would be hard to write into the
# template directly.
#  - tests to switch between templates depending on the type of item being formated
#  - global helper functions
#
# Copyright (C) Sierra Wireless Inc.
#

import interfaceIR
from jinja2 import contextfilter

#---------------------------------------------------------------------------------------------------
# Global objects used by the C API
#---------------------------------------------------------------------------------------------------
_CONTEXT_TYPE = interfaceIR.BasicType("context", 4)

#---------------------------------------------------------------------------------------------------
# Filters
#---------------------------------------------------------------------------------------------------

def FormatHeaderComment(comment):
    """
    Format a header comment into a C-style Doxygen comment
    """
    commentLines = comment.split(u'\n')
    prefix = u' *'
    result = u'/**' + commentLines[0] + u'\n'
    for commentLine in commentLines[1:-1]:
        result += prefix + commentLine + u'\n'
        if commentLine.strip() == u'@verbatim':
            prefix = u''
        elif commentLine.strip() == u'@endverbatim':
            prefix = u' *'
    # If comment ends on a trailing newline, replace newline '/' to close the comment, otherwise
    # need a full comment close.  But trailing newline is the common case.
    if commentLines[-1].strip() == u'':
        result += u' */'
    else:
        result += prefix + commentLines[-1] + u'*/'
    return result

_BasicTypeMapping = {
    interfaceIR.UINT8_TYPE:  "BigInteger",
    interfaceIR.UINT16_TYPE: "BigInteger",
    interfaceIR.UINT32_TYPE: "BigInteger",
    interfaceIR.UINT64_TYPE: "BigInteger",
    interfaceIR.INT8_TYPE:   "BigInteger",
    interfaceIR.INT16_TYPE:  "BigInteger",
    interfaceIR.INT32_TYPE:  "BigInteger",
    interfaceIR.INT64_TYPE:  "BigInteger",
    interfaceIR.BOOL_TYPE:   "boolean",
    interfaceIR.CHAR_TYPE:   "byte",
    interfaceIR.DOUBLE_TYPE: "double",
    interfaceIR.SIZE_TYPE:   "BigInteger",
    interfaceIR.STRING_TYPE: "String",
    interfaceIR.FILE_TYPE:   "FileDescriptor",
    interfaceIR.RESULT_TYPE: "Result",
    interfaceIR.ONOFF_TYPE:  "OnOff",
}

@contextfilter
def FormatType(context, apiType, qualifiedTypes=None):
    """Produce a Java type from an API type"""
    if qualifiedTypes == None:
        qualifiedTypes = context.resolve('qualifiedTypes')

    if apiType == None:
        return "void"
    elif isinstance(apiType, interfaceIR.BasicType):
        return _BasicTypeMapping[apiType]
    elif isinstance(apiType, interfaceIR.HandlerReferenceType):
        return "long"
    elif isinstance(apiType, interfaceIR.ReferenceType):
        if not qualifiedTypes and apiType.iface.name == context['apiName']:
            return "%sRef" % (apiType.name, )
        else:
            return "%s.%sRef" % (apiType.iface.name, apiType.name)
    else:
        if not qualifiedTypes and apiType.iface.name == context['apiName']:
            return "%s" % (apiType.name, )
        else:
            return "%s.%s" % (apiType.iface.name, apiType.name)

_BoxTypeMapping = {
    "boolean": "Boolean",
    "double":  "Double",
    "byte":    "Byte",
    "char":    "Character"
}

@contextfilter
def FormatBoxedType(context, apiType, qualifiedTypes=None):
    """Produce a boxed Java type from an API type"""
    javaType = FormatType(context, apiType, qualifiedTypes)
    if javaType in _BoxTypeMapping:
        javaType = _BoxTypeMapping[javaType]
    return javaType

_LiteralInitMapping = {
    "BigInteger": "BigInteger.ZERO",
    "boolean": "false",
    "float": "0.0f",
    "double": "0.0d",
    "byte": "'\u0000'",
    "char": "'\u0000'"
}

def GetDefaultValue(apiType):
    """Produce Java default value for an API type"""
    try:
        return _LiteralInitMapping[_BasicTypeMapping[apiType]]
    except KeyError, e:
        return "null"

@contextfilter
def FormatParameter(context, parameter, name=None, qualifiedTypes=None):
    if name == None:
        name = parameter.name
    if parameter.direction == interfaceIR.DIR_OUT:
        if isinstance(parameter, interfaceIR.ArrayParameter):
            return "Ref<%s[]> %s" % (FormatBoxedType(context, parameter.apiType, qualifiedTypes), name)
        else:
            return "Ref<%s> %s" % (FormatBoxedType(context, parameter.apiType, qualifiedTypes), name)
    elif isinstance(parameter, interfaceIR.ArrayParameter):
        return "%s[] %s" % (FormatType(context, parameter.apiType, qualifiedTypes), name)
    else:
        return "%s %s" % (FormatType(context, parameter.apiType, qualifiedTypes), name)

def IndentCode(text, width=4, indentfirst=False):
    """Indent block as code -- blank lines are not indented."""
    lines = text.split('\n')
    return "\n".join([ (" "*width if (indentfirst or lineNo != 0) and line != '' else "") + line
                       for lineNo, line in enumerate(lines) ])
