#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LEGATO_AT C)

set(LEGATO_COMPONENTS_AT_TARGET  le_mdm_at)

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")

    mkcomp(../at
                -v
            )

endif()
