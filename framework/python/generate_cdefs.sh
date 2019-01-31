#!/bin/bash

# Cleans up liblegato header files into definitions that cffi can understand.

set -e
out_dir=$1
for filename in "${LEGATO_ROOT}"/framework/include/le_* ; do
    new_filename="$(basename "${filename//.h/_cdef.h}")"
    if [ "$(basename "$filename")" =  le_basics.h ]; then
        cppflags=
    else
        cppflags="-imacros ${LEGATO_ROOT}/framework/include/le_basics.h"
    fi
    # Word splitting of cppflags is intentional:
    # shellcheck disable=SC2086
    ${CC} -E -imacros "${LEGATO_ROOT}/build/$TARGET/framework/include/le_config.h" $cppflags \
            '-D__attribute__(x)=' "$filename"                       | # run preprocessor
        "${LEGATO_ROOT}/framework/python/remove_implementations.py" |
        sed -z 's/\n;/;/g' > "$out_dir/$new_filename"
        # sed moves trailing semicolons to previous line (to appease cffi's CParser)
done
