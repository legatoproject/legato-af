#!/usr/bin/env python2.7
"""
Removes implementations from C headers by removing everything between curly braces.
(Unless the last character wasn't ")", in which case it's a typedef and should be kept)
"""

import sys

stack = 0 # how many blocks deep we are

last_non_space = ''
inside_implementation = False
skip_one = False

for lineno, line in enumerate(sys.stdin):
    for c in line:
        if c == '{': # start block
            if stack == 0:
                if last_non_space == ')':
                    inside_implementation = True
                    stack += 1
                else:
                    pass # inside typedef
            else:
                stack += 1
        elif c == '}': # end block
            if inside_implementation:
                stack -= 1
                if stack == 0: # if it was an outer block
                    sys.stdout.write(';')
                    skip_one = True
                    inside_implementation = False
            if stack < 0:
                sys.stderr.write("Unmatched close brace on line %s." % lineno)
        if not inside_implementation and not skip_one:
            sys.stdout.write(c)
        skip_one = False
        if not c.isspace():
            last_non_space = c
sys.stdout.flush()
