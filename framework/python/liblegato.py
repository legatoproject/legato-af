"""
Wrapper for _liblegato_py.so C Extension.
"""

from _liblegato_py import ffi, lib
from enum import IntEnum, Enum
import inspect
import sys
import types
from functools import wraps

__copyright__ = 'Copyright (C) Sierra Wireless Inc.'

def convert_to_py(obj):
    """
    Convert a ffi CData object to a python representation
    """
    if not isinstance(obj, ffi.CData):
        return obj
    type=ffi.typeof(obj)
    if type.kind == 'primitive':
        if 'char' in type.cname: # includes wchar, etc
            return ffi.string(obj)
        return int(obj)
    elif type.kind == 'pointer':
        if type.item.cname == 'char':
            return ffi.string(obj)
        else:
            # if it's not a char*, then it's an opaque reference that we shouldn't touch
            return obj
    elif type.kind == 'array':
            return [ convert_to_py(obj[i]) for i in range(type.length) ]
    else:
        print("convert_to_py() can't convert: ")
        print(type.kind)
        print(type)
        print(type.cname)
        return obj

def convert_to_c(obj):
    """
    Convert a python object to ffi CData
    """
    if isinstance(obj, ffi.CData):
        return obj
    if obj is None:
        return ffi.NULL
    if isinstance(obj, basestring):
        return ffi.new("char[]", obj)
    if obj is list or obj is tuple:
        return [ convert_to_c[x] for x in obj ]
    if isinstance(obj, Enum):
        return obj.value
    return obj

# -aliases to hide ffi methods-
# I have decided not to include this in the automatic conversion function,
# because C pointers MUST be kept alive, or else the data they point to
# is freed.
# For example, if a dict was implicitly converted to a pointer in something like
# le_foo_SetContext({'a':1})
# it will crash when deferencing later in a callback, because the pointer created
# would only be valid for the SetContext function call. As soon as SetContext
# returns, the C handle is freed.
# For more info see https://cffi.readthedocs.io/en/latest/using.html#working
# So! For explicit usage:
py_to_ptr = ffi.new_handle
ptr_to_py = ffi.from_handle
# -end ffi aliases-


class Result(IntEnum):
    """
    Python equivalent to C enum le_result_t
    """
    LE_OK = 0
    LE_NOT_FOUND = -1
    LE_NOT_POSSIBLE = -2
    LE_OUT_OF_RANGE = -3
    LE_NO_MEMORY = -4
    LE_NOT_PERMITTED = -5
    LE_FAULT = -6
    LE_COMM_ERROR = -7
    LE_TIMEOUT = -8
    LE_OVERFLOW = -9
    LE_UNDERFLOW = -10
    LE_WOULD_BLOCK = -11
    LE_DEADLOCK = -12
    LE_FORMAT_ERROR = -13
    LE_DUPLICATE = -14
    LE_BAD_PARAMETER = -15
    LE_CLOSED = -16
    LE_BUSY = -17
    LE_UNSUPPORTED = -18
    LE_IO_ERROR = -19
    LE_NOT_IMPLEMENTED = -20
    LE_UNAVAILABLE = -21
    LE_TERMINATED = -22


class Unbuffered(object):
   """
   Remove stream buffering
   """
   def __init__(self, stream):
       self.stream = stream
   def write(self, data):
       for chunk in data.split('\n'):
           self.stream.write(chunk+'\n')
           self.stream.flush()
   def writelines(self, datas):
       self.stream.writelines(datas)
       self.stream.flush()
   def __getattr__(self, attr):
       return getattr(self.stream, attr)

sys.stdout = Unbuffered(sys.stdout)

def convert_args_decorator(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        for k, v in kwargs.iteritems():
            kwargs[k] = convert_to_c(v)
        args = map(convert_to_c, args)
        return convert_to_py(f(*args, **kwargs))
    return wrapper

current_module = __import__(__name__)

# apply casting decorator to each function in lib, and import it into this module
for name in dir(lib):
    obj = getattr(lib, name)
    # BuiltinFunction refers to native C functions.
    # Leave internal things that start with '_le' untouched.
    if isinstance(obj, types.BuiltinFunctionType) and name.startswith("le_"):
        if hasattr(obj, '__module__'):
            obj.__module__ = __name__
        setattr(current_module, name, convert_args_decorator(obj))
    else:
        # for types, enums, etc, just import into this module
        setattr(current_module, name, obj)

def _le_log_msg(level, formatString, *args):
    frame, filename, lineno, function, lines, index = inspect.stack()[2]
    args = map(convert_to_py, args) # convert args to python for formatting
    if isinstance(formatString, basestring):
        formatString = formatString % tuple(args)
    elif not isinstance(formatString, ffi.CData): # if it's a non-str python object
        formatString = str(formatString) # convert it to a string (unsure if this should be allowed)
    if level >= le_log_GetFilterLevel():
        _le_log_Send(level, ffi.NULL, lib.LE_LOG_SESSION, ffi.new('char[]', filename),
                     ffi.new('char[]', function), lineno, formatString)

def LE_INFO(formatString, *args):
    _le_log_msg(LE_LOG_INFO, formatString, *args)

def LE_WARN(formatstring, *args):
    _le_log_msg(LE_LOG_WARN, formatString, *args)

def LE_DEBUG(formatString, *args):
    _le_log_msg(LE_LOG_DEBUG, formatString, *args)

def LE_ERR(formatString, *args):
    _le_log_msg(LE_LOG_ERR, formatString, *args)

def LE_CRIT(formatString, *args):
    _le_log_msg(LE_LOG_CRIT, formatString, *args)

def LE_EMERG(formatString, *args):
    _le_log_msg(LE_LOG_EMERG, formatString, *args)
