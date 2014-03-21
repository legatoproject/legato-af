# Copyright (c) 2007-2011, Lloyd Hilaiel <lloyd@hilaiel.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

PROJECT(LIB_YAJL)

SET (SRCS yajl.c yajl_lex.c yajl_parser.c yajl_buf.c
          yajl_encode.c yajl_gen.c yajl_alloc.c
          yajl_tree.c yajl_version.c
)
SET (HDRS yajl_parser.h yajl_lex.h yajl_buf.h yajl_encode.h yajl_alloc.h)
SET (PUB_HDRS api/yajl_parse.h api/yajl_gen.h api/yajl_common.h api/yajl_tree.h)

INCLUDE_DIRECTORIES(./include)

# ADD_LIBRARY(lib_yajl_static STATIC ${SRCS} ${HDRS} ${PUB_HDRS})
# SET_TARGET_PROPERTIES(lib_yajl_static PROPERTIES OUTPUT_NAME yajl)

ADD_LIBRARY(lib_yajl SHARED ${SRCS} ${HDRS} ${PUB_HDRS})
SET_TARGET_PROPERTIES(lib_yajl PROPERTIES OUTPUT_NAME yajl)

INSTALL(TARGETS lib_yajl LIBRARY DESTINATION lib)
