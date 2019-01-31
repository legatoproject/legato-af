#
# Test FD data port features
#
import os
import pexpect
import pytest
import sys
import time

app_name = "fdDataPort"
send_string = "testing 1 2 3"

@pytest.mark.skipif(os.getenv('DATA_PORT') is None, reason="DATA_PORT variable not set.")
def testFdDataPort(target, serialport, testloop):
    """Send text to serial port"""
    port_name = os.getenv('DATA_PORT')
    port = serialport.open(port_name, 115200)
    if port is None:
        pytest.fail("Failed to open data port at {0}!".format(port_name))
        return

    handlers = {}

    # Send test string after test step 2
    handlers[2] = {
        'expected': r'Waiting for ENTER-terminated user input on serial port',
        'poststep': lambda step: port.io.send(send_string + "\r")
    }
    # Receive test string at step 3
    handlers[3] = {
        'expected': r"Wrote {0} bytes back to serial data port \(input: '{1}'\)".format(
            len(send_string), send_string),
        'poststep': lambda step:                                            \
                port.io.expect_exact([send_string, pexpect.TIMEOUT]) == 0   \
            or  step.fail("Did not receive string from data port.")
    }

    testloop(target, handlers)
