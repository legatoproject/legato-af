/** @file atMachineUnsolicited.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINEUNSOLICITED_INCLUDE_GUARD
#define LEGATO_ATMACHINEUNSOLICITED_INCLUDE_GUARD

#include "legato.h"

#define ATMANAGER_UNSOLICITED_SIZE  256

//--------------------------------------------------------------------------------------------------
/**
 * typedef of atUnsolicited_t
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t unsolicitedReportId;                 ///< Event Id to report to
    char          unsolRsp[ATMANAGER_UNSOLICITED_SIZE];///< pattern to match
    bool          withExtraData;                       ///< Indicate if the unsolicited has more than one line
    bool          waitForExtraData;                    ///< Indicate if this is the extra data to send
    le_dls_Link_t link;                                ///< Used to link in Unsolicited List
}atUnsolicited_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atunsolicited
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineunsolicited_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an atUnsolicited_t struct
 *
 * @return pointer on the structure
 */
//--------------------------------------------------------------------------------------------------
atUnsolicited_t* atmachineunsolicited_Create
(
    void
);

#endif /* LEGATO_ATMACHINEUNSOLICITED_INCLUDE_GUARD */
