#
# Define types/classes for different code/message objects
#
# Copyright (C) Sierra Wireless Inc.
#

# Python libraries
import logging

# ifgen specific libraries
import commonTypes


#
# Explicitly import all the functions and classes, and then override as needed.
#
from commonTypes import *


#
# Language specific support
#

DefinedInterfaceTypes = dict(
    # unsigned ints
    uint8   = "byte",
    uint16  = "short",
    uint32  = "int",
    uint64  = "long",
    uint    = "int",

    # signed ints
    int8    = "byte",
    int16   = "short",
    int32   = "int",
    int64   = "long",
    int     = "int",

    # boolean
    bool = "boolean",

    # a single char -- this is not for string support
    char    = "byte",

    # double is sufficient -- no need to support float as well.
    double = "double",

    # for array sizes and indices, and things like that
    size = 'long',

    # these are not used for C, but may be needed for other languages
    string = 'String',
    file = 'FileDescriptor',

    # TODO: temporary until supported in le.api files
    le_result_t = 'Result'
)

commonTypes.InitPredefinedInterfaces(DefinedInterfaceTypes)



class HandlerFuncData(commonTypes.HandlerFuncData):

    def GetLangType(self, typeName):
        # NOTE: Don't need the ImportName, unlike below, since handler defns can't be imported
        return typeName


class ReferenceData(commonTypes.ReferenceData):

    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix(typeName + "Ref")


class EnumData(commonTypes.EnumData):

    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix(typeName)


class BitMaskData(commonTypes.BitMaskData):

    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix(typeName)


class EventFuncData(commonTypes.EventFuncData):

    def __init__(self, funcName, parmList, comment=""):
        super(commonTypes.EventFuncData, self).__init__(funcName, '', parmList, comment)


    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix(typeName + "Event")
