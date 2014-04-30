/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#define _GNU_SOURCE // this is required for strndup, which might cause warnings depending on the used toolchains
#include <string.h>
#include <arpa/inet.h>
#include "swi_devicetree.h"
#include "swi_log.h"
#include "dset_internal.h"
#include "emp.h"
#include "yajl_gen.h"
#include "yajl_tree.h"
#include "yajl_helpers.h"

#define DT_TYPE_INTEGER 0
#define DT_TYPE_DOUBLE 1
#define DT_TYPE_STRING 2
#define DT_TYPE_NULL 3
#define DT_TYPE_BOOL 4

typedef struct cb_list
{
  char *regId;
  swi_dt_NotifyCB_t cb;
  char *payload;
  struct cb_list *next;
} cb_list_t;

static rc_ReturnCode_t empNotifyVariables(uint32_t payloadsize, char *payload);

static cb_list_t *cb_list;
static EmpCommand empCmds[] = { EMP_NOTIFYVARIABLES };
static emp_command_hdl_t empHldrs[] = { empNotifyVariables };
static char initialized = 0;
static pthread_mutex_t cb_lock = PTHREAD_MUTEX_INITIALIZER;

static rc_ReturnCode_t empNotifyVariables(uint32_t payloadsize, char *payload)
{
  swi_dset_Iterator_t* data;
  rc_ReturnCode_t res;
  const char *regId, *varName;
  char *jsonPayload;
  cb_list_t *entry;
  yajl_val yval;
  int i;

  SWI_LOG("DT", DEBUG, "%s: begin\n", __FUNCTION__);

  res = swi_dset_Create(&data);
  if (res != RC_OK)
  {
    SWI_LOG("DT", ERROR, "%s: dset allocation failed, res %d\n", __FUNCTION__, res);
    emp_freemessage(payload);
    return RC_NO_MEMORY;
  }

  jsonPayload = strndup(payload, payloadsize);
  YAJL_TREE_PARSE(yval, jsonPayload);
  if (yval->type != yajl_t_array)
  {
    SWI_LOG("DT", ERROR, "%s: Invalid payload received from RA, expected array got type=%u\n", __FUNCTION__, yval->type);
    res = RC_BAD_FORMAT;
    goto quit;
  }

  SWI_LOG("DT", DEBUG, "%s: payload=%.*s, payloadsize=%u\n", __FUNCTION__, payloadsize, payload, payloadsize);

  regId = yval->u.array.values[0]->u.string;

  for (i = 0; i < yval->u.array.values[1]->u.object.len; i++)
  {
    varName = yval->u.array.values[1]->u.object.keys[i];

    switch(yval->u.array.values[1]->u.object.values[i]->type)
    {
      case yajl_t_string:
        res = swi_dset_PushString(data, varName, strlen(varName), yval->u.array.values[1]->u.object.values[i]->u.string,
                strlen(yval->u.array.values[1]->u.object.values[i]->u.string));
    SWI_LOG("DT", DEBUG, "%s: pushing string value %s -> %s\n", __FUNCTION__, varName, yval->u.array.values[1]->u.object.values[i]->u.string);
        break;
      case yajl_t_number:
        if (yval->u.array.values[1]->u.object.values[i]->u.number.flags & YAJL_NUMBER_INT_VALID)
    {
      res = swi_dset_PushInteger(data, varName, strlen(varName), yval->u.array.values[1]->u.object.values[i]->u.number.i);
      SWI_LOG("DT", DEBUG, "%s: pushing int value %s -> %lld\n", __FUNCTION__, varName, yval->u.array.values[1]->u.object.values[i]->u.number.i);
    }
        else if (yval->u.array.values[1]->u.object.values[i]->u.number.flags & YAJL_NUMBER_DOUBLE_VALID)
    {
      res = swi_dset_PushFloat(data, varName, strlen(varName), yval->u.array.values[1]->u.object.values[i]->u.number.d);
      SWI_LOG("DT", DEBUG, "%s: pushing double value %s -> %lf\n", __FUNCTION__, varName, yval->u.array.values[1]->u.object.values[i]->u.number.d);
    }
        break;
      case yajl_t_true:
    res = swi_dset_PushBool(data, varName, strlen(varName), true);
    break;
      case yajl_t_false:
    res = swi_dset_PushBool(data, varName, strlen(varName), false);
    break;
      case yajl_t_null:
    res = swi_dset_PushNull(data, varName, strlen(varName));
    break;
      default:
        break;
    }

    if (res != RC_OK)
    {
      SWI_LOG("DT", ERROR, "%s: Failed to push value to dset, res = %d\n", __FUNCTION__, res);
      goto quit;
    }
  }

  pthread_mutex_lock(&cb_lock);
  for (entry = cb_list; entry; entry = entry->next)
    if (entry->cb && !strcmp(entry->regId, regId))
      entry->cb(data);
  pthread_mutex_unlock(&cb_lock);

quit:
  SWI_LOG("DT", DEBUG, "%s: end\n", __FUNCTION__);
  swi_dset_Destroy(data);
  free(jsonPayload);
  emp_freemessage(payload);
  yajl_tree_free(yval);
  return res;
}

static rc_ReturnCode_t setVariable(const char *pathPtr, void *valuePtr, int type)
{
  rc_ReturnCode_t res;
  yajl_gen_status yres = 0;
  char* payload, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  if (pathPtr == NULL || (valuePtr == NULL && type != DT_TYPE_NULL))
    return RC_BAD_PARAMETER;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_array_open(gen);

  YAJL_GEN_STRING(pathPtr, "pathPtr");
  switch(type)
  {
    case DT_TYPE_INTEGER:
      yres = yajl_gen_integer(gen, *(int *)valuePtr);
      break;
    case DT_TYPE_DOUBLE:
      yres = yajl_gen_double(gen, *(double *)valuePtr);
      break;
    case DT_TYPE_STRING:
      yres = yajl_gen_string(gen, (const unsigned char *)valuePtr, strlen((char *)valuePtr));
      break;
    case DT_TYPE_NULL:
      yres = yajl_gen_null(gen);
      break;
    case DT_TYPE_BOOL:
      yres = yajl_gen_bool(gen, *(bool *)valuePtr);
      break;
    default:
      break;
  }

  if (yres != yajl_gen_status_ok)
  {
    SWI_LOG("DT", ERROR, "%s: value param serialization failed, res %d\n", __FUNCTION__, yres);
    return RC_BAD_PARAMETER;
  }
  yajl_gen_array_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_SETVARIABLE, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
     SWI_LOG("DT", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
  return res;
}

static rc_ReturnCode_t send_register_payload(const char *payload, size_t payloadLen, char **regId)
{
  rc_ReturnCode_t res;
  char *jsonPayload = NULL, *respPayload = NULL;
  uint32_t respPayloadLen = 0;
  yajl_val yval;

  res = emp_send_and_wait_response(EMP_REGISTERVARIABLE, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  if (RC_OK != res)
  {
    if (respPayload)
      free(respPayload);
    return res;
  }
  jsonPayload = strndup(respPayload, respPayloadLen);
  YAJL_TREE_PARSE(yval, jsonPayload);
  if (yval->type != yajl_t_string)
  {
    res = RC_BAD_PARAMETER;
    goto quit;
  }
  res = RC_OK;
  *regId = strndup(yval->u.string, strlen(yval->u.string));
quit:
  free(jsonPayload);
  yajl_tree_free(yval);
  return res;
}

static void empReregisterServices()
{
  cb_list_t *entry;
  rc_ReturnCode_t res;

  pthread_mutex_lock(&cb_lock);
  for (entry = cb_list; entry; entry = entry->next)
  {
    SWI_LOG("DT", DEBUG, "%s: reregistering payload %s\n", __FUNCTION__, entry->payload);
    free(entry->regId);
    res = send_register_payload(entry->payload, strlen(entry->payload), &entry->regId);
    if (res != RC_OK)
      SWI_LOG("DT", WARNING, "%s: Reregistration failed for callback \"cb=%p, payload=%s\" with code %d\n", __FUNCTION__, entry->cb, entry->payload, res);
  }
  pthread_mutex_unlock(&cb_lock);
}

rc_ReturnCode_t swi_dt_Init()
{
  rc_ReturnCode_t res;

  if (initialized)
    return RC_OK;

  res = emp_parser_init(1, empCmds, empHldrs, empReregisterServices);
  if (res != RC_OK)
  {
    SWI_LOG("DT", ERROR, "%s: error while init emp lib, res=%d\n", __FUNCTION__, res);
    return res;
  }
  initialized = 1;
  return RC_OK;
}

rc_ReturnCode_t swi_dt_Destroy()
{
  rc_ReturnCode_t res;

  if (!initialized)
    return RC_OK;

  res = emp_parser_destroy(1, empCmds, empReregisterServices);
  if (res != RC_OK)
  {
    SWI_LOG("DT", ERROR, "%s: error while destroy emp lib, res=%d\n", __FUNCTION__, res);
    return res;
  }

  initialized = 0;
  return RC_OK;
}

rc_ReturnCode_t swi_dt_Get(const char* pathPtr, swi_dset_Iterator_t** data, bool *isLeaf)
{
  rc_ReturnCode_t res = RC_OK;
  char* payload, *respPayload = NULL, *ptr = NULL;
  size_t payloadLen, len;
  uint32_t respPayloadLen = 0;
  uint8_t is_leaf = 1, null_value = 0;
  yajl_gen gen;
  yajl_val yval, yarray;
  int i;

  if (data == NULL)
    return RC_BAD_PARAMETER;

  SWI_LOG("DT", DEBUG, "%s: begin\n", __FUNCTION__);

  if (pathPtr == NULL)
  {
    *data = NULL;
    return RC_BAD_PARAMETER;
  }

  YAJL_GEN_ALLOC(gen);

  yajl_gen_array_open(gen);

  YAJL_GEN_STRING("", "prefix");
  YAJL_GEN_STRING(pathPtr, "pathPtr");
  res = swi_dset_Create(data);
  if (res != RC_OK)
  {
    SWI_LOG("DT", ERROR, "%s: dset allocation failed, res %d\n", __FUNCTION__, res);
    return res;
  }
  yajl_gen_array_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_GETVARIABLE, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("DT", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    if (respPayload)
    {
      SWI_LOG("DT", ERROR, "%s: respPayload = %.*s\n", __FUNCTION__, respPayloadLen, respPayload);
      free(respPayload);
    }
    return res;
  }

  payload = malloc(respPayloadLen + 1);
  if (payload == NULL)
  {
    SWI_LOG("DT", ERROR, "%s: Failed to alloc payload\n", __FUNCTION__);
    free(respPayload);
    return RC_NO_MEMORY;
  }

  memcpy(payload, respPayload, respPayloadLen);
  payload[respPayloadLen] = 0;

  YAJL_TREE_PARSE(yval, payload);
  if (yval->type != yajl_t_array)
  {
    SWI_LOG("DT", ERROR, "%s: Invalid object type returned by RA, expected array got %d\n", __FUNCTION__, yval->type);
    res = RC_BAD_FORMAT;
    goto status;
  }

  switch(yval->u.array.values[0]->type)
  {
  case yajl_t_number:
    if (yval->u.array.values[0]->u.number.flags & YAJL_NUMBER_INT_VALID)
    {
      res = swi_dset_PushInteger(*data, pathPtr,  strlen(pathPtr), yval->u.array.values[0]->u.number.i);
      SWI_LOG("DT", DEBUG, "%s: pushing int value %s -> %lld\n", __FUNCTION__, pathPtr, yval->u.array.values[0]->u.number.i);
    }
    else if (yval->u.array.values[0]->u.number.flags & YAJL_NUMBER_DOUBLE_VALID)
    {
      res = swi_dset_PushFloat(*data, pathPtr,  strlen(pathPtr), yval->u.array.values[0]->u.number.d);
      SWI_LOG("DT", DEBUG, "%s: pushing float value %s -> %lf\n", __FUNCTION__, pathPtr, yval->u.array.values[0]->u.number.d);
    }
    break;
  case yajl_t_string:
    res = swi_dset_PushString(*data, pathPtr, strlen(pathPtr), yval->u.array.values[0]->u.string, strlen(yval->u.array.values[0]->u.string));
    SWI_LOG("DT", DEBUG, "%s: pushing string value %s -> %s\n", __FUNCTION__, pathPtr, yval->u.array.values[0]->u.string);
    break;
  case yajl_t_null:
    null_value = 1;
    SWI_LOG("DT", DEBUG, "%s: null value, nothing pushed\n", __FUNCTION__);
    break;
  case yajl_t_true:
    res = swi_dset_PushBool(*data, pathPtr, strlen(pathPtr), true);
    SWI_LOG("DT", DEBUG, "%s: pushing true value.\n", __FUNCTION__);
    break;
  case yajl_t_false:
    res = swi_dset_PushBool(*data, pathPtr, strlen(pathPtr), false);
    SWI_LOG("DT", DEBUG, "%s: pushing false value.\n", __FUNCTION__);
    break;
  default:
    SWI_LOG("DT", DEBUG, "%s: Unsupported value type, got %d\n", __FUNCTION__, yval->u.array.values[0]->type);
    res = RC_BAD_FORMAT;
    goto status;
  }

  if (res != RC_OK)
  {
      SWI_LOG("DT", ERROR, "%s: Unable to push value to dset, res %d\n", __FUNCTION__, res);
      res = RC_UNSPECIFIED_ERROR;
      goto status;
  }

  if (yval->u.array.len < 2){
    //no more values in JSON payload, let's go to the end
    //is_leaf already set to 1.
    goto status;
  }

  //Additional values in JSON payload, likely the list of children of interior node.
  yarray = yval->u.array.values[1];
  SWI_LOG("DT", DEBUG, "%s: {\n", __FUNCTION__);
  for(i = 0; i < yarray->u.array.len; i++)
  {
    len = strlen(yarray->u.array.values[i]->u.string);
    ptr = strrchr(yarray->u.array.values[i]->u.string, '.');
    if (ptr == NULL)
      continue;
    is_leaf = 0;
    SWI_LOG("DT", DEBUG, "  %s: %s -> %.*s\n", __FUNCTION__, yarray->u.array.values[i]->u.string,
      (yarray->u.array.values[i]->u.string + len) - ptr, ptr + 1);
    swi_dset_PushString(*data, yarray->u.array.values[i]->u.string, len, ptr + 1, (yarray->u.array.values[i]->u.string + len) - ptr - 1);
  }
  SWI_LOG("DT", DEBUG, "%s: }\n", __FUNCTION__);

  if (null_value && is_leaf)
    res = RC_NOT_FOUND;

status:
  SWI_LOG("DT", DEBUG, "%s: end\n", __FUNCTION__);
  if (isLeaf)
    *isLeaf = is_leaf? true : false;
  free(payload);
  free(respPayload);
  yajl_tree_free(yval);
  return res;
}


rc_ReturnCode_t swi_dt_MultipleGet(size_t numVars, const char** pathPtr, swi_dset_Iterator_t** data)
{
  int i;
  rc_ReturnCode_t res;
  swi_dset_Iterator_t *set, *tmp;
  swi_dset_Type_t type;
  const char *name = NULL, *value = NULL;

  if (data == NULL)
    return RC_BAD_PARAMETER;

  if (pathPtr == NULL || numVars == 0){
    *data = NULL;
    return RC_BAD_PARAMETER;
  }


  res = swi_dset_Create(&set);
  if (res != RC_OK)
  {
    SWI_LOG("DT", ERROR, "%s: dset allocation failed, res %d\n", __FUNCTION__, res);
    return RC_NO_MEMORY;
  }

  for (i = 0; i < numVars; i++)
  {
    bool isLeaf = true;
    res = swi_dt_Get(pathPtr[i], &tmp, &isLeaf);
    if (!isLeaf)
    {
      swi_dset_Destroy(tmp);
      continue;
    }
    else if (res == RC_NOT_FOUND)
    {
      return res;
    }

    while (swi_dset_Next(tmp) != RC_NOT_FOUND)
    {
      type = swi_dset_GetType(tmp);
      name = swi_dset_GetName(tmp);
      switch(type)
      {
        case SWI_DSET_INTEGER:
          swi_dset_PushInteger(set, name, strlen(name), swi_dset_ToInteger(tmp));
      SWI_LOG("DT", DEBUG, "%s: concat %s=%ld\n", __FUNCTION__, name, swi_dset_ToInteger(tmp));
      break;
        case SWI_DSET_FLOAT:
      swi_dset_PushFloat(set, name, strlen(name), swi_dset_ToFloat(tmp));
      SWI_LOG("DT", DEBUG, "%s: concat %s=%f\n", __FUNCTION__, name, swi_dset_ToFloat(tmp));
      break;
        case SWI_DSET_STRING:
      value = swi_dset_ToString(tmp);
      swi_dset_PushString(set, name, strlen(name), value, strlen(value));
      SWI_LOG("DT", DEBUG, "%s: concat %s=%s\n", __FUNCTION__, name, value);
      break;
        default:
      break;
      }
    }
    swi_dset_Destroy(tmp);
  }
  *data = set;
  return RC_OK;
}

rc_ReturnCode_t swi_dt_SetInteger(const char* pathPtr, int value)
{
  return setVariable(pathPtr, &value, DT_TYPE_INTEGER);
}

rc_ReturnCode_t swi_dt_SetFloat(const char* pathPtr, double value)
{
  return setVariable(pathPtr, &value, DT_TYPE_DOUBLE);
}

rc_ReturnCode_t swi_dt_SetString(const char* pathPtr, const char* valuePtr)
{
  return setVariable(pathPtr, (void *)valuePtr, DT_TYPE_STRING);
}

rc_ReturnCode_t swi_dt_SetNull(const char *pathPtr)
{
  return setVariable(pathPtr, NULL, DT_TYPE_NULL);
}

rc_ReturnCode_t swi_dt_SetBool(const char* pathPtr, bool value)
{
  return setVariable(pathPtr, &value, DT_TYPE_BOOL);
}

rc_ReturnCode_t swi_dt_Register(size_t numRegVars, const char** regVarsPtr, swi_dt_NotifyCB_t cb,
    size_t numPassiveVars, const char** passiveVarsPtr, swi_dt_regId_t* regIdPtr)
{
  rc_ReturnCode_t res;
  char *regId = NULL, *payload = NULL;
  size_t payloadLen;
  int i = 0;
  cb_list_t *entry = NULL;
  yajl_gen gen;

  if(numRegVars == 0) // we could have returned RC_OK but there is no point to do a register without any var.
    return RC_BAD_PARAMETER;

  if (regVarsPtr == NULL || cb == NULL || regIdPtr == NULL)
    return RC_BAD_PARAMETER;

  if (numPassiveVars > 0 && passiveVarsPtr == NULL) //numPassiveVars are optional
    return RC_BAD_PARAMETER;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_array_open(gen);

  YAJL_GEN_STRING("", "prefix");

  yajl_gen_array_open(gen);
  for (i = 0; i < numRegVars; i++)
    YAJL_GEN_STRING(regVarsPtr[i], "regVars");
  yajl_gen_array_close(gen);

  yajl_gen_array_open(gen);
  for (i = 0; i < numPassiveVars; i++)
    YAJL_GEN_STRING(passiveVarsPtr[i], "passiveVars");
  yajl_gen_array_close(gen);

  yajl_gen_array_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = send_register_payload(payload, payloadLen, &regId);

  if (res == RC_BAD_PARAMETER)
  {
    SWI_LOG("DT", ERROR, "%s: Invalid object type returned by agent for registration id\n", __FUNCTION__);
    goto quit;
  }
  else if (res != RC_OK)
  {
    SWI_LOG("DT", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    goto quit;
  }

  pthread_mutex_lock(&cb_lock);
  if (cb_list == NULL)
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
  entry->cb = cb;
  entry->payload = strndup(payload, payloadLen);
  entry->next = NULL;
  pthread_mutex_unlock(&cb_lock);

  *regIdPtr = entry;
quit:
  yajl_gen_clear(gen);
  yajl_gen_free(gen);
  return res;
}

rc_ReturnCode_t swi_dt_Unregister(swi_dt_regId_t regId)
{
  rc_ReturnCode_t res;
  char* payload, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  cb_list_t *entry = NULL, *tmp = NULL;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);
  for (entry = cb_list; entry; entry = entry->next)
  {
    if (entry == regId)
      break;
  }

  if (entry == NULL)
    return RC_OK;

  YAJL_GEN_STRING(entry->regId, "regId");
  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_DEREGISTERVARIABLE, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("DT", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    if (respPayload)
    {
      SWI_LOG("DT", ERROR, "%s: respPayload = %.*s\n", __FUNCTION__, respPayloadLen, respPayload);
      free(respPayload);
    }
    return res;
  }

  pthread_mutex_lock(&cb_lock);
  if (cb_list == regId)
  {
    tmp = cb_list;
    cb_list = cb_list->next;
    free(tmp->regId);
    free(tmp->payload);
    free(tmp);
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
    free(tmp->regId);
    free(tmp->payload);
    free(tmp);
  }
  pthread_mutex_unlock(&cb_lock);

  free(respPayload);
  return RC_OK;
}
