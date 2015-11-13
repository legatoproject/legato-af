#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc.
#
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp( simToolkit.adef
       -i ${LEGATO_ROOT}/interfaces/modemServices
       DEPENDS
           legato
           ## TODO: Remove this when the mk tools do dependency checking.
           simToolkitComp/Component.cdef
           simToolkitComp/simToolkitTest.c
)
