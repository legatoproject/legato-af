#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


mkexe(  xattr
            xattr.c
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
        DEPENDS
            xattr.c
            )
