//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) 2014 Sierra Wireless Inc., Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Data Connection Service Client Library
 *
 * The linker/loader will automatically run this function when the library is loaded into a
 * process's address space at runtime.
 *
 * Based on available documentation and testing, if multiple libraries have constructor
 * functions, then they will be executed in the order that the libraries are loaded. Thus,
 * the constructor for the legato framework library will always be executed before this
 * constructor, since this library is loaded after the legato framework library.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((constructor)) static void InitClient
(
    void
)
{
    le_data_ConnectService();

    LE_INFO("Data Connection Service Client is ready.");
}

