#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

mkapp(  avcUserApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
     )

mkapp(  avcCtrlApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
     )

mkapp(  avcDataApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
     )

mkapp(  avcObserveApp.adef
            -i ${LEGATO_ROOT}/interfaces/airVantage
     )

#
# Test individual parts
#

add_subdirectory(assetData)
