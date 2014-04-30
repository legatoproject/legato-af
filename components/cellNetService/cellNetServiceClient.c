#include "legato.h"
#include "le_cellnet_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Cellular Network Service Client Library
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
    le_cellnet_StartClient("cellNetworkService");

    LE_INFO("Cellular Network Client is ready");
}

