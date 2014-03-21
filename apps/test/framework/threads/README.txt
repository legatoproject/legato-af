This app performs multi-threading tests.  These tests are intended to test multi-threading
functionality, like spawning, cancelling, and joining with threads of different priority levels,
and synchronizing threads with mutexes, and semaphores.

The tests are divided into different files, with a description of each test at the beginning of
that file.

The file init.c contains the Initialization Event Handler that kicks off all the tests.

Thoughts on Unit Test Framework:

    One of the things that CUnit offers is an "interactive test console", which
    allows the user to interactively run individual tests in a test suite.
        - Is this something that we need?
        - What is the main benefit of that feature?
        - Does it re-build the test, if a change was made, without having to start a
            new interactive session?


