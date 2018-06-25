import codeGenHelpers

import sys
sys.path.append('../')
import langC
import langC.codeGenHelpers

def AddLangArgumentGroup(argParser):
    pass

Filters = {
            'DecorateName':          langC.codeGenHelpers.DecorateName,
            'ToUnderscores' :        codeGenHelpers.CamelCaseToUnderscores,
            'DEBUG_show_all_attrs' : codeGenHelpers.DEBUG_show_all_attrs,
            'PyFormatHeaderComment': codeGenHelpers.FormatHeaderComment,
            'DecoratorNameForEvent' : codeGenHelpers.DecoratorNameForEvent,
            'HandlerParamsForEvent' : codeGenHelpers.HandlerParamsForEvent,
            'CDataToPython'         : codeGenHelpers.CDataToPython,
            'FormatHeaderComment': langC.codeGenHelpers.FormatHeaderComment,
            'FormatDirection':     langC.codeGenHelpers.FormatDirection,
            'FormatType':          langC.codeGenHelpers.FormatType,
            'FormatParameterName': langC.codeGenHelpers.FormatParameterName,
            'FormatParameterPtr':  langC.codeGenHelpers.FormatParameterPtr,
            'FormatParameter':     langC.codeGenHelpers.FormatParameter,
            'GetParameterCount':   langC.codeGenHelpers.GetParameterCount,
            'GetParameterCountPtr': langC.codeGenHelpers.GetParameterCountPtr,
            'PackFunction':        langC.codeGenHelpers.GetPackFunction,
            'UnpackFunction':      langC.codeGenHelpers.GetUnpackFunction,
            'CAPIParameters':      langC.codeGenHelpers.IterCAPIParameters,
            'EscapeString':        langC.codeGenHelpers.EscapeString,
            }


Tests = { 'SizeParameter':         langC.codeGenHelpers.IsSizeParameter }

Globals = langC.Globals.copy()
Globals.update({
    'dontGenerateAttrs': True
})

GeneratedFiles = { 'interface' : '%s.py',
                   'cdef' : '%s_cdef.h',
                   'c_interface' : 'C/%s_interface.h',
                   'c_local' : 'C/%s_messages.h',
                   'c_client' : 'C/%s_client.c',
                   'c_server-interface' : 'C/%s_server.h',
                   'c_server' : 'C/%s_server.c'
                   }
