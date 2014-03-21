/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef SMS_H_
#define SMS_H_
#include <stdint.h>
#include "lua.h"

#define SMS_API
#define FOR_LUA

typedef struct
{
  char*     address; // null terminated string
  char      timestamp[14];
  char*     message; // binary string of length message_length
  uint16_t  message_length;

  // Only used for SMS with port headers
  uint8_t   portbits;
  uint16_t  dstport;
  uint16_t  srcport;

  // Only used for concatenated sms
  uint16_t  concat_ref;
  uint8_t   concat_maxnb;
  uint8_t   concat_seqnb;
}SMS;

typedef struct
{
  uint16_t  size;
  char*     buffer;
}PDU;

int decode_smspdu(const char* pdu, SMS** sms);
void free_sms(SMS* sms);
int encode_smspdu(const char* message, int length, const char* address, PDU** pdu);
void free_pdu(void* pdu);
SMS_API int luaopen_smspdu(lua_State *L);

#endif /* SMS_H_ */
