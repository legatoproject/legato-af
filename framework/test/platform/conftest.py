#
# Customize test collection to ignore platform dependant tests on Linux platform
#
# Copyright (C) Sierra Wireless Inc.
#

import os

def pytest_ignore_collect(path, config):
    if ((os.environ.get('LE_CONFIG_LINUX') == "y") and
        (path.basename == "testPlatform.adef" or
        path.basename == "exitTest.adef" or
        path.basename == "rebootTest.adef")):
        return True
