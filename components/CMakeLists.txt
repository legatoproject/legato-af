#*******************************************************************************
# Copyright (C) 2013-2014, Sierra Wireless Inc.
# Use of this work is subject to license.
#*******************************************************************************

### Only the Air Vantage support components get built into libraries.
### Everything else under this directory gets included as a component
### in an exe or app.
if(INCLUDE_AIRVANTAGE)
    add_subdirectory(airvantage)
endif()
