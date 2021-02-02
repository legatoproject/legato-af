#
# Functions for easing writing templates.
#
# These are:
#  - filters format single elements or small lists which would be hard to write into the
# template directly.
#  - tests to switch between templates depending on the type of item being formated
#  - global helper functions
#
# Reporting functions are largely copied from the language helpers (specifically langC).
#
# Copyright (C) Sierra Wireless Inc.
#

import interfaceIR
import os

def OptimizableArray(parameter):
    apitype = parameter.apiType
    if apitype == interfaceIR.UINT8_TYPE or apitype == interfaceIR.INT8_TYPE or \
       apitype == interfaceIR.CHAR_TYPE:
        return True
    else:
        return False

def GetLocalParameterMsgSize(parameter, pointerSize, direction):
    if isinstance(parameter, interfaceIR.StringParameter) or \
       (isinstance(parameter, interfaceIR.ArrayParameter) and OptimizableArray(parameter)):
        return interfaceIR.UINT32_TYPE.size + pointerSize
    else:
        return parameter.GetMaxSize(direction)

def GetLocalFunctionMsgSize(function, pointerSize):
    return max(sum([GetLocalParameterMsgSize(parameter, pointerSize, interfaceIR.DIR_IN)
                    for parameter in function.parameters]),
               sum([function.returnType.size if function.returnType else 0] +
                   [GetLocalParameterMsgSize(parameter, pointerSize, interfaceIR.DIR_OUT)
                    for parameter in function.parameters]))

def GetLocalIfaceMessageSize(interface, pointerSize):
    """
    Get size of largest possible message to a function or handler.

    A message is 4-bytes for message ID, optional 4
    bytes for required output parameters, optional 1 byte for TagID of required output parameters
    optional 1 byte TagID for EOF,
    and a variable number of bytes to pack
    the return value (if the function has one), and all input and output parameters.
    """
    padding = 8
    if (os.environ.get('LE_CONFIG_RPC') == "y"):
        pointerSize = pointerSize + 1
        # Include two 1-byte TagIDs
        padding = padding + 2
    return padding + max([1] +
                   [GetLocalFunctionMsgSize(function, pointerSize)
                    for function in interface.functions.values()] +
                   [handler.GetMessageSize()
                    for handler in interface.types.values()
                    if isinstance(handler, interfaceIR.HandlerType)])

def GetLocalMessageSize(interfaceObj, pointerSize):
    if isinstance(interfaceObj, interfaceIR.Interface):
        return GetLocalIfaceMessageSize(interfaceObj, pointerSize)
    if isinstance(interfaceObj, interfaceIR.Function):
        return GetLocalFunctionMsgSize(interfaceObj, pointerSize)
    if isinstance(interfaceObj, interfaceIR.HandlerType):
        return interfaceObj.GetMessageSize()
    raise TypeError("Object does not require a message", interfaceObj)

def GetMessageSize(interfaceObj):
    if isinstance(interfaceObj, interfaceIR.Interface) or \
       isinstance(interfaceObj, interfaceIR.Function) or \
       isinstance(interfaceObj, interfaceIR.HandlerType):
        return interfaceObj.GetMessageSize()
    raise TypeError("Object does not require a message", interfaceObj)
