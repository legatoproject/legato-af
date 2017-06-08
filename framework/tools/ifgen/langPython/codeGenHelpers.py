import re
import interfaceIR
import sys

#---------------------------------------------------------------------------------------------------
# Globals
#---------------------------------------------------------------------------------------------------

re_camelcase_to_underscores = re.compile('((?<=[a-z0-9])[A-Z]|(?!^)[A-Z](?=[a-z]))')

#---------------------------------------------------------------------------------------------------
# Filters
#---------------------------------------------------------------------------------------------------

def DEBUG_show_all_attrs(value):
    res = []
    for k in dir(value):
        res.append('%r %r\n' % (k, getattr(value, k)))
    return '\n'.join(res)

def FormatHeaderComment(comment):
    """
    Format a header comment into a Python comment
    """
    commentLines = comment.split(u'\n')
    return "'''" + u'\n'.join([line.rstrip() for line in commentLines]) + "'''"

def DecoratorNameForEvent(event):
    return CamelCaseToUnderscores(event.name) + "_handler"

def CamelCaseToUnderscores(name):
    """
    Convert CamelCase to pythonic_underscores
    """
    return re_camelcase_to_underscores.sub(r'_\1', name).lower()

def HandlerParamsForEvent(event):
    handler = next(param for param in event.parameters if isinstance(param.apiType, interfaceIR.HandlerType))
    return handler.apiType.parameters

def CDataToPython(var):
    if isinstance(var.apiType, interfaceIR.BitmaskType):
        # Convert cdata to enum
        cast = "%s(%s[0])" % (var.apiType.name.replace("BitMask", ""), var.name)
    elif re.match(r'u?int64', var.apiType.name):
        cast = "long(%s)" % var.name
    elif re.match(r'u?int\d*', var.apiType.name) or var.apiType in [interfaceIR.CHAR_TYPE, interfaceIR.SIZE_TYPE]:
        cast = "int(%s)" % var.name
    elif var.apiType == interfaceIR.STRING_TYPE:
        cast = "ffi.string(%s)" % var.name
    elif var.apiType in [interfaceIR.BOOL_TYPE,interfaceIR.ONOFF_TYPE]:
        cast = "bool(%s)" % var.name
    elif var.apiType == interfaceIR.DOUBLE_TYPE:
        cast = "float(%s)" % var.name
    elif var.apiType == interfaceIR.RESULT_TYPE:
        cast = "Result(%s)" % var.name
    elif isinstance(var.apiType, interfaceIR.EnumType):
        cast = "%s(%s[0])" % (var.apiType.name, var.name)
    else:
        sys.stderr.write(
            "Couldn't convert variable '%s' of type '%s' from C to Python." %
            (var.name, var.apiType.name))
        return "# No rule for converting %s (%s) to Python type."
    return var.name + " = " + cast
