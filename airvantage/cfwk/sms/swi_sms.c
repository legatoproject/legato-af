/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#define _GNU_SOURCE // this is required for strndup, which might cause warnings depending on the used toolchains
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "swi_sms.h"
#include "swi_log.h"
#include "emp.h"
#include "yajl_gen.h"
#include "yajl_tree.h"
#include "yajl_helpers.h"

typedef struct cb_list
{
  int regId;
  swi_sms_ReceptionCB_t cb;
  char *payload;
  struct cb_list *next;
} cb_list_t;

static rc_ReturnCode_t newSmsHdlr(uint32_t payloadsize, char* payload);
static cb_list_t *cb_list;
static EmpCommand empCmds[] = { EMP_NEWSMS };
static emp_command_hdl_t empHldrs[] = { newSmsHdlr };
static uint8_t module_initialized = 0;
static pthread_mutex_t handler_lock = PTHREAD_MUTEX_INITIALIZER;

static rc_ReturnCode_t newSmsHdlr(uint32_t payloadsize, char* payload)
{
  int regId;
  rc_ReturnCode_t res = RC_OK;
  cb_list_t *entry;
  char *sender, *message, *jsonPayload;
  yajl_val yval;

  jsonPayload = malloc(payloadsize + 1);
  memcpy(jsonPayload, payload, payloadsize);
  jsonPayload[payloadsize] = '\0';

  YAJL_TREE_PARSE(yval, jsonPayload);
  if (yval->u.array.values[0]->type != yajl_t_string)
  {
    SWI_LOG("SMS", ERROR, "%s: sender is not an string, got type=%d\n", __FUNCTION__, yval->u.array.values[0]->type);
    res = RC_BAD_PARAMETER;
    goto quit;
  }
  sender = YAJL_GET_STRING(yval->u.array.values[0]);

  if (yval->u.array.values[1]->type != yajl_t_string)
  {
    SWI_LOG("SMS", ERROR, "%s: message is not an string, got type=%d\n", __FUNCTION__, yval->u.array.values[1]->type);
    res = RC_BAD_PARAMETER;
    goto quit;
  }
  message = YAJL_GET_STRING(yval->u.array.values[1]);

  if (yval->u.array.values[2]->type != yajl_t_number)
  {
    SWI_LOG("SMS", ERROR, "%s: regId is not an number, got type=%d\n", __FUNCTION__, yval->u.array.values[2]->type);
    res = RC_BAD_PARAMETER;
    goto quit;
  }
  regId = YAJL_GET_INTEGER(yval->u.array.values[2]);

  pthread_mutex_lock(&handler_lock);
  for (entry = cb_list; entry; entry = entry->next)
  {
    if (entry->regId == regId)
    {
      entry->cb(sender, message);
      break;
    }
  }
  pthread_mutex_unlock(&handler_lock);
quit:
  free(jsonPayload);
  emp_freemessage(payload);
  yajl_tree_free(yval);

  return res;
}

static rc_ReturnCode_t send_reg_payload(const char *payload, size_t payloadLen, int *regId)
{
  rc_ReturnCode_t res;
  char *rpayload = NULL, *respPayload = NULL;
  uint32_t respPayloadLen;
  yajl_val yval = NULL;

  res = emp_send_and_wait_response(EMP_REGISTERSMSLISTENER, 0, payload, payloadLen, &respPayload, &respPayloadLen);

  if (res != RC_OK)
    goto quit;

  rpayload = strndup(respPayload, respPayloadLen);
  if (rpayload == NULL)
  {
    res = RC_NO_MEMORY;
    goto quit;
  }

  YAJL_TREE_PARSE(yval, rpayload);
  if (yval->type != yajl_t_number)
  {
    SWI_LOG("SMS", ERROR, "Invalid regId type received from RA, got type=%u, expected %u\n", __FUNCTION__, yval->type, yajl_t_number);
    res = RC_BAD_PARAMETER;
    goto quit;
  }
  *regId = YAJL_GET_INTEGER(yval);
quit:
  yajl_tree_free(yval);
  free(respPayload);
  free(rpayload);
  return res;
}

static void empReregisterServices()
{
  cb_list_t *entry;
  rc_ReturnCode_t res;

  for (entry = cb_list; entry; entry = entry->next)
  {
    res = send_reg_payload(entry->payload, strlen(entry->payload), &entry->regId);
    if (res != RC_OK)
      SWI_LOG("SMS", WARNING, "Failed to register back callback %p with regId %p\n", entry->cb, entry);
  }
}

rc_ReturnCode_t swi_sms_Init()
{
  rc_ReturnCode_t res;

  if (module_initialized)
    return RC_OK;

  res = emp_parser_init(1, empCmds, empHldrs, empReregisterServices); // No async EMP cmd is managed by this lib
  if (res != RC_OK)
  {
    SWI_LOG("SMS", ERROR, "%s: Error while init emp lib, res=%d\n", __FUNCTION__, res);
    return res;
  }
  module_initialized = 1;
  return RC_OK;
}

rc_ReturnCode_t swi_sms_Destroy()
{
  rc_ReturnCode_t res;

  if (!module_initialized)
    return RC_OK;
  res = emp_parser_destroy(1, empCmds, empReregisterServices);
  if (res != RC_OK)
    SWI_LOG("SMS", ERROR, "error while destroy emp lib, res=%d\n", res);
  module_initialized = 0;
  return res;
}

rc_ReturnCode_t swi_sms_Send(const char *recipientPtr, const char* messagePtr, swi_sms_Format_t format)
{
  rc_ReturnCode_t res;
  char *payload, *respPayload;
  const char *smsFormat = "";
  size_t payloadLen;
  uint32_t respPayloadLen;
  yajl_gen gen;

  if (recipientPtr == NULL || messagePtr == NULL || format < SWI_SMS_7BITS || format > SWI_SMS_UCS2)
    return RC_BAD_PARAMETER;

  switch(format)
  {
    case SWI_SMS_7BITS:
      smsFormat = "7bits";
      break;
    case SWI_SMS_8BITS:
      smsFormat = "8bits";
      break;
    case SWI_SMS_UCS2:
      smsFormat = "ucs2";
      break;
    default:
      break;
  }

  YAJL_GEN_ALLOC(gen);

  yajl_gen_array_open(gen);

  YAJL_GEN_STRING(recipientPtr, "recipientPtr");
  YAJL_GEN_STRING(messagePtr, "messagePtr");
  YAJL_GEN_STRING(smsFormat, "smsFormat");

  yajl_gen_array_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_SENDSMS, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {
    SWI_LOG("SMS", ERROR, "error while sending sms, res=%d\n", res);
    SWI_LOG("SMS", ERROR, "got error msg=%s\n", respPayload);
  }
  free(respPayload);
  return res;
}

rc_ReturnCode_t swi_sms_Register(swi_sms_ReceptionCB_t callback, const char* senderPatternPtr,
    const char* messagePatternPtr, swi_sms_regId_t *regIdPtr)
{
  rc_ReturnCode_t res;
  char *payload;
  size_t payloadLen;
  const char *senderP, *messageP;
  cb_list_t *entry;
  yajl_gen gen;
  int regId;

  if (callback == NULL || regIdPtr == NULL)
    return RC_BAD_PARAMETER;

  senderP = senderPatternPtr ? senderPatternPtr : "";
  messageP = messagePatternPtr ? messagePatternPtr : "";

  YAJL_GEN_ALLOC(gen);

  yajl_gen_array_open(gen);

  YAJL_GEN_STRING(senderP, "sender pattern");
  YAJL_GEN_STRING(messageP, "message pattern");

  yajl_gen_array_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = send_reg_payload(payload, payloadLen, &regId);

  if (res != RC_OK)
    goto quit;

  pthread_mutex_lock(&handler_lock);
  if (!cb_list)
  {
    cb_list = malloc(sizeof(cb_list_t));
    entry = cb_list;
  }
  else
  {
    for (entry = cb_list; entry->next; entry = entry->next)
      ;
    entry->next = malloc(sizeof(cb_list_t));
    entry = entry->next;
  }
  entry->regId = regId;
  entry->cb = callback;
  entry->payload = strndup(payload, payloadLen);
  entry->next = NULL;
  *regIdPtr = entry;
  pthread_mutex_unlock(&handler_lock);
quit:
  yajl_gen_clear(gen);
  yajl_gen_free(gen);
  return res;
}

rc_ReturnCode_t swi_sms_Unregister(swi_sms_regId_t regId)
{
  rc_ReturnCode_t res;
  char *payload, *respPayload;
  size_t payloadLen;
  uint32_t respPayloadLen;
  cb_list_t *entry, *tmp;
  yajl_gen gen;

  for (entry = cb_list; entry; entry = entry->next)
    if (entry == regId)
      break;
  if (entry == NULL)
    return RC_BAD_PARAMETER;

  YAJL_GEN_ALLOC(gen);

  YAJL_GEN_INTEGER(entry->regId, "regId");

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_UNREGISTERSMSLISTENER, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {
    SWI_LOG("SMS", ERROR, "Error while unregister sms, res=%d\n", res);
    SWI_LOG("SMS", ERROR, "got error msg=%s\n", respPayload);
    goto quit;
  }

  pthread_mutex_lock(&handler_lock);
  if (cb_list == regId)
  {
    free(cb_list);
    cb_list = NULL;
  }
  else
  {
    for (entry = cb_list; entry; entry = entry->next)
    {
      if (entry->next == regId)
      {
    break;
      }
    }
    tmp = entry->next;
    entry->next = tmp->next;
    free(tmp->payload);
    free(tmp);
  }
  pthread_mutex_unlock(&handler_lock);
quit:
  free(respPayload);
  return res;
}
