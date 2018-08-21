#
# Helpers for creating templates.
#
# This file includes:
#  - Tests to check parameter types, inputs vs. outputs, etc.
#
# Copyright (c) Sierra Wireless Inc.
#

import interfaceIR
import itertools
from jinja2 import contextfunction


#---------------------------------------------------------------------------------------------------
# Tests
#---------------------------------------------------------------------------------------------------

### Type tests
def IsBasicType(typeObj):
    return isinstance(typeObj, interfaceIR.BasicType)

def IsEnumType(typeObj):
    return isinstance(typeObj, interfaceIR.EnumType)

def IsBitMaskType(typeObj):
    return isinstance(typeObj, interfaceIR.BitmaskType)

def IsStructType(typeObj):
    return isinstance(typeObj, interfaceIR.StructType)

def IsHandlerType(typeObj):
    return isinstance(typeObj, interfaceIR.HandlerType)

def IsReferenceType(typeObj):
    return isinstance(typeObj, interfaceIR.ReferenceType)

def IsHandlerReferenceType(typeObj):
    return isinstance(typeObj, interfaceIR.HandlerReferenceType)

### Parameter tests
def IsInParameter(paramObj):
    return (paramObj.direction & interfaceIR.DIR_IN) == interfaceIR.DIR_IN

def IsOutParameter(paramObj):
    return (paramObj.direction & interfaceIR.DIR_OUT) == interfaceIR.DIR_OUT

def IsStringParameter(paramObj):
    return isinstance(paramObj, interfaceIR.StringParameter)

def IsArrayParameter(paramObj):
    return isinstance(paramObj, interfaceIR.ArrayParameter)

### Structure member tests
def IsStringMember(memberObj):
    return isinstance(memberObj, interfaceIR.StructStringMember)

def IsArrayMember(memberObj):
    return isinstance(memberObj, interfaceIR.StructArrayMember)

### Function tests
def HasCallbackFunction(typeObj):
    """Does this function have a callback?"""
    return len([parameter
                for parameter in typeObj.parameters
                if isinstance(parameter.apiType, interfaceIR.HandlerType)]) > 0

def IsEventFunction(typeObj):
    """Event functions are add/remove handler functions"""
    return isinstance(typeObj, interfaceIR.EventFunction)

def IsAddHandlerFunction(functionObj):
    return (isinstance(functionObj, interfaceIR.EventFunction)
            and functionObj.name.startswith("Add"))

def IsRemoveHandlerFunction(functionObj):
    return (isinstance(functionObj, interfaceIR.EventFunction)
            and functionObj.name.startswith("Remove"))

### Other helper tests
@contextfunction
def AnyFilter(context, iterable, filterName):
    filterFunc = context.environment.tests.get(filterName)
    return any(itertools.imap(lambda item: filterFunc(item),
                              iterable))
