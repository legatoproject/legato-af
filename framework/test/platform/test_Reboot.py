#
# Test reboot() platform function
#

import pexpect

app_name = "rebootTest"

def testReboot(target):
    """Expect reboot() operation"""
    assert target.expect_exact("\r\nUnit test for platform dependent function: reboot() call\r\n",
                               timeout=15) == 0
    assert target.expect_exact("Machine reboot\r\n", timeout=15) == 0
    assert target.expect_exact("Hit '\\r' or '\\e' key to stop autoboot: ", timeout=15) == 0
