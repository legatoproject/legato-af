#
# Test exit() platform function
#

import pexpect

app_name = "exitTest"

def testExit(target):
    """Expect exit() operation"""
    target.expect_exact("\r\nUnit test for platform dependent function: exit() call\r\n",
        timeout=60)
    target.expect_exact("\r\nFreeRtos task test exit\r\n", timeout=60)
    target.expect_exact("\r\n_exit was called with code 0 No return!!!!\r\n", timeout=60)
