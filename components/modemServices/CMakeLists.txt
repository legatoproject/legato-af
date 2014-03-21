#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

### Devices
add_subdirectory(platformAdaptor/at/atManager/devices/uart)

# TODO: Move AT implementation dependencies
add_subdirectory(platformAdaptor/at/atManager)

### Platform Adaptor
add_subdirectory(platformAdaptor)

### Component Implementation
add_subdirectory(implementation)

