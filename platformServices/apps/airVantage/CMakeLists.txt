#**********************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#**********************************************************************************

find_package(Legato REQUIRED)


mkapp(  avcService.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/airVantage/*
            ${LEGATO_ROOT}/components/airVantage/*
        )

