#
# Makefile for original program
#

add_executable(legacy main.c)

# This is a C sample app
add_dependencies(samples_c legacy)
