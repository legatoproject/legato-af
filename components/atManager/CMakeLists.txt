#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LEGATO_ATMGR C)

set(LEGATO_COMPONENTS_AT_MANAGER_TARGET  le_mdm_atmgr)

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")

    mkcomp(../atManager
                -i devices/uart/inc
                -i devices/adapter_layer/inc
                -v
            )

endif()
