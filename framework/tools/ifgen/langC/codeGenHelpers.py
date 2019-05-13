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

import os
import interfaceIR
from jinja2 import contextfilter, environmentfilter

#---------------------------------------------------------------------------------------------------
# Global objects used by the C API
#---------------------------------------------------------------------------------------------------
_CONTEXT_TYPE = interfaceIR.BasicType("context", 4)

# Taken from http://en.cppreference.com/w/cpp/keyword
_KEYWORDS = frozenset(['alignas',
                       'alignof',
                       'and',
                       'and_eq',
                       'asm',
                       'atomic_cancel',
                       'atomic_commit',
                       'atomic_noexcept',
                       'auto',
                       'bitand',
                       'bitor',
                       'bool',
                       'break',
                       'case',
                       'catch',
                       'char',
                       'char16_t',
                       'char32_t',
                       'class',
                       'compl',
                       'concept',
                       'const',
                       'constexpr',
                       'const_cast',
                       'continue',
                       'co_await',
                       'co_return',
                       'co_yield',
                       'decltype',
                       'default',
                       'delete',
                       'do',
                       'double',
                       'dynamic_cast',
                       'else',
                       'enum',
                       'explicit',
                       'export',
                       'extern',
                       'false',
                       'float',
                       'for',
                       'friend',
                       'goto',
                       'if',
                       'import',
                       'inline',
                       'int',
                       'long',
                       'module',
                       'mutable',
                       'namespace',
                       'new',
                       'noexcept',
                       'not',
                       'not_eq',
                       'nullptr',
                       'operator',
                       'or',
                       'or_eq',
                       'private',
                       'protected',
                       'public',
                       'register',
                       'reinterpret_cast',
                       'requires',
                       'return',
                       'short',
                       'signed',
                       'sizeof',
                       'static',
                       'static_assert',
                       'static_cast',
                       'struct',
                       'switch',
                       'synchronized',
                       'template',
                       'this',
                       'thread_local',
                       'throw',
                       'true',
                       'try',
                       'typedef',
                       'typeid',
                       'typename',
                       'union',
                       'unsigned',
                       'using',
                       'virtual',
                       'void',
                       'volatile',
                       'wchar_t',
                       'while',
                       'xor',
                       'xor_eq'])

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

def FormatDirection(direction):
    if direction == interfaceIR.DIR_IN:
        return "IN"
    elif direction == interfaceIR.DIR_OUT:
        return "OUT"
    else:
        return "INOUT"

def FormatType(apiType, useBaseName=False):
    """Produce a C type from an API type"""
    BasicTypeMapping = {
        interfaceIR.UINT8_TYPE:  "uint8_t",
        interfaceIR.UINT16_TYPE: "uint16_t",
        interfaceIR.UINT32_TYPE: "uint32_t",
        interfaceIR.UINT64_TYPE: "uint64_t",
        interfaceIR.INT8_TYPE:   "int8_t",
        interfaceIR.INT16_TYPE:  "int16_t",
        interfaceIR.INT32_TYPE:  "int32_t",
        interfaceIR.INT64_TYPE:  "int64_t",
        interfaceIR.BOOL_TYPE:   "bool",
        interfaceIR.CHAR_TYPE:   "char",
        interfaceIR.DOUBLE_TYPE: "double",
        interfaceIR.SIZE_TYPE:   "size_t",
        interfaceIR.STRING_TYPE: "char*",
        interfaceIR.FILE_TYPE:   "int",
        interfaceIR.RESULT_TYPE: "le_result_t",
        interfaceIR.ONOFF_TYPE:  "le_onoff_t",
        _CONTEXT_TYPE: "void*"
    }

    if apiType == None:
        return "void"
    elif isinstance(apiType, interfaceIR.BasicType):
        return BasicTypeMapping[apiType]

    if useBaseName:
        apiName = apiType.iface.baseName
    else:
        apiName = apiType.iface.name

    if isinstance(apiType, interfaceIR.ReferenceType):
        return "%s_%sRef_t" % (apiName, apiType.name)
    elif isinstance(apiType, interfaceIR.HandlerType):
        return "%s_%sFunc_t" % (apiName, apiType.name)
    else:
        return "%s_%s_t" % (apiName, apiType.name)

def FormatTypeInitializer(apiType,useBaseName=False):
    """Produce a C initializer from an API type"""
    BasicTypeMapping = {
        interfaceIR.UINT8_TYPE:  "0",
        interfaceIR.UINT16_TYPE: "0",
        interfaceIR.UINT32_TYPE: "0",
        interfaceIR.UINT64_TYPE: "0",
        interfaceIR.INT8_TYPE:   "0",
        interfaceIR.INT16_TYPE:  "0",
        interfaceIR.INT32_TYPE:  "0",
        interfaceIR.INT64_TYPE:  "0",
        interfaceIR.BOOL_TYPE:   "false",
        interfaceIR.CHAR_TYPE:   "0",
        interfaceIR.DOUBLE_TYPE: "0",
        interfaceIR.SIZE_TYPE:   "0",
        interfaceIR.STRING_TYPE: "NULL",
        interfaceIR.FILE_TYPE:   "0",
        interfaceIR.RESULT_TYPE: "LE_OK",
        interfaceIR.ONOFF_TYPE:  "LE_OFF",
        _CONTEXT_TYPE:           "NULL"
    }
    if isinstance(apiType, interfaceIR.BasicType):
        return BasicTypeMapping[apiType]
    elif isinstance(apiType, interfaceIR.EnumType) or isinstance(apiType, interfaceIR.BitmaskType):
        return "({0}) 0".format(FormatType(apiType,useBaseName))
    elif isinstance(apiType, interfaceIR.StructType):
        return "{ }"
    else:
        return "NULL"

def DecorateName(name):
    if name in _KEYWORDS:
        return '_' + name
    else:
        return name

def FormatParameterName(parameter, forceInput=False):
    if (isinstance(parameter, interfaceIR.ArrayParameter)
        or isinstance(parameter.apiType, interfaceIR.HandlerType)
        or isinstance(parameter.apiType, interfaceIR.StructType)
        or (not forceInput
            and (parameter.direction & interfaceIR.DIR_OUT) == interfaceIR.DIR_OUT
            and not isinstance(parameter, interfaceIR.StringParameter))):
        return parameter.name + "Ptr"
    else:
        return DecorateName(parameter.name)

def FormatParameterPtr(parameter):
    """Get a pointer to the storage indicated by the parameter from a parameter"""
    if (parameter.direction & interfaceIR.DIR_OUT) == interfaceIR.DIR_OUT:
        # Output parameters, and arrays and strings are themselves pointers
        return FormatParameterName(parameter)
    else:
        # Everything else needs to have its address taken
        return "&" + FormatParameterName(parameter)

@environmentfilter
def FormatParameter(env, parameter, forceInput=False, useBaseName=False):
    if isinstance(parameter, interfaceIR.StringParameter):
        return ((u"const " if forceInput or parameter.direction == interfaceIR.DIR_IN else u"") +
                FormatType(parameter.apiType,useBaseName) +
                (u" LE_NONNULL " if not env.globals.get('dontGenerateAttrs') and
                 parameter.direction == interfaceIR.DIR_IN else u" ") +
                parameter.name)
    elif isinstance(parameter, interfaceIR.ArrayParameter):
        return ((u"const " if forceInput or parameter.direction == interfaceIR.DIR_IN else u"") +
                FormatType(parameter.apiType,useBaseName) + "* " + parameter.name + "Ptr")
    elif isinstance(parameter.apiType, interfaceIR.HandlerType):
        return FormatType(parameter.apiType,useBaseName) + " " + parameter.name + "Ptr"
    elif isinstance(parameter.apiType, interfaceIR.StructType):
        if forceInput or parameter.direction == interfaceIR.DIR_IN:
            return u"const " + FormatType(parameter.apiType,useBaseName) + " * " + \
                (u"LE_NONNULL " if not env.globals.get('dontGenerateAttrs') else "") + \
                parameter.name + "Ptr"
        else:
            return FormatType(parameter.apiType,useBaseName) + " * " + parameter.name + "Ptr"
    elif forceInput or parameter.direction == interfaceIR.DIR_IN:
        return FormatType(parameter.apiType,useBaseName) + " " + DecorateName(parameter.name)
    else:
        return FormatType(parameter.apiType,useBaseName) + "* " + parameter.name + "Ptr"

def GetParameterCount(param):
    """
    Get actual number of elements in a array or string passed as a parameter.
    """
    if param.direction == interfaceIR.DIR_IN:
        if isinstance(param, interfaceIR.StringParameter):
            return "strnlen(%s, %d)" % (param.name, param.maxCount)
        elif isinstance(param, interfaceIR.ArrayParameter):
            return "%sSize" % (param.name,)
    else:
        if isinstance(param, interfaceIR.StringParameter):
            return "(%sSize-1)" % (param.name,)
        elif isinstance(param, interfaceIR.ArrayParameter):
            return "(*%sSizePtr)" % (param.name,)

def GetParameterCountPtr(param):
    """
    Get address of the count of elements in a array or string passed as a parameter.
    """
    if not isinstance(param, interfaceIR.ArrayParameter):
        return None

    if param.direction == interfaceIR.DIR_IN:
        return "&%sSize" % (param.name,)
    else:
        return "%sSizePtr" % (param.name,)

_PackFunctionMapping = {
    interfaceIR.UINT8_TYPE:  "le_pack_%sUint8",
    interfaceIR.UINT16_TYPE: "le_pack_%sUint16",
    interfaceIR.UINT32_TYPE: "le_pack_%sUint32",
    interfaceIR.UINT64_TYPE: "le_pack_%sUint64",
    interfaceIR.INT8_TYPE:   "le_pack_%sInt8",
    interfaceIR.INT16_TYPE:  "le_pack_%sInt16",
    interfaceIR.INT32_TYPE:  "le_pack_%sInt32",
    interfaceIR.INT64_TYPE:  "le_pack_%sInt64",
    interfaceIR.BOOL_TYPE:   "le_pack_%sBool",
    interfaceIR.CHAR_TYPE:   "le_pack_%sChar",
    interfaceIR.DOUBLE_TYPE: "le_pack_%sDouble",
    interfaceIR.SIZE_TYPE:   "le_pack_%sSize",
    interfaceIR.STRING_TYPE: "le_pack_%sString",
    interfaceIR.RESULT_TYPE: "le_pack_%sResult",
    interfaceIR.ONOFF_TYPE:  "le_pack_%sOnOff",
}

@contextfilter
def GetPackFunction(context, apiType):
    if isinstance(apiType, interfaceIR.ReferenceType):
        return "le_pack_PackReference"
    elif isinstance(apiType, interfaceIR.BitmaskType) or \
         isinstance(apiType, interfaceIR.EnumType) or \
         isinstance(apiType, interfaceIR.StructType):
        return "{}_Pack{}".format(apiType.iface.baseName, apiType.name)
    else:
        return _PackFunctionMapping[apiType] % ("Pack", )

@contextfilter
def GetUnpackFunction(context, apiType):
    if isinstance(apiType, interfaceIR.ReferenceType):
        return "le_pack_UnpackReference"
    elif isinstance(apiType, interfaceIR.BitmaskType) or \
         isinstance(apiType, interfaceIR.EnumType) or \
         isinstance(apiType, interfaceIR.StructType):
        return "{}_Unpack{}".format(apiType.iface.baseName, apiType.name)
    else:
        return _PackFunctionMapping[apiType] % ("Unpack", )

def EscapeString(string):
    return string.encode('string_escape').replace('"', '\\"')

def GetLocalParameterMsgSize(parameter, pointerSize, direction):
    if isinstance(parameter, interfaceIR.StringParameter) or \
       isinstance(parameter, interfaceIR.ArrayParameter):
        return interfaceIR.UINT32_TYPE.size + pointerSize
    else:
        return parameter.GetMaxSize(direction)

def GetLocalFunctionMsgSize(function, pointerSize):
    return max(sum([GetLocalParameterMsgSize(parameter, pointerSize, interfaceIR.DIR_IN)
                    for parameter in function.parameters]),
               sum([function.returnType.size if function.returnType else 0] +
                   [GetLocalParameterMsgSize(parameter, pointerSize, interfaceIR.DIR_OUT)
                    for parameter in function.parameters]))


def GetLocalMessageSize(interface, pointerSize):
    """
    Get size of largest possible message to a function or handler.

    A message is 4-bytes for message ID, optional 4
    bytes for required output parameters, optional 1 byte for TagID,
    and a variable number of bytes to pack
    the return value (if the function has one), and all input and output parameters.
    """
    padding = 8
    if (os.environ.get('LE_CONFIG_RPC') == "y"):
        pointerSize = pointerSize + 1
        padding = padding + 1
    return padding + max([1] +
                   [GetLocalFunctionMsgSize(function, pointerSize)
                    for function in interface.functions.values()] +
                   [handler.GetMessageSize()
                    for handler in interface.types.values()
                    if isinstance(handler, interfaceIR.HandlerType)])

def GetCOutputBufferCount(function):
    outputCount = 0
    for parameter in function.parameters:
        if parameter.direction & interfaceIR.DIR_OUT != 0:
            if isinstance(parameter, interfaceIR.ArrayParameter) or \
               isinstance(parameter, interfaceIR.StringParameter):
                outputCount += 1
    return outputCount

def GetMaxCOutputBuffers(interface):
    return max([GetCOutputBufferCount(function) for function in interface.functions.values()])

#---------------------------------------------------------------------------------------------------
# Test functions
#---------------------------------------------------------------------------------------------------
def IsSizeParameter(parameter):
    return isinstance(parameter, SizeParameter)

#---------------------------------------------------------------------------------------------------
# Global functions
#---------------------------------------------------------------------------------------------------
class SizeParameter(interfaceIR.Parameter):
    """
    C adds size parameters to the API for some string and array parameters.  Define a class for
    them here
    """
    def __init__(self, relatedParameter, direction):
        super(SizeParameter, self).__init__(interfaceIR.SIZE_TYPE,
                                            relatedParameter.name + 'Size',
                                            direction)
        self.relatedParameter = relatedParameter

def IterCAPIParameters(function):
    """
    Given a list of parameters, yield the parameters which are present in the C API.

    Effectively this coverts character array + size to null-terminated character array parameters
    for both input & output parameters, and otherwise passes through unmodified.
    """
    for parameter in function.parameters:
        if isinstance(parameter, interfaceIR.ArrayParameter):
            # Arrays have added size parameters indicating number of elements and/or buffer size
            if parameter.direction == interfaceIR.DIR_IN:
                yield parameter
                yield SizeParameter(parameter, interfaceIR.DIR_IN)
            elif parameter.direction == interfaceIR.DIR_OUT:
                yield parameter
                yield SizeParameter(parameter, interfaceIR.DIR_INOUT)
        elif (isinstance(parameter, interfaceIR.StringParameter) and
            parameter.direction == interfaceIR.DIR_OUT):
            # String out parameters take a maximum string size
            yield parameter
            yield SizeParameter(parameter, interfaceIR.DIR_IN)
        elif isinstance(parameter.apiType, interfaceIR.HandlerType):
            # Handlers get an extra context parameter
            yield parameter
            yield interfaceIR.Parameter(_CONTEXT_TYPE, 'contextPtr')
        else:
            # All other parameters just pass through
            yield parameter

    # Handlers have an extra context pointer added on at the end.
    if isinstance(function, interfaceIR.HandlerType):
        yield interfaceIR.Parameter(_CONTEXT_TYPE, 'contextPtr')

class Labeler(object):
    def __init__(self, label):
        self.label = label
        self.used = False

    def __str__(self):
        self.used = True
        return self.label

    def IsUsed(self):
        return self.used
