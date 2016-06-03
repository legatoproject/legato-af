#
# Define types/classes for different code/message objects
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    uint8   = "s_uint8",
    uint16  = "s_uint16",
    uint32  = "s_uint32",
    uint64  = "s_uint64",
    uint    = "s_uint32",

    # signed ints
    int8    = "s_int8",
    int16   = "s_int16",
    int32   = "s_int32",
    int64   = "s_int64",
    int     = "s_int32",

    # boolean
    bool = "s_bool",

    # a single char -- this is not for string support
    char    = "s_char",

    # double is sufficient -- no need to support float as well.
    double = "s_double",

    # for array sizes and indices, and things like that
    size = 's_size',

    # these are not used for C, but may be needed for other languages
    string = 's_string',
    file = 's_file',

    # TODO: temporary until supported in le.api files
    le_result_t = 'le_result_t'
)

commonTypes.InitPredefinedInterfaces(DefinedInterfaceTypes)



class HandlerFuncData(commonTypes.HandlerFuncData):

    def GetLangType(self, typeName):
        # NOTE: Don't need the ImportName, unlike below, since handler defns can't be imported
        return 's_Func_'+typeName


class ReferenceData(commonTypes.ReferenceData):

    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix("s_Ref_"+typeName)


class EnumData(commonTypes.EnumData):

    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix("s_Enum_"+typeName)


class BitMaskData(commonTypes.BitMaskData):

    def GetLangType(self, typeName):
        return commonTypes.AddImportNamePrefix("s_Bitmask_"+typeName)


