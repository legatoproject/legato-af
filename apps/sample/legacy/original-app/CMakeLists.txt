#
# Original program as a cross-compiled app
#

include(CMakeForceCompiler)

# Build the legacy app binary
add_executable(legacy-app-bin main.c)

# Build the legacy app, which in turn depends on the binary built above.
mkapp(legacy-app.adef)
add_dependencies(legacy-app legacy-app-bin)

# This is a C sample app
add_dependencies(samples_c legacy-app)
