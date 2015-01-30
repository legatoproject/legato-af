#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  rogue
            rogue.c
            -o ${EXECUTABLE_OUTPUT_PATH}/rogue
            -i ${LEGATO_ROOT}/framework/c/src
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            rogue.c
     )
