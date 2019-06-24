#
# Init file for the C language package
#
# Copyright (C) Sierra Wireless Inc.
#

import codeGenHelpers
# No extra arguments, no helpers needed, and only one generated file -- for rpcConfig
def AddLangArgumentGroup(parser):
    pass

Filters = {
    'serialize_iface' : codeGenHelpers.serialize_iface,
}
Tests = { }
Globals = { }

GeneratedFiles = { 'dump' : '%s.json' }
