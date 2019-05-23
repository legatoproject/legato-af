#
# Custom KConfig preprocessor functions for Legato configuration.
#
# Copyright (C) Sierra Wireless Inc.
#

import os
import shutil

def b2k(kconf, _, val):
    """Convert a binary 1/0 value to a KConfig y/n."""
    return "y" if val in ("1", "y") else "n"

def empty(kconf, _, val):
    """Convert an empty/non-empty string to a KConfig y/n."""
    return "y" if not val else "n"

def dfault(kconf, _, val, defval):
    """If a value is empty, use the provided default."""
    return val if val else defval

def getccache(kconfig, _):
    """Find the local version of ccache to use, if any."""
    ccache = os.environ.get("CCACHE", "")
    if not ccache:
        try:
            ccache = shutil.which("sccache") or ""
            if not ccache:
                ccache = shutil.which("cache") or ""
        except:
            pass
    return ccache

functions = {
    "b2k":       (b2k,       1, 1),
    "def":       (dfault,    2, 2),
    "empty":     (empty,     1, 1),
    "getccache": (getccache, 0, 0)
}
