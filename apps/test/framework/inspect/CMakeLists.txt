#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test app.
mkapp(SubpoolFlux.adef)

mkapp(ThreadFlux.adef)

mkapp(TimerFlux.adef
      -i ${LEGATO_ROOT}/framework/c/src/
    )

mkapp(MutexFlux.adef
      -i ${LEGATO_ROOT}/framework/c/src/
    )

mkapp(SemaphoreFlux.adef
      -i ${LEGATO_ROOT}/framework/c/src/
    )
