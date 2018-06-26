#
# Test harness for log testing
#

app_name = 'logTester'
logStrings = [ '(DBUG \| [^|]* \| [^|]* \| frame 0 msg)',
               '(INFO \| [^|]* \| [^|]* \| frame 1 msg)',
               '(-WRN- \| [^|]* \| [^|]* \| frame 2 msg)',
               '(=ERR= \| [^|]* \| [^|]* \| frame 3 msg)',
               '(\*CRT\* \| [^|]* \| [^|]* \| frame 4 msg)',
               '(\*EMR\* \| [^|]* \| [^|]* \| frame 5 msg)' ]

def testLogFilter(target):
    for filterLevel in range(len(logStrings)):
        for message in range(filterLevel,len(logStrings)):
            assert message == \
                target.expect(logStrings), \
                "Expected log message to match %s but found %s" % \
                    (logStrings[message], target.match.group(1))
