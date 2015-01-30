#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LEGATO_ATMGR C)

find_package(Legato REQUIRED)

set(LEGATO_COMPONENTS_AT_MANAGER_TARGET  le_mdm_atmgr)

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")

    mkcomp(../atManager
                -i devices/uart/inc
                -i devices/adapter_layer/inc
                -v
            DEPENDS
                devices/uart/src/le_uart.c
                src/init.c
                src/atCmdSync.c
                src/atMachineCommand.c
                src/atMachineDevice.c
                src/atMachineManager.c
                src/atMachineMgr.c
                src/atMachineMgrItf.c
                src/atMachineParser.c
                src/atMachineString.c
                src/atMachineUnsolicited.c
                src/atPorts.c
                legato
            )

endif()
