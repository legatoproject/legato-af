#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

mkapp(  avcUserApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/airVantage/*
            componentUserApp/* 
     )


mkapp(  avcCtrlApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/airVantage/*
            componentCtrlApp/* 
     )


mkapp(  avcDataApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/airVantage/*
            componentDataApp/* 
     )


#
# Test individual parts
#

add_subdirectory(assetData)
