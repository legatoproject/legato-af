#
# Init file for the C language package
#
# Copyright (C) Sierra Wireless Inc.
#

import codeGenHelpers

def AddLangArgumentGroup(argParser):
    pass

# Custom filters needed for C templates
Filters = { 'FormatHeaderComment': codeGenHelpers.FormatHeaderComment,
            'FormatType':          codeGenHelpers.FormatType,
            'FormatBoxedType':     codeGenHelpers.FormatBoxedType,
            'DefaultValue':        codeGenHelpers.GetDefaultValue,
            'FormatParameter':     codeGenHelpers.FormatParameter,
            'indent':              codeGenHelpers.IndentCode }

# No custom tests for C templates
Tests = { }

Globals = {  }

GeneratedFiles = { 'interface' : 'io/legato/api/%s.java',
                   'client' : 'io/legato/api/implementation/%sClient.java',
                   'server' : 'io/legato/api/implementation/%sServer.java' }
