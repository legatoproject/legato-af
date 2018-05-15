#
# Init file for the C language package
#
# Copyright (C) Sierra Wireless Inc.
#

import codeGenHelpers

def AddLangArgumentGroup(parser):
    parser.add_argument('--async-server',
                        dest="async",
                        action='store_true',
                        default=False,
                        help='generate asynchronous-style server functions')

# Custom filters needed for C templates
Filters = { 'DecorateName':        codeGenHelpers.DecorateName,
            'EscapeString':        codeGenHelpers.EscapeString,
            'FormatHeaderComment': codeGenHelpers.FormatHeaderComment,
            'FormatDirection':     codeGenHelpers.FormatDirection,
            'FormatType':          codeGenHelpers.FormatType,
            'FormatParameterName': codeGenHelpers.FormatParameterName,
            'FormatParameterPtr':  codeGenHelpers.FormatParameterPtr,
            'FormatParameter':     codeGenHelpers.FormatParameter,
            'GetParameterCount':   codeGenHelpers.GetParameterCount,
            'GetParameterCountPtr': codeGenHelpers.GetParameterCountPtr,
            'PackFunction':        codeGenHelpers.GetPackFunction,
            'UnpackFunction':      codeGenHelpers.GetUnpackFunction,
            'CAPIParameters':      codeGenHelpers.IterCAPIParameters }


Tests = { 'SizeParameter':         codeGenHelpers.IsSizeParameter }

Globals = { 'Labeler':             codeGenHelpers.Labeler }

GeneratedFiles = { 'interface' : '%s_interface.h',
                   'local' : '%s_messages.h',
                   'client' : '%s_client.c',
                   'server-interface' : '%s_server.h',
                   'server' : '%s_server.c' }
