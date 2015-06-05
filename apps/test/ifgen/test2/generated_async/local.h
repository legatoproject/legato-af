/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef LOCAL_H_INCLUDE_GUARD
#define LOCAL_H_INCLUDE_GUARD


#include "legato.h"

#define PROTOCOL_ID_STR "42eddea8de69ec9529f26c243ae01f6c5a7e8792cf6dc40ecfc7b120e0ef6328"

#define SERVICE_INSTANCE_NAME "example"


// todo: This will need to depend on the particular protocol, but the exact size is not easy to
//       calculate right now, so in the meantime, pick a reasonably large size.  Once interface
//       type support has been added, this will be replaced by a more appropriate size.
#define _MAX_MSG_SIZE 1100

// Define the message type for communicating between client and server
typedef struct
{
    uint32_t id;
    uint8_t buffer[_MAX_MSG_SIZE];
}
_Message_t;

#define _MSGID_AddTestAHandler 0
#define _MSGID_RemoveTestAHandler 1
#define _MSGID_allParameters 2
#define _MSGID_FileTest 3
#define _MSGID_TriggerTestA 4
#define _MSGID_AddBugTestHandler 5
#define _MSGID_RemoveBugTestHandler 6
#define _MSGID_TestCallback 7
#define _MSGID_TriggerCallbackTest 8


#endif // LOCAL_H_INCLUDE_GUARD

