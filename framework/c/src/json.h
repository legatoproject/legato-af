//--------------------------------------------------------------------------------------------------
/**
 * @file json.h
 *
 * Interfaces exported by the JSON parser to other modules inside the Legato framework
 * implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef JSON_H_INCLUDE_GUARD
#define JSON_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the JSON parser module.
 *
 * Must be called exactly once at start-up before any other JSON module functions are called.
 */
//--------------------------------------------------------------------------------------------------
void json_Init
(
    void
);

#endif // JSON_H_INCLUDE_GUARD
