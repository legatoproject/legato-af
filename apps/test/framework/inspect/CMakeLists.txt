#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test app.
mkapp(subpoolFlux.adef)

mkapp(threadFlux.adef)

mkapp(timerFlux.adef
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/framework/liblegato/linux/
      )

mkapp(mutexFlux.adef
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/framework/liblegato/linux/
      )

mkapp(semaphoreFlux.adef
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/framework/liblegato/linux/
    )

# This is a C test
add_dependencies(tests_c subpoolFlux threadFlux timerFlux mutexFlux semaphoreFlux)
