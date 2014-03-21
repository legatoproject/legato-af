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

#include <stdlib.h>
#include <string.h>
#include "swi_update.h"
#include "swi_log.h"
#include "emp.h"
#include "yajl_gen.h"
#include "yajl_tree.h"
#include "yajl_helpers.h"

#define UNREGISTERUPDATELISTENER()                                                         \
do                                                                                         \
{                                                                                          \
  rc_ReturnCode_t res;                                                                     \
  res = emp_send_and_wait_response(EMP_UNREGISTERUPDATELISTENER, 0, NULL, 0, NULL, NULL);  \
  if (res != RC_OK)                                                                        \
  {                                                                                        \
    SWI_LOG("UPDATE", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res); \
    return RC_NOT_AVAILABLE;                                                               \
  }                                                                                        \
} while(0)

static rc_ReturnCode_t newStatusNotification(uint32_t payloadsize, char *payload);

static uint8_t module_initialized = 0;
static EmpCommand empCmds[] = { EMP_SOFTWAREUPDATESTATUS };
static emp_command_hdl_t empHldrs[] = { newStatusNotification };
static swi_update_StatusNotifictionCB_t user_cb;

static rc_ReturnCode_t newStatusNotification(uint32_t payloadsize, char *payload)
{
  yajl_val yval;
  swi_update_Notification_t notification;
  char *jsonPayload;

  if (user_cb == NULL)
    goto quit;

  jsonPayload = __builtin_strndup(payload, payloadsize);
  YAJL_TREE_PARSE(yval, jsonPayload);

  notification.event = (swi_update_Event_t)YAJL_GET_INTEGER(yval->u.array.values[0]);
  notification.eventDetails = yval->u.array.values[1]->u.string;
  user_cb(&notification);

  free(jsonPayload);
quit:
  emp_freemessage(payload);
  return RC_OK;
}

static void empReregisterServices()
{
  rc_ReturnCode_t res;

  res = emp_send_and_wait_response(EMP_REGISTERUPDATELISTENER, 0, NULL, 0, NULL, NULL);
  if (res != RC_OK)
    SWI_LOG("UPDATE", WARNING, "Failed to register back callback for status notifications\n");
}

rc_ReturnCode_t swi_update_Init()
{
  rc_ReturnCode_t res;

  if (module_initialized)
    return RC_OK;

  res = emp_parser_init(1, empCmds, empHldrs, empReregisterServices);
  if (res != RC_OK)
  {
    SWI_LOG("UPDATE", ERROR, "%s: Error while init emp lib, res=%d\n", __FUNCTION__, res);
    return res;
  }
  module_initialized = 1;
  return RC_OK;
}

rc_ReturnCode_t swi_update_Destroy()
{
  rc_ReturnCode_t res;

  if (!module_initialized)
    return RC_OK;
  if (user_cb)
  {
    UNREGISTERUPDATELISTENER();
    user_cb = NULL;
  }
  res = emp_parser_destroy(1, empCmds, empReregisterServices);
  if (res != RC_OK)
    SWI_LOG("UPDATE", ERROR, "error while destroy emp lib, res=%d\n", res);
  module_initialized = 0;
  return res;
}

rc_ReturnCode_t swi_update_RegisterStatusNotification(swi_update_StatusNotifictionCB_t cb)
{
  rc_ReturnCode_t res;

  if (cb == NULL)
  {
    UNREGISTERUPDATELISTENER();
    user_cb = cb;
    return RC_OK;
  }

  res = emp_send_and_wait_response(EMP_REGISTERUPDATELISTENER, 0, NULL, 0, NULL, NULL);
  if (res != RC_OK)
  {
    SWI_LOG("UPDATE", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    return res;
  }

  user_cb = cb;
  return RC_OK;
}

rc_ReturnCode_t swi_update_Request(swi_update_Request_t req)
{
  rc_ReturnCode_t res;
  char* payload, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  if (req < SWI_UPDATE_REQ_PAUSE || req > SWI_UPDATE_REQ_ABORT)
    return RC_BAD_PARAMETER;

  YAJL_GEN_ALLOC(gen);

  YAJL_GEN_INTEGER(req, "request code");

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_SOFTWAREUPDATEREQUEST, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("UPDATE", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    if (respPayload)
    {
      SWI_LOG("UPDATE", ERROR, "%s: got response = %.*s\n", __FUNCTION__, respPayloadLen, respPayload);
      free(respPayload);
    }
    return res;
  }
  free(respPayload);
  return RC_OK;
}
