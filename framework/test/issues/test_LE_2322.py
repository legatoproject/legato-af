#
# Test harness for LE-2322: le_thread_CleanupLegatoThreadData doesn't seem to cleanup properly
# anymore
#
# The app is expected to abort when an attempt is made to access the thread info after cleanup.
#

app_name = 'LE_2322'

def testLogFilter(target):
    assert target.expect('Legato threading API used in non-Legato thread!', timeout=10) == 0, \
        'Missing non-Legato thread error'
