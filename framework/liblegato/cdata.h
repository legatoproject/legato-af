/**
 * @file cdata.h
 *
 * Legato @ref c_cdata internal include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LIBLEGATO_CDATA_INCLUDE_GUARD
#define LEGATO_LIBLEGATO_CDATA_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Map component key to a component instance number.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    short key;
    short instance;
} cdata_MapEntry_t;

typedef const cdata_MapEntry_t cdata_ThreadRec_t;

#endif /* LEGATO_LIBLEGATO_CDATA_INCLUDE_GUARD */
