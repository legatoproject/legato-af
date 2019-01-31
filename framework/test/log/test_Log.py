#
# Test harness for log testing
#

app_name = 'logTester'
logStrings = [ '[^|]* \| [^|]* \| frame 0 msg',
               '[^|]* \| [^|]* \| frame 1 msg',
               '[^|]* \| [^|]* \| frame 2 msg',
               '[^|]* \| [^|]* \| frame 3 msg',
               '[^|]* \| [^|]* \| frame 4 msg',
               '[^|]* \| [^|]* \| frame 5 msg' ]

def testLogFilter(target):
    for filterLevel in range(len(logStrings)):
        for message in range(filterLevel,len(logStrings)):
            assert message == \
                target.expect(logStrings), \
                "Expected log message to match %s but found %s" % \
                    (logStrings[message], target.match.group(1))
