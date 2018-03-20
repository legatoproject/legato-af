#!/usr/bin/env python

import os
import sys
from utils import *

KEEP_PREVIOUS_COMMAND = True

def main():
    print 'Content-Type: text/html'
    print ''
    if os.environ.get("REQUEST_METHOD") == 'POST':
        dct = decode_params(sys.stdin.read())
    else:
        dct={}

    print '''<html>
    <head>
    <title>Legato Run Shell Command</title>
    <link type="text/css" href="/stylesheet.css" rel="stylesheet"/>
    </head>
    <body>
    <a href="/"><img src="legato_logo.png"/></a>
    <h1>Run Shell Command</h1>
    <form method="post">'''
    print '<input autofocus type="text" name="command" size="64">'
    print '</form>'
    if 'command' in dct:
        print '<h3 id="cmd">' + dct['command'] + '</h3>'
        print '<pre>'
        live_shell(dct['command'])
        print '</pre>'
    else:
        print '<small> Tip: Try pressing &lt;up&gt; in the input field after executing a command.</small>'
    print '''
    <script>
    var elem = document.getElementsByName("command")[0];
    elem.onkeydown = function(e){
     e = e || window.event;
    if (e.keyCode == '38') {
    elem.value= "%s";
    }}</script>''' % dct.get('command','')  if KEEP_PREVIOUS_COMMAND else ''
    print '''</body>
    </html>  '''

if __name__ == '__main__':
    main()
