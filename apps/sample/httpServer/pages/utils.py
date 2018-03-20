import os
import sys
import re
def shell(command):
    return escape(os.popen(command + " 2>&1").read())

def live_shell(command):
    p = os.popen(command + " 2>&1")
    while True:
        line = p.readline()
        if not line:
            break
        sys.stdout.write(escape(line))
        sys.stdout.flush()

def escape(s):
    s = s.replace("&", "&amp;" )
    s = s.replace("<", "&lt;" )
    s = s.replace(">", "&gt;" )
    s = s.replace('"', "&quot;")
    return s

def decode_params(querystr):
    if querystr is None:
        return {}

    queries = querystr.split('&')
    params = {}
    for x in [z.split("=") for z in queries]:
        if len(x) == 2:
            params[urldecode(x[0])] = urldecode(x[1])

    return params

def urldecode(s):
    return unquote(s.replace('+',' '))

#http://stackoverflow.com/a/15627281/765210
def unquote(url):
    return re.compile('%([0-9a-fA-F]{2})',re.M).sub(lambda m: chr(int(m.group(1),16)), url)
