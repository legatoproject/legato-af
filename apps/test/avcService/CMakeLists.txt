#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if(LEGATO_AVC_APP STREQUAL "legacy")

    mkapp(  avcUserApp.adef
                -i ${LEGATO_ROOT}/interfaces/airVantage/legacy
         )

    mkapp(  avcCtrlApp.adef
                -i ${LEGATO_ROOT}/interfaces/airVantage/legacy
         )

    mkapp(  avcDataApp.adef
                -i ${LEGATO_ROOT}/interfaces/airVantage/legacy
         )

    mkapp(  avcObserveApp.adef
                -i ${LEGATO_ROOT}/interfaces/airVantage/legacy
         )

    mkapp(  avcTimeSeriesApp.adef
                -i ${LEGATO_ROOT}/interfaces/airVantage/legacy
         )

    # This is a C test
    add_dependencies(tests_c avcCtrlApp avcDataApp avcObserveApp avcTimeSeriesApp)

endif()

#
# Test individual parts
#

add_subdirectory(assetData)
