#include "legato.h"
#include "pos_interface.h"
#include "pos_sample_interface.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Positioning Daemon Client Library
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
    le_pos_StartClient("posDaemon.le_pos");
    le_pos_sample_StartClient("posDaemon.le_pos_sample");

    LE_INFO("Positioning Daemon Client is ready.");
}

