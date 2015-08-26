#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#--------------------------------------------------------------------------------------------------

find_package(Legato REQUIRED)

# Build the on-target test app.
mkapp(SubpoolFlux.adef
      DEPENDS
        SubpoolFlux/Component.cdef
        SubpoolFlux/SubpoolFlux.c
        )

mkapp(ThreadFlux.adef
      DEPENDS
        ThreadFlux/Component.cdef
        ThreadFlux/ThreadFlux.c
        )

mkapp(TimerFlux.adef
      -i ${LEGATO_ROOT}/framework/c/src/
      DEPENDS
        TimerFlux/Component.cdef
        TimerFlux/TimerFlux.c
        )

mkapp(MutexFlux.adef
      -i ${LEGATO_ROOT}/framework/c/src/
      DEPENDS
        MutexFlux/Component.cdef
        MutexFlux/MutexFlux.c
        )

mkapp(SemaphoreFlux.adef
      -i ${LEGATO_ROOT}/framework/c/src/
      DEPENDS
        SemaphoreFlux/Component.cdef
        SemaphoreFlux/SemaphoreFlux.c
        )
