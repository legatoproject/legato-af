#
# Common definitions and functions shared by different language packages
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#


import jinja2
import re
import logging


#---------------------------------------------------------------------------------------------------
# Direction for function parameters.  IN and OUT can be specified in the .api file, but INOUT
# is for internal auto-generated variables only.
#
# todo: Update checks for DIR_IN to not assume DIR_OUT in the else part
#---------------------------------------------------------------------------------------------------
DIR_IN = "IN"
DIR_OUT = "OUT"
DIR_INOUT = "INOUT"   # for internal use only



#---------------------------------------------------------------------------------------------------
# Template related definitions and functions
#---------------------------------------------------------------------------------------------------


# Cache used for pre-processed templates.  This provides a noticeable improvement in processing
# speed, especially with large files.  This cache is used and modified in FormatCode() below.
# TODO: Maybe this case should be inited in InitTemplateEnvironment() below.
TemplateCache = {}



def InitTemplateEnvironment():
    # This environment is shared by all templates, and is used in FormatCode() below, as well as
    # by the caller.  The caller uses it for registering filters.
    global Environment

    # Note that '$' is used as a line statement prefix, instead of '#', since some templates have
    # C macro definitions, which start with '#'.
    Environment = jinja2.Environment(trim_blocks=True, line_statement_prefix='$')

    return Environment



def FormatCode(templateString, **dataArgs):
    # Get the template from the cache, or create and cache it, if not in the cache.
    if templateString in TemplateCache:
        template = TemplateCache[templateString]
    else:
        template = Environment.from_string(templateString)
        TemplateCache[templateString] = template

    # Apply the template
    result = template.render(dataArgs)

    # Add back the trailing newline that gets stripped by render()
    result += '\n'

    # Remove trailing spaces
    result = re.sub(' +$', '', result, flags=re.M)

    return result



#---------------------------------------------------------------------------------------------------
# Add support for log tracing
#---------------------------------------------------------------------------------------------------

class TraceFilter(object):

    def __init__(self):
        self.traceList = []


    def filter(self, record):
        if hasattr(record, 'traceName'):
            if record.traceName not in self.traceList:
                return False

        return True


    def enableTrace(self, traceList):
        for traceName in traceList:
            if not traceName in self.traceList:
                self.traceList.append(traceName)



#
# Use a separate logger for tracing, and set up the tracing filter for it.
# This way tracing is independent of the logging level for the root logger,
# and a different log format can be used.
#
TraceFormatter = logging.Formatter("IFGEN_TRACE_%(traceName)s:%(module)s/%(funcName)s/%(lineno)s: %(message)s")
TraceHandler = logging.StreamHandler()
TraceHandler.setFormatter(TraceFormatter)
Tracer = TraceFilter()

TraceLogger = logging.getLogger('trace')
TraceLogger.setLevel(logging.DEBUG)
TraceLogger.addHandler(TraceHandler)
TraceLogger.addFilter(Tracer)

# All loggers are children of the root logger.  We don't want the root logger
# to see the trace log messages.
TraceLogger.propagate = False

# Enable traces for module level code, since this gets executed before the command line is parsed.
# Uncomment if needed.
#Tracer.enableTrace(['type'])

#
# Public tracing interface
#

def EnableTrace(traceList):
    Tracer.enableTrace(traceList)


class TraceObject(object):

    def __init__(self, traceName):
        self.traceName = traceName

    def log(self, msg):
        TraceLogger.debug(msg, extra=dict(traceName=self.traceName))

