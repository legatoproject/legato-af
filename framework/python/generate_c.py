from cffi import FFI
import os
import re
import sys

"""

Generates a CPython extension for liblegato using cffi.
Once compiled, it can be used to call liblegato functions from Python.

Usage:
    generate_c.py cdef_dir/ out_dir/
"""


ffibuilder = FFI()


cdef = """
typedef int... time_t;
typedef int... mode_t;
typedef int... uid_t;
typedef int... pid_t;
"""


# workaround for <fts.h> not working with <pyconfig.h>'s file offset define
# should be used with -D_FTS_H
c_source = """
#undef __USE_FILE_OFFSET64
#undef _FTS_H
#include <legato.h>
"""

filenames = os.listdir(sys.argv[1])

# order header files by the same order that they are found in inside legato.h
# so that e.g. typedefs are not referenced before declaration

with open(os.path.join(os.environ["LEGATO_ROOT"], 'framework/include/', "legato.h")) as f:
    legatoh = f.readlines()

def index_in_legatoh(header):
    return next((lineno for lineno, line in enumerate(legatoh)
                if line.startswith('#include "%s"' % header)), 100)

filenames = sorted(filenames, key=lambda x: index_in_legatoh(x.replace('_cdef','')))


for fname in filenames:
    with open(os.path.join(sys.argv[1], fname)) as f:
        cdef += f.read() + "\n"
    m = re.search("le_(.+)_cdef.h",fname)
    if m:
        c_source += '#include <le_%s.h>\n' % m.group(1)
    else:
        print("Warning, wrong filename format for '%s', so we're not including the implementation header.")

ffibuilder.cdef(cdef)
ffibuilder.set_source("_liblegato_py", c_source)


if __name__ == "__main__":
    ffibuilder.emit_c_code(os.path.join(sys.argv[2], "_liblegato_py.c"))
