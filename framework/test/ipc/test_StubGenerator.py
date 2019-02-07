#
# Test generating stub functions using ifgen
#
# Copyright (C) Sierra Wireless Inc.
#

from __future__ import print_function
import os, shutil, tempfile

def test_Stub():
    tmpdir = tempfile.mkdtemp()
    scriptdir = os.path.dirname(os.path.realpath(__file__))
    assert os.system("ifgen --gen-server-stub --gen-server-interface --output-dir {dir} {testdir}/interfaces/ipcTest.api".format(dir=tmpdir, testdir=scriptdir)) == 0
    with open("{dir}/interface.h".format(dir=tmpdir), "w") as interfaceFile:
        print("#include \"ipcTest_server.h\"", file=interfaceFile)
    assert os.system("$(findtoolchain $TARGET_TYPE dir)/$(findtoolchain $TARGET_TYPE prefix)gcc --sysroot=$(findtoolchain $TARGET_TYPE sysroot) -I$LEGATO_ROOT/framework/include -I$LEGATO_ROOT/build/$TARGET_TYPE/framework/include -c {dir}/ipcTest_stub.c -o {dir}/ipcTest.o".format(dir=tmpdir)) == 0
    shutil.rmtree(tmpdir)

def test_AsyncStub():
    tmpdir = tempfile.mkdtemp()
    scriptdir = os.path.dirname(os.path.realpath(__file__))
    assert os.system("ifgen --gen-server-stub --gen-server-interface --async-server --output-dir {dir} {testdir}/interfaces/ipcTest.api".format(dir=tmpdir,testdir=scriptdir)) == 0
    with open("{dir}/interface.h".format(dir=tmpdir), "w") as interfaceFile:
        print("#include \"ipcTest_server.h\"", file=interfaceFile)
    assert os.system("$(findtoolchain $TARGET_TYPE dir)/$(findtoolchain $TARGET_TYPE prefix)gcc --sysroot=$(findtoolchain $TARGET_TYPE sysroot) -I$LEGATO_ROOT/framework/include -I$LEGATO_ROOT/build/$TARGET_TYPE/framework/include -c {dir}/ipcTest_stub.c -o {dir}/ipcTest.o".format(dir=tmpdir)) == 0
    shutil.rmtree(tmpdir)
