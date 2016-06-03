#! /usr/bin/python
import os
import sys
from utils import *
import re
import time
def main():
    print 'Content-Type: text/html'
    print ''

    query_string=os.environ.get("QUERY_STRING", None)
    dct = decode_params(query_string)

    print '''<html>
    <head>
    <title>Legato App Installer</title>
    <link type="text/css" href="/stylesheet.css" rel="stylesheet"/>
    </head>
    <body>
    <a href="/"><img src="legato_logo.png"/></a>
    <h1>App Installer</h1>
    <form method="post" method="post" enctype="multipart/form-data">'''
    print '<input type="file" name="filename" ><p>'
    print '<input type="submit" value="Install">'
    print '</form>'
    print ''
    if os.environ.get("REQUEST_METHOD") == 'POST':
        if 'multipart/form-data' in os.environ.get("CONTENT_TYPE"):
            print '<p><pre>'
            handle_update()
            print '</pre>'
    print '''</body>
    </html>  '''
def handle_update():
    boundary = sys.stdin.readline().rstrip()
    content_disposition_line = sys.stdin.readline()
    filename = re.search('filename="(.*)"', content_disposition_line).group(1)
    if filename == '':
        print "There doesn't seem to be a file in that request."
        return
    content_type_line = sys.stdin.readline()
    sys.stdin.readline() # separator
    # begin downloading to file, stopping at "[boundary]--"
    templog = "/web_appinstaller_%s.log" % str(int(time.time()))
    with os.popen('/legato/systems/current/bin/update > ' + templog, 'w') as f:
        for line in sys.stdin:
            if line != boundary + '--\r\n':
                data = line
                f.write(data)
    with open(templog) as f:
        output = f.read()
    print(output)
    os.remove(templog)
if __name__ == '__main__':
    main()
