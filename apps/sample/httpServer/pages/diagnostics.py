#!/usr/bin/env python

import os
import sys

from utils import *

def main():
    print 'Content-Type: text/html'
    print ''

    print '''<html>
    <head>
    <title>Legato Diagnostics</title>
    <link type="text/css" href="/stylesheet.css" rel="stylesheet"/>
    </head>
    <body>
    <a href="/"><img src="legato_logo.png"/></a>
    <h1>Diagnostics</h1>  '''

    query_string=os.environ.get("QUERY_STRING", None)
    if query_string:
        print "query_string: "+query_string+"<br/>"

    print '<h2>Stats</h2>'
    print '<dl>'
    print '<dt>Date</dt><dd>' + shell('date') + '</dd>'
    print '<dt>Uptime</dt><dd>' + shell('uptime') + '</dd>'
    print '<dt>Legato Version</dt><dd>' + shell('/legato/systems/current/bin/legato version') + '</dd>'
    print '</dl>'
    print '<a href="/server-status">lighttpd status</a>'

    print '<h2>App status</h2>'
    print '<pre>'
    print shell('/legato/systems/current/bin/app status')
    print '</pre>'

    print '<h2>Network [ifconfig]</h2>'
    print '<pre>'
    print shell('ifconfig')
    print '</pre>'

    print '<h2>Running Processes [ps aux]</h2>'
    print '<pre>'
    print shell('ps aux')
    print '</pre>'

    print '''</body>
    </html>  '''

if __name__ == '__main__':
    main()
