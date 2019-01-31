#
# Customize test collection to ignore Unix sockets on non-POSIX platforms
#
# Copyright (C) Sierra Wireless Inc.
#

import os

def pytest_ignore_collect(path, config):
    if os.environ.get('LE_CONFIG_LINUX') != "y" and path.basename == "testUnixMessaging.adef":
        return True
