#
# Run test applications which use le_test API on Legato.
#
# Goes through applications called  "test*.adef" and runs them, collecting TAP output.
# If all TAP tests pass, the item application test is considered passed.
#
# To run tests which do not produce TAP output, create a test*.py to run the test (as with pytest).
#
# Copyright (C) Sierra Wireless Inc.
#

import pytest, re, os, stat, sys, termios
import tap.parser
import pexpect, pexpect.fdpexpect, pexpect.pxssh
import time

def pytest_addoption(parser):
    """Add test configuration options."""
    group = parser.getgroup('legato')
    group.addoption(
        '--target',
        action='store',
        dest='target',
        default='',
        help='Target to run tests against.'
    )
    group.addoption(
        '--baudrate',
        action='store',
        dest='baudrate',
        default='115200',
        help='Debug UART baud rate.'
    )

class SerialPort:
    """Class providing serial port configuration and a pexpect wrapper."""

    # Available baud rates.
    BAUD = {
        9600: termios.B9600,
        19200: termios.B19200,
        38400: termios.B38400,
        57600: termios.B57600,
        115200: termios.B115200,
        230400: termios.B230400,
        460800: termios.B460800
    }

    # TTY attribute fields.
    IFLAG = 0
    OFLAG = 1
    CFLAG = 2
    LFLAG = 3
    ISPEED = 4
    OSPEED = 5
    CC = 6

    class ttyspawn(pexpect.fdpexpect.fdspawn):
        """Wrap fdspawn to hold reference to file object"""
        def __init__ (self, fd, **kwargs):
            # Hold a reference so file object is not garbage collected
            self.fd = fd
            super(SerialPort.ttyspawn, self).__init__(fd.fileno(), **kwargs)

        # Ubuntu 14.04 does not have __enter__ and __exit__ in pexpect, so define
        # them here.
        def __enter__(self):
            try:
                return super(SerialPort.ttyspawn, self).__enter__()
            except AttributeError, e:
                return self

        def __exit__(self, etype, evalue, tb):
            try:
                super(SerialPort.ttyspawn, self).__exit__(etype, evalue, tb)
            except AttributeError, e:
                pass

    @staticmethod
    def open(device, baudrate=None):
        """
        Open a serial port.

        @param  device      Device name.
        @param  baudrate    Initial baud rate to set.  Defaults to port's default.

        @return Port instance, or None if device not found.
        """
        port = None
        device_stat = None
        try:
            device_stat = os.stat(device)
        except OSError:
            pass

        if device_stat and stat.S_ISCHR(device_stat.st_mode):
            port = SerialPort(device, baudrate)
        return port

    def __init__(self, device, baudrate):
        """
        Instantiate serial port wrapper.

        @param  device      Device name.
        @param  baudrate    Initial baud rate to set.  Defaults to port's default if None.
        """
        self._device = device
        self._tty = open(self._device, 'r+')
        self._tty_attr = termios.tcgetattr(self._tty)
        self._tty_attr[self.IFLAG] = termios.IGNBRK
        self._tty_attr[self.OFLAG] = 0
        if baudrate is None:
            self._tty_attr[self.CFLAG] = termios.CS8 | (self._tty_attr[self.CFLAG] & termios.CBAUD)
        else:
            self._tty_attr[self.CFLAG] = termios.CS8 | self.BAUD[baudrate]
            self._tty_attr[self.ISPEED] = self._tty_attr[self.OSPEED] = self.BAUD[baudrate]
        termios.tcsetattr(self._tty, termios.TCSADRAIN, self._tty_attr)
        self._io = SerialPort.ttyspawn(self._tty, logfile=sys.stderr)
        self._io.port = self

        if baudrate is None:
            baud_value = self._tty_attr[self.CFLAG] & termios.CBAUD
            for b, v in self.BAUD.iteritems():
                if v == baud_value:
                    baudrate = b
                    break
        self._baudrate = baudrate
        assert self._baudrate in self.BAUD

    def __del__(self):
        """Close port."""
        self._io.close()
        self._tty.close()

    def flush(self):
        """Flush serial port input and output buffers."""
        termios.tcflush(self._tty, termios.TCIOFLUSH)

    @property
    def baudrate(self):
        """
        Get current baud rate.

        @return Current baud rate.
        """
        return self._baudrate

    @baudrate.setter
    def set_baudrate(self, baudrate):
        """
        Change the baud rate.

        @param  baudrate    New baud rate to set.
        """
        self._tty_attr[self.CFLAG] = termios.CS8 | self.BAUD[baudrate]
        self._tty_attr[self.ISPEED] = self._tty_attr[self.OSPEED] = self.BAUD[baudrate]
        termios.tcsetattr(self._tty, termios.TCSADRAIN, self._tty_attr)
        self._baudrate = baudrate

    @property
    def io(self):
        """
        Get pexpect wrapper.

        @return pexpect wrapper of port.
        """
        return self._io

    def __repr__(self):
        """
        Get printable representation of port.

        @return Port string representation.
        """
        return "SerialPort<{0} @ {1} | {2}:{3}>".format(self._device, self.baudrate, self._tty,
            self._io)

def connect_target(app_name, target_name, baudrate=115200):
    """
    Connect to Legato test target via serial or SSH.

    @param  app_name    Name of test application to start on target.
    @param  target_name Serial port name or SSH host to connect to.
    @param  baudrate    Initial baud rate to connect at for a serial target.

    @return A pexpect wrapper around the connection that can be used to run tests.
    """
    port = SerialPort.open(target_name)
    if port:
        app = port.io
        gotConsole=False
        app.send("\r")
        for i in range(5):
            prompt = app.expect_exact(["\n>", "\n#", pexpect.TIMEOUT], timeout=1)
            if prompt == 0:
                # Reset to clear out any threads, etc. left from a previous test.
                app.send("reset\r")
                # Resetting will set baud rate back to 115200
                app.expect_exact("Performing reboot...\r\n")
                port.baudrate = 115200
                app.expect(["Hit '\\\\r' or '\\\\e' key to stop autoboot:", '[0-9]'])
                app.send("\r")
                gotConsole=1
                break
            elif prompt == 1:
                gotConsole=1
                break
            else:
                if i > 2:
                    # After a couple of tries, switch to other baud rate
                    if port.baudrate == 115200:
                        port.baudrate = baudrate
                    else:
                        port.baudrate = 115200
                app.send("\r")
        assert gotConsole
        # Switch baud rates (if neede)
        app.send("env set baudrate {}\r\r".format(baudrate))
        result = app.expect(
            ["## Switch baudrate [^\r\n]* ENTER \\.\\.\\.",
             "# \r\n# "])
        if result == 0:
            port.baudrate = baudrate
            port.flush()
            app.send("\r")
        app.send("boot\r")
        time.sleep(1)
        app.send("\r")
        for n in range(5):
            result = app.expect_exact(["\n>", pexpect.TIMEOUT], timeout=15)
            if result == 0:
                break
            app.send("\r")
        app.send("app status\r")
        app_status=app.expect_exact(["[stopped] " + app_name + "\r\n",
                                     "[running] " + app_name + "\r\n",
                                     pexpect.TIMEOUT], timeout=2)
        if app_status == 0:
            app.expect_exact(">")
            app.send("app start " + app_name + "\r")
            return app
        elif app_status == 1:
            pytest.fail(app_name + " is already running")
        elif app_status == 2:
            pytest.skip()
    else:
        # Assume IP or hostname for ssh
        app = pexpect.pxssh.pxssh(logfile=sys.stderr)
        app.login(target_name, "root")
        app.send("app status\r")
        app_status=app.expect_exact(["[stopped] " + app_name + "\r\n",
                                     "[running] " + app_name + "\r\n",
                                     pexpect.TIMEOUT], timeout=2)
        if app_status == 0:
            app.send("logread -f & app start " + app_name + "\n")
            return app
        elif app_status == 1:
            pytest.fail(app_name + " is already running")
        elif app_status == 2:
            pytest.skip()

def reset_target(target):
    """
    Reset a target's state.

    This is only relevant on RTOS, where everything is part of one executable and memory state is
    preserved between app executations.

    @param  target  Pexpect wrapper around target connection.
    """
    if isinstance(target, SerialPort.ttyspawn):
        target.port.flush()
        target.send("\r")
        target.expect_exact([">", pexpect.TIMEOUT], timeout=5)
        target.send('reset\r')
        target.port.baudrate = 115200

def pytest_collect_file(path, parent):
    """ Search for test files."""
    # Only search if in a "test" directory.
    def is_test_path(path):
        if path:
            dirpath, basename = os.path.split(path)
            if basename == "test":
                return True
            else:
                return is_test_path(dirpath)
        else:
            return False
    if not is_test_path(path.relto(parent.fspath)):
        return None
    if path.ext == ".adef" and path.basename.startswith("test_"):
            return LegatoFile(path, parent)

class LegatoFile(pytest.File):
    """Class to generate test cases from adef files."""
    def collect(self):
        """Collect Legato test application.  Each test application yields a single test item."""
        yield LegatoTapItem(self.fspath.basename[:-5], self)

class LegatoTestStep:
    """Class wrapping a single TAP test line."""

    def __init__(self, tap_parser, process, expected_number):
        """
        Instantiate a test step.

        @param  tap_parser      TAP parser instance for the test run.
        @param  process         Pexpect-wrapped connection to running test process.
        @param  expected_number Number of the next expected TAP line.
        """
        self._parser = tap_parser
        self._process = process
        self._timeout = False
        self._expected_number = expected_number
        self._failure = None
        self._total_tests = -1
        self._bail = False
        self._run = False
        self._found_number = -1
        self._text = ""

    def run(self):
        """Evaluate the test line and pass or fail based on the output of the test process."""
        self._run = True
        choice = self._process.expect([' \| TAP \| ([^\r\n]*)\r\n', pexpect.TIMEOUT])
        if choice == 1:
            self._timeout = True
            self.fail("Test timed out.")
            return ""

        self._text = self._process.match.group(1)
        tap_parsed_line = self._parser.parse_line(self._text)

        if tap_parsed_line.category == "plan":
            self._total_tests = tap_parsed_line.expected_tests

        elif tap_parsed_line.category == "test":
            self._found_number = tap_parsed_line.number

            if self._found_number < self._expected_number:
                self.fail("ERROR: Duplicate test {0}".format(self._found_number))
            elif self._found_number > self._expected_number:
                self.fail("not ok {0} - missing test".format(self._expected_number))

            if not (tap_parsed_line.ok or tap_parsed_line.skip or tap_parsed_line.todo):
                self.fail(self._text)

        elif tap_parsed_line.category == "bail":
            self.fail(self._text)
            self._bail = True

        return self._text + "\n"

    def fail(self, failure):
        """
        Force the test step to a failed state.

        @param  failure Failure text.
        """
        self._failure = failure

    def __repr__(self):
        """
        Get test step textual representation.

        @return String prepresentation of test step.
        """
        state = "run" if self._run else "not run"
        if self.failed:
            state += ", failed"
        if self.timeout:
            state += ", timed out"
        if self.bail:
            state += ", bail out"
        text = " - " if self._text or self._failure else ""
        if self._text:
            text += self._text + "; "
        if self._failure:
            text += self._failure
        return "LegatoTestStep<{0} ({1}): {2}>{3}".format(
            self._found_number, self._expected_number, state, text)

    @property
    def failed(self):
        """
        Query failure state of the step.

        @return True if step has failed.
        """
        return self._failure is not None

    @property
    def failure(self):
        """
        Query failure text.

        @return Failure text or None if test has not failed.
        """
        return self._failure

    @property
    def timeout(self):
        """
        Query timeout state of the step.

        @return True if step has failed due to timeout.
        """
        return self._timeout

    @property
    def next(self):
        """
        Get the number of the next expected TAP line.

        @return Expected test line number of the next line.
        """
        if self._found_number > 0:
            return self._found_number + 1
        else:
            return self._expected_number

    @property
    def number(self):
        """
        Get the number of this TAP line.

        @return TAP line number, if discovered, otherwise -1.
        """
        return self._found_number if self._found_number > 0 else -1

    @property
    def total(self):
        """
        Get the total number of tests, if available.

        @return Total number of TAP tests, if the current line provided it, otherwise -1.
        """
        return self._total_tests

    @property
    def bail(self):
        """
        Determine if the test run should stop.

        @return True if the test has failed in a manner that requires ignoring further steps.
        """
        return self._bail

    @property
    def text(self):
        """
        Get the parsed test line.

        @return Text of the test line, without logging preamble.
        """
        return self._text

def test_loop(process, step_handlers=None, catch_legato=True):
    """
    Execute a TAP test loop on the given test process.

    @param  process         Wrapped test application.
    @param  step_handlers   A dictionary of extra properties to apply to specific TAP test numbers.
                            For each entry, the key is the integer test number, and the value is a
                            dictionary containing the following fields, both of which are optional:
                                'expected': An RE to be matched against the line, with the test
                                            failing if the match fails.
                                'poststep': A function to execute after the step is run.  The
                                            function takes the step instance as its only argument.
    @param  catch_legato    Whether to catch and handle Legato test exceptions within the loop or
                            allow them to propagate up out of the function.

    @return Collected TAP output from the test run.
    """
    tap_parser = tap.parser.Parser()
    total_tests = -1
    failed_tests = []
    tap_output = ""
    next_test = 1
    bail_out = False

    try:
        while True:
            if total_tests >= 0 and next_test > total_tests:
                break

            expected = None
            poststep = None
            if step_handlers and next_test in step_handlers:
                expected = step_handlers[next_test].get('expected')
                poststep = step_handlers[next_test].get('poststep')

            step = LegatoTestStep(tap_parser, process, next_test)
            tap_output += step.run()
            if step.number == next_test:
                if expected is not None and not re.search(expected, step.text):
                    step.fail("Did not find expected text '{0}' at step {1}".format(
                        expected, step.number))
                if poststep:
                    poststep(step)
            next_test = step.next

            if step.failed:
                failed_tests.append(step.failure)
                if step.bail or step.timeout:
                    bail_out = step.bail
                    break
            elif step.total > 0:
                total_tests = step.total

        if not bail_out:
            while next_test <= total_tests:
                failed_tests.append("not ok {0} - missing test".format(next_test))
                next_test += 1

        if len(failed_tests) != 0:
            raise LegatoException(failed_tests)
        if total_tests == -1:
            raise LegatoException(["No test output"])
        return tap_output
    except Exception as e:
        if catch_legato:
            if isinstance(e, LegatoException):
                pytest.fail(repr(e))
                return
        raise

class LegatoTapItem(pytest.Item):
    """Default wrapper to execute a TAP test application identified by its adef file."""

    def __init__(self, appName, parent):
        """Instantiate the test."""
        super(LegatoTapItem, self).__init__(name=appName, parent=parent)

    def runtest(self):
        """Connect to the target and execute the test loop."""
        target_name = self.config.getoption('target')
        baudrate = int(self.config.getoption('baudrate'))
        if target_name == '':
            raise Exception('Target not set')
        with connect_target(self.name, target_name, baudrate) as test_proc:
            tap_output = test_loop(test_proc, catch_legato=False)

            try:
                self.add_report_section("call", "tap", tap_output)
            except AttributeError:
                print tap_output
            return
        raise LegatoException(["Test execution failed"])

    def repr_failure(self, excinfo):
        """
        Handle and exception raised by runtest().

        @param  excinfo Exception wrapper.

        @return String describing the exception.
        """
        if isinstance(excinfo.value, LegatoException):
            return repr(excinfo.value)
        else:
            return super(LegatoTapItem, self).repr_failure(excinfo)

    def reportinfo(self):
        """Generate test report."""
        return self.fspath, None, self.name

class LegatoException(Exception):
    """Legato test failure exception type."""

    def __init__(self, failedTests):
        """
        Create exception.

        @param  failedTests List of test failure messages.
        """
        self.failedTests = failedTests

    def __repr__(self):
        """
        Get failure text.

        @return Text of test failures that caused this exception.
        """
        text = "Tests Failed:\n"
        text += "    "
        text += "\n    ".join(self.failedTests)
        return text

@pytest.fixture
def target(request):
    """
    Fixture to obtain the test application process wrapper for customized tests.  This provides the
    primary interface for communicating with a test application's stdin/stdout/stderr.
    """
    target_name = request.config.getoption('target')
    baudrate = int(request.config.getoption('baudrate'))
    if target_name == '':
        request.raiseerror("Target not set")
    app = connect_target(request.module.app_name, target_name, baudrate)
    def teardown():
        reset_target(app)
    request.addfinalizer(teardown)
    return app

@pytest.fixture
def testloop(request):
    """Fixture to obtain the test loop function for customized tests."""
    return test_loop

@pytest.fixture
def serialport(request):
    """
    Fixture to obtain the serial port class for customized tests.
    This is used for interacting with a separate serial port, for example to send AT commands,
    though any context-relevant serial data can be sent or received.  This can be used for
    communications over an appropriate serial connection even if the primary target interface is
    over SSH.
    """
    return SerialPort
