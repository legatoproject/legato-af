#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test app.
mkapp(SubpoolFlux.adef)

mkapp(ThreadFlux.adef)

mkapp(TimerFlux.adef
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/framework/liblegato/linux/
      )

mkapp(MutexFlux.adef
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/framework/liblegato/linux/
      )

mkapp(SemaphoreFlux.adef
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/framework/liblegato/linux/
    )

# This is a C test
add_dependencies(tests_c SubpoolFlux TimerFlux MutexFlux SemaphoreFlux)
