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

#define PROTOCOL_ID_STR "6eb229106644f1fcc141ad4230e46f51cf8e6677afecfa672d6b4b6617e4da43"


// todo: This will need to depend on the particular protocol, but the exact size is not easy to
//       calculate right now, so in the meantime, pick a reasonably large size.  Once interface
//       type support has been added, this will be replaced by a more appropriate size.
#define _MAX_MSG_SIZE 1000

// Define the message type for communicating between client and server
typedef struct
{
    uint32_t id;
    uint8_t buffer[_MAX_MSG_SIZE];
}
_Message_t;

#define _MSGID_AddTestA 0
#define _MSGID_RemoveTestA 1
#define _MSGID_allParameters 2
#define _MSGID_TriggerTestA 3
#define _MSGID_AddBugTest 4
#define _MSGID_RemoveBugTest 5


#endif // LOCAL_H_INCLUDE_GUARD

