#
# Functions for easing writing templates.
#
# These are:
#  - filters format single elements or small lists which would be hard to write into the
# template directly.
#  - tests to switch between templates depending on the type of item being formated
#  - global helper functions
#
# Copyright (C) Sierra Wirless Inc.
#

import interfaceIR
from jinja2 import environmentfilter

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

def FormatType(apiType):
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
    elif isinstance(apiType, interfaceIR.ReferenceType):
        return "%s_%sRef_t" % (apiType.iface.name, apiType.name)
    elif isinstance(apiType, interfaceIR.HandlerType):
        return "%s_%sFunc_t" % (apiType.iface.name, apiType.name)
    else:
        return "%s_%s_t" % (apiType.iface.name, apiType.name)

def DecorateName(name):
    if name in _KEYWORDS:
        return '_' + name
    else:
        return name

def FormatParameterName(parameter, forceInput=False):
    if (isinstance(parameter, interfaceIR.ArrayParameter)
        or isinstance(parameter.apiType, interfaceIR.HandlerType)
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
def FormatParameter(env, parameter, forceInput=False):
    if isinstance(parameter, interfaceIR.StringParameter):
        return ((u"const " if forceInput or parameter.direction == interfaceIR.DIR_IN else u"") +
                FormatType(parameter.apiType) +
                (u" LE_NONNULL " if not env.globals.get('dontGenerateAttrs') and
                 parameter.direction == interfaceIR.DIR_IN else u" ") +
                parameter.name)
    elif isinstance(parameter, interfaceIR.ArrayParameter):
        return ((u"const " if forceInput or parameter.direction == interfaceIR.DIR_IN else u"") +
                FormatType(parameter.apiType) + "* " + parameter.name + "Ptr")
    elif isinstance(parameter.apiType, interfaceIR.HandlerType):
        return FormatType(parameter.apiType) + " " + parameter.name + "Ptr"
    elif forceInput or parameter.direction == interfaceIR.DIR_IN:
        return FormatType(parameter.apiType) + " " + DecorateName(parameter.name)
    else:
        return FormatType(parameter.apiType) + "* " + parameter.name + "Ptr"

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

def GetPackFunction(apiType):
    if isinstance(apiType, interfaceIR.ReferenceType):
        return "le_pack_PackReference"
    elif isinstance(apiType, interfaceIR.BitmaskType) or \
         isinstance(apiType, interfaceIR.EnumType):
        if (apiType.size == 4):
            return "le_pack_PackUint32"
        elif (apiType.size == 8):
            return "le_pack_PackUint64"
        else:
            raise KeyError(apiType.name)
    else:
        return _PackFunctionMapping[apiType] % ("Pack", )

def GetUnpackFunction(apiType):
    if isinstance(apiType, interfaceIR.ReferenceType):
        return "le_pack_UnpackReference"
    elif isinstance(apiType, interfaceIR.BitmaskType) or \
         isinstance(apiType, interfaceIR.EnumType):
        if (apiType.size == 4):
            return "le_pack_UnpackUint32"
        elif (apiType.size == 8):
            return "le_pack_UnpackUint64"
        else:
            raise KeyError(apiType.name)
    else:
        return _PackFunctionMapping[apiType] % ("Unpack", )

def EscapeString(string):
    return string.encode('string_escape').replace('"', '\\"')

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
