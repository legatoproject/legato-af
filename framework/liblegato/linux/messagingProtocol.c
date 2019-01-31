/** @file messagingProtocol.c
 *
 * Implements the "Protocol" objects in the Legato @ref c_messaging.
 *
 * See @ref messaging.c for the overall subsystem design.
 *
 * @warning The code in this file @b must be thread safe and re-entrant.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "unixSocket.h"
#include "serviceDirectory/serviceDirectoryProtocol.h"
#include "messagingMessage.h"
#include "messagingProtocol.h"

// =======================================
//  PRIVATE DATA
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect data structures in this module from multi-threaded race conditions.
 *
 * @note This is a pthreads FAST mutex, chosen to minimize overhead.  It is non-recursive.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * List of Protocol objects.
 *
 * @note Nothing is ever deleted from this list, so the list only needs to be protected from thread
 * races when traversing to the next item while searching the list and when adding an item to the
 * list.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t ProtocolList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Protocol objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProtocolPoolRef;


// =======================================
//  PRIVATE FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Looks for a Protocol object in the Protocol List.
 *
 * @return A pointer to the Protocol object or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static msgProtocol_Protocol_t* FindProtocol
(
    const char* protocolId      ///< [in] String uniquely identifying the the protocol and version.
)
//--------------------------------------------------------------------------------------------------
{
    le_sls_Link_t* linkPtr;

    LOCK

    linkPtr = le_sls_Peek(&ProtocolList);
    while (linkPtr != NULL)
    {
        msgProtocol_Protocol_t* protocolPtr = CONTAINER_OF(linkPtr, msgProtocol_Protocol_t, link);
        if (strcmp(protocolId, protocolPtr->id) == 0)
        {
            UNLOCK

            return protocolPtr;
        }

        linkPtr = le_sls_PeekNext(&ProtocolList, linkPtr);
    }

    UNLOCK

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Protocol object.
 *
 * @return  A pointer to the object.  Never returns on failure.
 */
//--------------------------------------------------------------------------------------------------
static msgProtocol_Protocol_t* CreateProtocol
(
    const char* protocolId,     ///< [in] String uniquely identifying the the protocol and version.
    size_t largestMsgSize       ///< [in] Size (in bytes) of the largest message in the protocol.
)
//--------------------------------------------------------------------------------------------------
{
    msgProtocol_Protocol_t* protocolPtr = le_mem_ForceAlloc(ProtocolPoolRef);

    protocolPtr->link = LE_SLS_LINK_INIT;
    protocolPtr->maxPayloadSize = largestMsgSize;
    if (le_utf8_Copy(protocolPtr->id, protocolId, sizeof(protocolPtr->id), NULL) == LE_OVERFLOW)
    {
        LE_CRIT("Protocol identifier truncated from '%s' to '%s'.", protocolId, protocolPtr->id);
    }

    protocolPtr->messagePoolRef = msgMessage_CreatePool(protocolId, largestMsgSize);

    LOCK

    le_sls_Queue(&ProtocolList, &protocolPtr->link);

    UNLOCK

    return protocolPtr;
}


// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Initializes this module.  This must be called only once at start-up, before any other functions
 * in this module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgProto_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    ProtocolPoolRef = le_mem_CreatePool("Protocol", sizeof(msgProtocol_Protocol_t));
    le_mem_ExpandPool(ProtocolPoolRef, 5);  /// @todo Make this configurable.
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocate a Message object from a given Protocol's Message Pool.
 *
 * @return A pointer to the (uninitialized) Message object memory.
 */
//--------------------------------------------------------------------------------------------------
UnixMessage_t *msgProto_AllocMessage
(
    le_msg_ProtocolRef_t protocolRef
)
//--------------------------------------------------------------------------------------------------
{
    // Allocate a Message object from this Protocol's Message Pool.
    return le_mem_ForceAlloc(protocolRef->messagePoolRef);
}


// =======================================
//  PUBLIC API FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference that can be used to refer to a particular version of a particular protocol.
 *
 * @return  The protocol reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ProtocolRef_t le_msg_GetProtocolRef
(
    const char* protocolId,     ///< [in] String uniquely identifying the the protocol and version.
    size_t largestMsgSize       ///< [in] Size (in bytes) of the largest message in the protocol.
)
//--------------------------------------------------------------------------------------------------
{
    msgProtocol_Protocol_t* protocolPtr = FindProtocol(protocolId);
    if (protocolPtr == NULL)
    {
        protocolPtr = CreateProtocol(protocolId, largestMsgSize);
    }
    else if (protocolPtr->maxPayloadSize != largestMsgSize)
    {
        LE_FATAL("Wrong maximum message size (%zu) specified for protocol '%s' (expected %zu).",
                 largestMsgSize,
                 protocolId,
                 protocolPtr->maxPayloadSize);
    }

    return protocolPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the unique identifier string of the protocol.
 *
 * @return  A pointer to the protocol identifier (null-terminated, UTF-8 string).
 */
//--------------------------------------------------------------------------------------------------
const char* le_msg_GetProtocolIdStr
(
    le_msg_ProtocolRef_t protocolRef    ///< [in] Reference to the protocol.
)
//--------------------------------------------------------------------------------------------------
{
    return protocolRef->id;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the protocol's maximum message size.
 *
 * @return  The size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_msg_GetProtocolMaxMsgSize
(
    le_msg_ProtocolRef_t protocolRef    ///< [in] Reference to the protocol.
)
//--------------------------------------------------------------------------------------------------
{
    return protocolRef->maxPayloadSize;
}
