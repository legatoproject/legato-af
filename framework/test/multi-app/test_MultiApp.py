#
# Test harness for multi-app testing
#

import pexpect

app_name = "helloWorld"

def testMultiApp(target):
    """Expect both hello and goodbye message to be printed, in some order"""
    messages=[]
    # Sometimes prompt can be interspersed in message, so design a regex that
    # ignores the prompt.
    possibleMessages=[pexpect.TIMEOUT,
                      " Hello, World!\r\n",
                      " Goodbye, cruel World!\r\n"]
    while True:
        message=target.expect(possibleMessages, timeout=5)
        if message == 0:
            break
        messages.append(message)
    assert 1 in messages
    assert 2 in messages
