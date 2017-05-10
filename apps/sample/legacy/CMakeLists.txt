#
# Legacy app porting sample
#

# Basic app built without a cross-compiler
add_subdirectory(original)

# Cross-compiled program
add_subdirectory(original-cross)

# Cross-compiled program as an app
add_subdirectory(original-app)

# Cross-compiled program which uses a Legato service
add_subdirectory(useLegatoApi)

# Cross-compiled program which uses a callback from a Legato service
add_subdirectory(useLegatoHandlers)
