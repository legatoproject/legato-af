/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Minh Giang         for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#define _GNU_SOURCE // this is required for strndup, which might cause warnings depending on the used toolchains
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include "swi_airvantage.h"
#include "emp.h"
#include "swi_log.h"
#include "swi_dset.h"
#include "dset_internal.h" //we need access to private features of dset
#include "yajl_tree.h"
#include "yajl_gen.h"
#include "yajl_helpers.h"

#define ASSET_MAGIC_ID 0x32aeb53a // For CHECK_CONTEXT
#define CHECK_ASSET(asset)  if (!asset || asset->magic != ASSET_MAGIC_ID) return RC_BAD_PARAMETER

#define CHECK_RETURN(a) do { if ((res=(a)) != RC_OK){ return res;} } while(0)

#define SWI_AV_TABLE_ENTRY_STRING 0
#define SWI_AV_TABLE_ENTRY_INT 1
#define SWI_AV_TABLE_ENTRY_DOUBLE 2

static char initialized = 0;

/*
 * Map of all started asset
 * keys are asset names, values: int64 holding pointers to asset struct.
 */
static swi_dset_Iterator_t* assetList;

static rc_ReturnCode_t empSendDataHdlr(uint32_t payloadsize, char* payload);
static rc_ReturnCode_t empUpdateNotifHdlr(uint32_t payloadsize, char* payload);
static EmpCommand empCmds[] = { EMP_SENDDATA, EMP_SOFTWAREUPDATE };
static emp_command_hdl_t empHldrs[] = { empSendDataHdlr, empUpdateNotifHdlr };
static pthread_mutex_t assets_lock = PTHREAD_MUTEX_INITIALIZER;

struct swi_av_Asset
{
  unsigned int magic;
  char started;
  /*Data reception:*/
  swi_av_DataWriteCB dwCb; // Data Writing callback and userdata
  void* dwCbUd;
  swi_av_updateNotificationCB updCb; // Asset Update Notification callback and userdata
  void* updCbUd;
  char* assetId;
};

struct swi_av_Table
{
  const char *identifier;
  const char **column;
  int columnSize;

  struct
  {
    int len;
    struct swi_av_table_entry
    {
      union
      {
        char *string;
        int i;
        double d;
      } u;
      uint8_t type;
    }*data;
  } row;
};

static rc_ReturnCode_t send_asset_registration(swi_av_Asset_t *asset)
{
  rc_ReturnCode_t res;
  char *payload = NULL, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);

  YAJL_GEN_STRING(asset->assetId, "assetId");
  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_REGISTER, 0, payload, payloadLen,
                   &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  free(respPayload);
  return res;
}

static void empReregisterServices()
{
  swi_av_Asset_t *asset;
  rc_ReturnCode_t res;

  while (swi_dset_Next(assetList) != RC_NOT_FOUND)
  {
    asset = (swi_av_Asset_t *)(intptr_t)swi_dset_ToInteger(assetList);
    res = send_asset_registration(asset);
    if (res != RC_OK)
      SWI_LOG("AV", WARNING, "Failed to register back asset %s, res = %d\n", asset->assetId, res);
  }
  swi_dset_Rewind(assetList);
}

rc_ReturnCode_t swi_av_Init()
{
  if (initialized)
    return RC_OK;

  //init emp parser, registering 2 emp command handlers
  rc_ReturnCode_t res = emp_parser_init(2, empCmds, empHldrs, empReregisterServices);
  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "error while init emp lib, res=%d\n", res);
    return res;
  }

  //create internal data
  res = swi_dset_Create(&assetList);
  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "error while creating, res=%d\n", res);
    swi_av_Destroy();
    return res;
  }
  initialized = 1;

  return RC_OK;
}

rc_ReturnCode_t swi_av_Destroy()
{
  if (!initialized)
    return RC_OK;

  //destroy emp parser, un-registerering the 2 EMP command callbacks
  rc_ReturnCode_t res = emp_parser_destroy(2, empCmds, empReregisterServices);
  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "error while destroying emp lib, res=%d\n", res);
    return res;
  }

  //we can directly destroy the dset
  //it is up to the user to release any asset that would still be there.
  pthread_mutex_lock(&assets_lock);
  res = swi_dset_Destroy(assetList);
  pthread_mutex_unlock(&assets_lock);
  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "error while destroying dset, res=%d\n", res);
    return res;
  }

  initialized = 0;
  return RC_OK;
}

rc_ReturnCode_t swi_av_ConnectToServer(unsigned int latency)
{
  rc_ReturnCode_t res;
  char* payload, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  SWI_LOG("AV", DEBUG, "%s: latency=%u\n", __FUNCTION__, latency);
  if (latency == SWI_AV_CX_SYNC)
    return RC_OK;

  YAJL_GEN_ALLOC(gen);

  YAJL_GEN_INTEGER(latency, "latency");
  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_CONNECTTOSERVER, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("AV", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    if (respPayload)
    {
      SWI_LOG("AV", ERROR, "%s: respPayload = %.*s\n", __FUNCTION__, respPayloadLen, respPayload);
      free(respPayload);
    }
    return res;
  }
  free(respPayload);
  return RC_OK;
}

rc_ReturnCode_t swi_av_TriggerPolicy(const char* policyPtr)
{
  rc_ReturnCode_t res;
  char* payload, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_map_open(gen);

  if(policyPtr != NULL){ //'default' policy is designated when policy field is absent
    YAJL_GEN_STRING("policy", "policy");
    YAJL_GEN_STRING(policyPtr, "policyPtr");
  }

  yajl_gen_map_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_PFLUSH, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("AV", ERROR, "%s: failed to send EMP cmd, res = %d\n", __FUNCTION__, res);
    if (respPayload)
    {
      SWI_LOG("AV", ERROR, "%s: respPayload = %.*s\n", __FUNCTION__, respPayloadLen, respPayload);
      free(respPayload);
    }
    return res;
  }
  free(respPayload);
  return RC_OK;
}

rc_ReturnCode_t swi_av_asset_Create(swi_av_Asset_t** asset, const char* assetIdPtr)
{
  swi_av_Asset_t* a = NULL;
  if (NULL == assetIdPtr || 0 == strlen(assetIdPtr) || NULL == asset)
    return RC_BAD_PARAMETER;

  a = malloc(sizeof(*a) + strlen(assetIdPtr) + 1);
  if (NULL == a)
    return RC_NO_MEMORY;
  a->assetId = (char*) (a + 1); //assetId space starts right after the struct space
  strcpy(a->assetId, assetIdPtr);

  a->started = 0;
  a->magic = ASSET_MAGIC_ID;

  *asset = a;
  return RC_OK;
}

rc_ReturnCode_t swi_av_asset_Start(swi_av_Asset_t* asset)
{
  rc_ReturnCode_t res;

  CHECK_ASSET(asset);

  if (asset->started)
    return RC_OK;

  res = send_asset_registration(asset);
  if (RC_OK != res)
  {
    SWI_LOG("AV", ERROR, "%s: failed to register, res = %d\n", __FUNCTION__, res);
    return res;
  }

  asset->started = 1;
  pthread_mutex_lock(&assets_lock);
  //once the asset is started/registered, it is ready to received update/dataWriting.
  //if register cmd has succeeded, RA ensure assetId can't be repeated, so it's ok to add it to dset
  res = swi_dset_PushInteger(assetList, asset->assetId, strlen(asset->assetId), (uintptr_t) asset);
  if (RC_OK != res)
  {
    SWI_LOG("AV", ERROR, "%s: failed to add asset to list, res = %d\n", __FUNCTION__, res);
  }
  pthread_mutex_unlock(&assets_lock);
  return res;
}

rc_ReturnCode_t swi_av_asset_Destroy(swi_av_Asset_t* asset)
{
  CHECK_ASSET(asset);
  swi_dset_Element_t* elt = NULL;
  rc_ReturnCode_t res;
  char* payload, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  if (!asset->started)
    return RC_OK;

  YAJL_GEN_ALLOC(gen);

  YAJL_GEN_STRING(asset->assetId, "assetId");
  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_UNREGISTER, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("AV", ERROR, "%s: failed to unregister, res = %d\n", __FUNCTION__, res);
    return res;
  }
  pthread_mutex_lock(&assets_lock);
  swi_dset_RemoveByName(assetList, asset->assetId, &elt);
  pthread_mutex_unlock(&assets_lock);
  if (elt)
    free(elt);
  asset->magic = ~asset->magic;
  free(asset); //no need to free assetid, done in one alloc
  free(respPayload);
  return res;
}

/*
 * internal enum used to implement swi_av_asset_Push* functions
 * with only one common function swi_av_asset_Push
 */
typedef enum PDataType
{
  PData_Int, PData_Float, PData_String
} PDataType_t;

/* split path like "toto.titi.tutu" in "toto.titi" and "tutu"
 * simple one for now: only last element, only '.' as separator.
 *
 * pathPtr: must be null terminated, cannot start by '.', ends by '.', or contains '..' sequences.
 * parentPath and varName will be allocated by this fct, need to be freed by caller.
 * using example "toto.titi.tutu", parentPath is "toto.titi", varName is "tutu"
 */
rc_ReturnCode_t get_path_element(int first, const char* pathPtr, char** remainingPath, char** varName)
{
  int pathLeftPartSize = 0;
  const char* pathLeftPartTmp = NULL; //points chars in pathPtr
  const char* pathRightPartTmp = NULL; //points chars in pathPtr
  int pathRightPartSize = 0;
  char* pathLeftPart = NULL; //will be allocated and returned
  char* pathRightPart = NULL; //will be allocated and returned
  SWI_LOG("AV", DEBUG, "%s: %s\n", __FUNCTION__, pathPtr);
  //forbids starting '.', trailing '.' or ".." sequence
  if (pathPtr[0] == '.' || pathPtr[strlen(pathPtr) - 1] == '.' || strstr(pathPtr, ".."))
    return RC_BAD_PARAMETER;

  if (first)
    pathRightPartTmp = strchr(pathPtr, '.'); //first occurrence of '.'
  else
    pathRightPartTmp = strrchr(pathPtr, '.'); //last occurrence of '.'

  if (NULL == pathRightPartTmp) //no occurrence of '.'
  {
    if (first)
    {
      pathRightPartTmp = ""; //we'll allocate and serialize "", may be optimized
      pathRightPartSize = 1;
      pathLeftPartTmp = pathPtr;
      pathLeftPartSize = strlen(pathPtr) + 1; //+1 for '\0'
    }
    else
    {
      pathLeftPartTmp = ""; //we'll allocate and serialize "", may be optimized
      pathLeftPartSize = 1;
      pathRightPartTmp = pathPtr;
      pathRightPartSize = strlen(pathPtr) + 1; //+1 for '\0'
    }
  }
  else
  {
    pathLeftPartTmp = pathPtr;
    pathLeftPartSize = pathRightPartTmp - pathPtr + 1; //we don't count last '.', +1 for '\0'
    pathRightPartTmp++; //we are on the last '.' in pathPtr, just go after it
    pathRightPartSize = strlen(pathRightPartTmp) + 1; //pathPtr is correctly ended , we can strlen
  }

  SWI_LOG("AV", DEBUG, "%s: left[%s][%d], right[%s][%d]\n",
      __FUNCTION__, pathLeftPartTmp, pathLeftPartSize, pathRightPartTmp, pathRightPartSize);

  pathLeftPart = malloc(pathLeftPartSize);
  if (NULL == pathLeftPart)
    return RC_NO_MEMORY;
  //bzero(pathLeftPart, pathLeftPartSize);
  pathRightPart = malloc(pathRightPartSize);
  if (NULL == pathRightPart)
  {
    free(pathLeftPart);
    return RC_NO_MEMORY;
  }
  //bzero(pathRightPart, pathRightPartSize);
  strncpy(pathLeftPart, pathLeftPartTmp, pathLeftPartSize - 1);
  //when pathLeftPartTmp is pathPtr, strncpy will copy part of pathPtr *without* adding '\0', so add it!
  pathLeftPart[pathLeftPartSize - 1] = '\0';
  strncpy(pathRightPart, pathRightPartTmp, pathRightPartSize);
  //here strncpy will copy ending '\0' of pathPtr

  SWI_LOG("AV", DEBUG, "%s: pathLeftPart=%s, pathRightPart=%s\n", __FUNCTION__, pathLeftPart, pathRightPart);

  if (first)
  {
    *remainingPath = pathRightPart;
    *varName = pathLeftPart;
  }
  else
  {
    *remainingPath = pathLeftPart;
    *varName = pathRightPart;
    SWI_LOG("AV", DEBUG, "%s: remainingPath=%s, varName=%s\n", __FUNCTION__, *remainingPath, *varName);
  }

  return RC_OK;
}

/*
 * internal function to push simple data
 */
static rc_ReturnCode_t swi_av_asset_Push(swi_av_Asset_t* asset, const char *pathPtr, const char* policyPtr,
    uint32_t timestamp, PDataType_t dataType, void* valuePtr)
{
  rc_ReturnCode_t res;
  yajl_gen_status yres = 0;
  char *payload = NULL, *respPayload = NULL, *globalPath = NULL, *varName = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  CHECK_ASSET(asset);
  if (NULL == pathPtr || NULL == pathPtr || 0 == strlen(pathPtr))
    return RC_BAD_PARAMETER;

  YAJL_GEN_ALLOC(gen);

  //splitting path in parentPath/variableName
  res = get_path_element(0, pathPtr, &globalPath, &varName);
  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "%s: Failed to get path element, res %d\n", __FUNCTION__, res);
    return RC_BAD_PARAMETER;
  }

  SWI_LOG("AV", DEBUG, "%s: globalPath=%s, varName=%s\n", __FUNCTION__, globalPath, varName);

  yajl_gen_map_open(gen);

  YAJL_GEN_STRING("asset", "asset");
  YAJL_GEN_STRING(asset->assetId, "assetId");
  YAJL_GEN_STRING("path", "path");
  YAJL_GEN_STRING(globalPath, "globalPath");
  if(policyPtr != NULL){ //'default' policy is designated when policy field is absent
    YAJL_GEN_STRING("policy", "policy");
    YAJL_GEN_STRING(policyPtr, "policyPtr");
  }
  YAJL_GEN_STRING("data", "data");

  //data is a map with timestamp(if requested) and varname.
  yajl_gen_map_open(gen);

  if (SWI_AV_TSTAMP_NO != timestamp)
  {
    yres = yajl_gen_string(gen, (const unsigned char *) "timestamp", strlen("timestamp"));
    if (SWI_AV_TSTAMP_AUTO == timestamp)
    {
      time_t t;
      if (((time_t) -1) == time(&t))
      {
        SWI_LOG("AV", ERROR, "%s: time() failed: %s\n", __FUNCTION__, strerror(errno));
        res = RC_UNSPECIFIED_ERROR;
      }
      YAJL_GEN_INTEGER(t, "timestamp");
    }
    else
    {
      YAJL_GEN_INTEGER(timestamp, "timestamp");
    }
  }

  YAJL_GEN_STRING(varName, "varName");

  //variable type depends on dataType args
  switch (dataType)
  {
    case PData_Float:
      yres = yajl_gen_double(gen, *(double *) valuePtr);
      break;

    case PData_Int:
      yres = yajl_gen_integer(gen, *(int *) valuePtr);
      break;

    case PData_String:
      yres = yajl_gen_string(gen, (const unsigned char *) valuePtr, strlen((char *) valuePtr));
      break;

    default:
      SWI_LOG("AV", ERROR, "%s: internal error, invalid type\n", __FUNCTION__);
      res = RC_UNSPECIFIED_ERROR;
      break;
  }

  if (yres != yajl_gen_status_ok)
  {
    SWI_LOG("AV", ERROR, "%s: valuePtr serialization failed, res %d\n", __FUNCTION__, yres);
    return RC_BAD_FORMAT;
  }
  yajl_gen_map_close(gen); // close data map
  yajl_gen_map_close(gen); // close eclosing map

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_PDATA, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (RC_OK != res)
  {
    SWI_LOG("AV", ERROR, "%s: failed to send EMP_PFLUSH cmd, res = %d\n", __FUNCTION__, res);
    if (NULL != respPayload)
    {
      SWI_LOG("AV", ERROR, "%s: respPayload data = %.*s\n", __FUNCTION__, respPayloadLen, respPayload);
      free(respPayload);
    }

    return res;
  }

  free(globalPath);
  free(varName);
  free(respPayload);

  return RC_OK;
}

rc_ReturnCode_t swi_av_asset_PushString(swi_av_Asset_t* asset, const char *pathPtr, const char* policyPtr,
    uint32_t timestamp, const char* valuePtr)
{
  if (NULL == valuePtr)
    return RC_BAD_PARAMETER;
  return swi_av_asset_Push(asset, pathPtr, policyPtr, timestamp, PData_String, (void *) valuePtr);
}

rc_ReturnCode_t swi_av_asset_PushInteger(swi_av_Asset_t* asset, const char *pathPtr, const char* policyPtr,
    uint32_t timestamp, int64_t value)
{
  return swi_av_asset_Push(asset, pathPtr, policyPtr, timestamp, PData_Int, &value);
}

rc_ReturnCode_t swi_av_asset_PushFloat(swi_av_Asset_t* asset, const char *pathPtr, const char* policyPtr,
    uint32_t timestamp, double value)
{
  return swi_av_asset_Push(asset, pathPtr, policyPtr, timestamp, PData_Float, &value);
}

rc_ReturnCode_t swi_av_table_Create(swi_av_Asset_t* asset, swi_av_Table_t** table, const char* pathPtr, size_t numColumns,
    const char** columnNamesPtr, const char* policyPtr, swi_av_Table_Storage_t persisted, int purge)
{
  int i;
  rc_ReturnCode_t res;
  char *payload = NULL, *respPayload = NULL;
  const char *storage;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;
  yajl_val yval;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_map_open(gen);
  YAJL_GEN_STRING("asset", "asset");
  YAJL_GEN_STRING(asset->assetId, "assetId");

  YAJL_GEN_STRING("storage", "storage");
  switch (persisted)
  {
    case STORAGE_RAM:
      storage = "ram";
      break;
    case STORAGE_FLASH:
      storage = "flash";
      break;
    default:
      return RC_BAD_PARAMETER;
  }
  YAJL_GEN_STRING(storage, "storage value");
  if(policyPtr != NULL){ //'default' policy is designated when policy field is absent
    YAJL_GEN_STRING("policy", "policy");
    YAJL_GEN_STRING(policyPtr, "policyPtr");
  }

  YAJL_GEN_STRING("path", "path");
  YAJL_GEN_STRING(pathPtr, "pathPtr");

  YAJL_GEN_STRING("columns", "columns");

  yajl_gen_array_open(gen);
  for (i = 0; i < numColumns; i++)
  {
    YAJL_GEN_STRING(columnNamesPtr[i], "columnNamesPtr");
  }
  yajl_gen_array_close(gen);

  yajl_gen_map_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_TABLENEW, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "%s: EMP command failed, res %d\n", __FUNCTION__, res);
    free(respPayload);
    return res;
  }

  payload = __builtin_strndup(respPayload, respPayloadLen);
  YAJL_TREE_PARSE(yval, payload);
  if (yval->type != yajl_t_string)
  {
    SWI_LOG("AV", ERROR, "%s: Invalid payload received from RA, expected array got type=%u\n",
        __FUNCTION__, yval->type);
    free(respPayload);
    return RC_BAD_FORMAT;
  }

  *table = malloc(sizeof(swi_av_Table_t));
  (*table)->identifier = strdup(yval->u.string);
  (*table)->column = malloc(numColumns * sizeof(const char *));
  (*table)->columnSize = numColumns;
  (*table)->row.data = malloc(numColumns * sizeof(struct swi_av_table_entry));
  (*table)->row.len = 0;

  for (i = 0; i < numColumns; i++)
    (*table)->column[i] = strdup(columnNamesPtr[i]);
  free(payload);
  free(respPayload);
  yajl_tree_free(yval);
  return RC_OK;
}

rc_ReturnCode_t swi_av_table_Destroy(swi_av_Table_t* table)
{
  int i;
  rc_ReturnCode_t res;
  char *payload = NULL, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_map_open(gen);

  YAJL_GEN_STRING("table", "table");
  YAJL_GEN_STRING(table->identifier, "tableId");

  yajl_gen_map_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_TABLERESET, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "%s: EMP command failed, res %d\n", __FUNCTION__, res);
    free(respPayload);
    return res;
  }

  free((char *) table->identifier);
  for (i = 0; i < table->columnSize; i++)
    free((char *) table->column[i]);
  free(table->column);
  free(table->row.data);
  free(table);
  free(respPayload);
  return RC_OK;
}

rc_ReturnCode_t swi_av_table_PushFloat(swi_av_Table_t* table, double value)
{
  if (table->row.len >= table->columnSize)
    return RC_OUT_OF_RANGE;
  table->row.data[table->row.len].type = SWI_AV_TABLE_ENTRY_DOUBLE;
  table->row.data[table->row.len++].u.d = value;
  return RC_OK;
}

rc_ReturnCode_t swi_av_table_PushInteger(swi_av_Table_t* table, int value)
{
  if (table->row.len >= table->columnSize)
    return RC_OUT_OF_RANGE;
  table->row.data[table->row.len].type = SWI_AV_TABLE_ENTRY_INT;
  table->row.data[table->row.len++].u.i = value;
  return RC_OK;
}

rc_ReturnCode_t swi_av_table_PushString(swi_av_Table_t* table, const char* value)
{
  if (table->row.len >= table->columnSize)
    return RC_OUT_OF_RANGE;
  table->row.data[table->row.len].type = SWI_AV_TABLE_ENTRY_STRING;
  table->row.data[table->row.len++].u.string = strdup(value);
  return RC_OK;
}

rc_ReturnCode_t swi_av_table_PushRow(swi_av_Table_t* table)
{
  int i;
  rc_ReturnCode_t res;
  char *payload = NULL, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_map_open(gen);

  YAJL_GEN_STRING("table", "table");
  YAJL_GEN_STRING(table->identifier, "tableId");

  YAJL_GEN_STRING("row", "row");
  yajl_gen_map_open(gen);
  for (i = 0; i < table->row.len; i++)
  {
    YAJL_GEN_STRING(table->column[i], table->column[i]);

    switch (table->row.data[i].type)
    {
      case SWI_AV_TABLE_ENTRY_STRING:
        YAJL_GEN_STRING(table->row.data[i].u.string, "string value");
        break;
      case SWI_AV_TABLE_ENTRY_INT:
        YAJL_GEN_INTEGER(table->row.data[i].u.i, "int value");
        break;
      case SWI_AV_TABLE_ENTRY_DOUBLE:
        YAJL_GEN_DOUBLE(table->row.data[i].u.d, "string value");
        break;
      default:
        break;
    }
  }
  yajl_gen_map_close(gen);

  yajl_gen_map_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_TABLEROW, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "%s: EMP command failed, res %d\n", __FUNCTION__, res);
    free(respPayload);
    return res;
  }
  for (i = 0; i < table->row.len; i++)
    if (table->row.data[i].type == SWI_AV_TABLE_ENTRY_STRING)
      free(table->row.data[i].u.string);
  bzero(table->row.data, table->row.len * sizeof(struct swi_av_table_entry));
  table->row.len = 0;
  free(respPayload);
  return RC_OK;
}

rc_ReturnCode_t swi_av_RegisterDataWrite(swi_av_Asset_t *asset, swi_av_DataWriteCB cb, void * userDataPtr)
{
  CHECK_ASSET(asset);
  asset->dwCb = cb;
  asset->dwCbUd = userDataPtr;
  return RC_OK;
}

rc_ReturnCode_t swi_av_Acknowledge(int ackId, int status, const char* errMsgPtr, const char* policyPtr, int persisted)
{
  rc_ReturnCode_t res;
  char *payload = NULL, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_map_open(gen);

  YAJL_GEN_STRING("ticket", "ticket");
  YAJL_GEN_INTEGER(ackId, "ackId");

  YAJL_GEN_STRING("status", "status key");
  YAJL_GEN_INTEGER(status, "status value");

  YAJL_GEN_STRING("message", "message");
  YAJL_GEN_STRING(errMsgPtr, "errMsgPtr");

  if(policyPtr != NULL){ //'now' policy is designated when policy field is absent
    YAJL_GEN_STRING("policy", "policy");
    YAJL_GEN_STRING(policyPtr, "policyPtr");
  }

  YAJL_GEN_STRING("persisted", "persisted key");
  YAJL_GEN_INTEGER(persisted, "persisted value");

  yajl_gen_map_close(gen);

  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_PACKNOWLEDGE, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {
    SWI_LOG("AV", ERROR, "%s: Acknowlegment failed, res %d\n", __FUNCTION__, res);
    free(respPayload);
    return res;
  }
  free(respPayload);
  return RC_OK;
}

rc_ReturnCode_t swi_av_RegisterUpdateNotification(swi_av_Asset_t* asset, swi_av_updateNotificationCB cb, void *userDataPtr)
{
  CHECK_ASSET(asset);
  asset->updCb = cb;
  asset->updCbUd = userDataPtr;
  return RC_OK;
}

rc_ReturnCode_t swi_av_SendUpdateResult(swi_av_Asset_t* asset, const char* componentNamePtr, int updateResult)
{
  rc_ReturnCode_t res;
  char *payload = NULL, *respPayload = NULL;
  size_t payloadLen;
  uint32_t respPayloadLen = 0;
  yajl_gen gen;

  YAJL_GEN_ALLOC(gen);

  yajl_gen_array_open(gen);

  //concatenate assetId + componentName before sending SoftwareUpdateResult EMP message
  char* assetIdComponentName = malloc(sizeof(char)*( strlen(asset->assetId) + strlen(componentNamePtr)) + 2);
  strcpy(assetIdComponentName, asset->assetId);
  if (strcmp(componentNamePtr, ""))
  {
    strcat(assetIdComponentName, ".");
    strcat(assetIdComponentName, componentNamePtr);
  }

  YAJL_GEN_STRING(assetIdComponentName, "assetIdComponentName");
  YAJL_GEN_INTEGER(updateResult, "updateResult");

  free(assetIdComponentName);
  yajl_gen_array_close(gen);
  YAJL_GEN_GET_BUF(payload, payloadLen);

  res = emp_send_and_wait_response(EMP_SOFTWAREUPDATERESULT, 0, payload, payloadLen, &respPayload, &respPayloadLen);
  yajl_gen_clear(gen);
  yajl_gen_free(gen);

  if (res != RC_OK)
  {

    SWI_LOG("AV", ERROR, "%s:  Unable to send the result to the agent, res %d\n",
          __FUNCTION__, res);
    return res;
  }
  return RC_OK;
}

/*
 * Push values in dSet object according to types
 * Input : vals, i, name, namelength
 * Ouput : SetOut
 */
rc_ReturnCode_t pushDSet(swi_dset_Iterator_t** setOut, yajl_val* vals, int i, const char* name, size_t namelength)
{
  rc_ReturnCode_t res;

  switch (vals[i]->type)
  {
    case yajl_t_true:
      res = swi_dset_PushBool(*setOut, name, namelength, true);
      break;

    case yajl_t_false:
      res = swi_dset_PushBool(*setOut, name, namelength, false);
      break;

    case yajl_t_null:
      res = swi_dset_PushNull(*setOut, name, namelength);
      break;

    case yajl_t_string:
      res = swi_dset_PushString(*setOut, name, namelength, vals[i]->u.string, strlen(vals[i]->u.string));
      break;

    case yajl_t_number:
      if (YAJL_IS_INTEGER(vals[i]))
        res = swi_dset_PushInteger(*setOut, name, namelength, vals[i]->u.number.i);
      else
      {
        if (YAJL_IS_DOUBLE(vals[i]))
          res = swi_dset_PushFloat(*setOut, name, namelength, vals[i]->u.number.d);
        else
          res = swi_dset_PushUnsupported(*setOut, name, namelength);
      }
      break;

    default:
      res = swi_dset_PushUnsupported(*setOut, name, namelength);
      break;
  }
  return res;
}

/*
 * Allow to push data in object Dset according to array type or map type of 'yajlValue' value
 * Input : yajlValue
 * Output: dset object
 */
rc_ReturnCode_t process(yajl_val yajlValue, swi_dset_Iterator_t* setOut)
{
  SWI_LOG("AV", DEBUG, "%s\n", __FUNCTION__);

  rc_ReturnCode_t res = RC_OK;
  char* key = NULL;
  int length = yajlValue->type == yajl_t_object ? yajlValue->u.object.len : yajlValue->u.array.len;
  yajl_val* vals = yajlValue->type == yajl_t_object ? yajlValue->u.object.values : yajlValue->u.array.values;
  int i;
  for (i = 0; i < length; i++)
  {
    if (yajlValue->type == yajl_t_object)
      pushDSet(&setOut, vals, i, yajlValue->u.object.keys[i], strlen(yajlValue->u.object.keys[i]));

    else /*it is a list, we create a key number for each element*/
    {
      int keyLength = snprintf(key, 0, "%d", i);
      key = malloc(keyLength + 1);
      if (NULL == key)
        return RC_NO_MEMORY;
      snprintf(key, keyLength + 1, "%d", i);

      pushDSet(&setOut, vals, i, key, strlen(key));
      free(key);
    }
  }
  return res;
}

rc_ReturnCode_t processDataWriting(yajl_val body, swi_dset_Iterator_t* setOut)
{
  SWI_LOG("AV", DEBUG, "%s\n", __FUNCTION__);

  rc_ReturnCode_t res = RC_OK;
  res = process(body, setOut);

  return res;
}

rc_ReturnCode_t processResponse(yajl_val body)
{
  //todo improve
  SWI_LOG("AV", INFO, "Received response\n");
  return RC_OK;
}

/*
 * input : path   - is a remaining path (without asset id, can be ""), this path must be malloc and be freed within the function
 *       : body   - is a variable which contains the contents of command, the command will be modified in M3DA style
 * output: setOut - dset object
 * */
rc_ReturnCode_t processCommand(yajl_val body, swi_dset_Iterator_t* setOut, char** path)
{
  SWI_LOG("AV", DEBUG, "%s\n", __FUNCTION__);

  rc_ReturnCode_t res = RC_OK;
  yajl_val sub_body = NULL;
  const char* commandName = NULL;

  if (body->type != yajl_t_object)
  {
    SWI_LOG("AV", ERROR, "%s: Invalid body class received from RA, expected object type, got type=%u\n",
        __FUNCTION__, body->type);
    return res = RC_BAD_FORMAT;
  }

  int i;
  /*we get command name*/
  for (i = 0; i < body->u.object.len; i++)
  {
    if (!strcmp(body->u.object.keys[i], "Command"))
    {
      commandName = body->u.object.values[i]->u.string;
      break;
    }
  }

  int commandNameLength = strlen(commandName);

  /*compute new path for command (M3DA style) by adding "commands." string*/
  char* commandPath = NULL;
  int pathLength = strlen(*path);
  int commandPathLength = 0;
  if (pathLength)
  {
    // "commands" + "." + $path + "." + $cmdname + '\0'
    commandPathLength = 8 + 1 + pathLength + 1 + commandNameLength + 1;
    commandPath = malloc(commandPathLength);
    strcpy(commandPath, "commands.");
    strncat(commandPath, commandName, commandNameLength);
    strcat(commandPath, ".");
    strcat(commandPath, *path);
    commandPath[commandPathLength - 1] = '\0';
  }
  else
  {
    // "commands" + "." + $cmdname + '\0'
    commandPathLength = 8 + 1 + commandNameLength + 1;
    commandPath = malloc(commandPathLength);
    strcpy(commandPath, "commands.");
    strncat(commandPath, commandName, commandNameLength);
    commandPath[commandPathLength - 1] = '\0';
  }

  free(*path);
  *path = commandPath;

  /*we get the object corresponding with the key "Args"*/
  for (i = 0; i < body->u.object.len; i++)
  {
    if (!strcmp(body->u.object.keys[i], "Args"))
    {
      sub_body = body->u.object.values[i];
      if (sub_body->type == yajl_t_object || sub_body->type == yajl_t_array)
        res = process(sub_body, setOut);

      else
      {
        SWI_LOG("AV", ERROR,
            "%s: Invalid sub-body class received from RA, expected object or array type, got type=%u\n",
            __FUNCTION__, sub_body->type);
        return res = RC_BAD_FORMAT;
      }
      break;
    }
  }

  //commandPath will need to be freed by caller.
  SWI_LOG("AV", DEBUG, "%s: %s\n", __FUNCTION__, *path);
  return res;
}

/* read common data of all messages
 * Input  : yval
 * Output : body, body_class, path, ticket_id
 */
rc_ReturnCode_t readMessage(yajl_val* yval, yajl_val* body, char ** body_class, char ** path, int* ticket_id)
{
  SWI_LOG("AV", DEBUG, "%s...\n", __FUNCTION__);

  char format_error = 0;
  char* class = NULL;
  int i;

  /*parse message in yval value*/
  for (i = 0; i < (*yval)->u.object.len; i++)
  {
    //Path
    if (!strcmp((*yval)->u.object.keys[i], "Path"))
    {
      if ((*yval)->u.object.values[i]->type != yajl_t_string)
      {
        format_error = 1;
        break;
      }
      *path = (*yval)->u.object.values[i]->u.string;
      continue;
    }

    /*Ticketid*/
    if (!strcmp((*yval)->u.object.keys[i], "TicketId"))
    {
      if ((*yval)->u.object.values[i]->type != yajl_t_number)
      {
        format_error = 1;
        break;
      }
      *ticket_id = (*yval)->u.object.values[i]->u.number.i;
      continue;
    }

    /*Body*/
    if (!strcmp((*yval)->u.object.keys[i], "Body"))
    {
      if ((*yval)->u.object.values[i]->type != yajl_t_object && (*yval)->u.object.values[i]->type != yajl_t_array)
      {
        format_error = 1;
        break;
      }
      *body = (*yval)->u.object.values[i];
      continue;
    }

    /*__class: m3da class (must be message here)*/
    if (!strcmp((*yval)->u.object.keys[i], "__class"))
    {
      if ((*yval)->u.object.values[i]->type != yajl_t_string)
      {
        format_error = 1;
        break;
      }
      class = (*yval)->u.object.values[i]->u.string;
      continue;
    }
  }

  /*find body class*/
  if (*body)
  {
    for (i = 0; i < (*body)->u.object.len; i++)
    {
      if (!strcmp((*body)->u.object.keys[i], "__class"))
      {
        if ((*body)->u.object.values[i]->type != yajl_t_string)
        {
          format_error = 1;
          break;
        }
        *body_class = (*body)->u.object.values[i]->u.string;
        break;
      }
    }
  }

  /*valid object format*/
  if (format_error || class == NULL || *path == NULL || *body == NULL || strcmp(class, "AWT-DA::Message"))
  {
    SWI_LOG("AV", ERROR, "%s: Invalid payload received from RA, object content invalid\n", __FUNCTION__);
    return RC_BAD_FORMAT;
  }

  return RC_OK;
}
/*
 * This handler needs to free incoming data.
 * This handler is executed in a new thread!
 */
rc_ReturnCode_t empSendDataHdlr(uint32_t payloadsize, char* payload)
{
  SWI_LOG("AV", DEBUG, "%s\n", __FUNCTION__);

  rc_ReturnCode_t res = RC_OK;
  yajl_val yval = NULL, body = NULL;
  swi_dset_Iterator_t* user_set = NULL;
  swi_av_Asset_t* asset = NULL;
  int64_t addr = 0;
  int ticket_id = 0;
  char* path = NULL, *json_payload = NULL, *asset_id = NULL, *body_class = NULL;
  char* remaining_path = NULL; /*without asset_id*/

  json_payload = strndup(payload, payloadsize);
  YAJL_TREE_PARSE(yval, json_payload);
  SWI_LOG("AV", DEBUG, "%s: json_payload = %s\n", __FUNCTION__, json_payload);
  emp_freemessage(payload);
  free(json_payload);

  if (yval->type != yajl_t_object)
  {
    SWI_LOG("AV", ERROR, "%s: Invalid payload received from RA, expected object type, got type=%u\n",
        __FUNCTION__, yval->type);
    return res = RC_BAD_FORMAT;
  }

  /*Get values in message*/
  res = readMessage(&yval, &body, &body_class, &path, &ticket_id);
  if (res != RC_OK)
  {
    SWI_LOG("AV", DEBUG, "%s: readMessage failed %d\n", __FUNCTION__, res);
    return res;
  }

  /*get asset_id and remaining_path from path*/
  CHECK_RETURN(get_path_element(1, path, &remaining_path, &asset_id));

  pthread_mutex_lock(&assets_lock);
  CHECK_RETURN(swi_dset_GetIntegerByName(assetList, asset_id, &addr));
  pthread_mutex_unlock(&assets_lock);
  asset = (swi_av_Asset_t *) (intptr_t) addr;
  free(asset_id);

  SWI_LOG("AV", DEBUG, "readMessage: asset found: %s\n", asset->assetId);

  if (asset->dwCb)
  {
    res = swi_dset_Create(&user_set);
    if (res != RC_OK)
    {
      SWI_LOG("AV", ERROR, "%s: can't create dset to forward parameters field to user callback\n", __FUNCTION__);
      return res;
    }

    /*full body format validation will be done for each body class*/

    if (body_class == NULL )
      res = processDataWriting(body, user_set); /*data writing case*/

    else if (!strcmp(body_class, "AWT-DA::Command"))
      res = processCommand(body, user_set, &remaining_path); /*Command case!*/

    else if (!strcmp(body_class, "AWT-DA::Response"))
      res = processResponse(body); /*Response case!*/

    else /*unsupported type!!*/
    {
      SWI_LOG("AV", ERROR, "%s: Invalid payload received from RA, object body class invalid\n", __FUNCTION__);
      return res = RC_BAD_FORMAT;
    }

    asset->dwCb(asset, remaining_path, user_set, ticket_id, asset->dwCbUd);
    swi_dset_Destroy(user_set);
  }

  free(remaining_path);
  yajl_tree_free(yval);
  return res;
}

/*
 * This handler needs to free incoming data.
 * This handler is executed in a new thread!
 */
rc_ReturnCode_t empUpdateNotifHdlr(uint32_t payloadsize, char* payload)
{
  SWI_LOG("AV", DEBUG, "%s\n", __FUNCTION__);

  char *componentName = NULL, *componentVersion = NULL, *componentFile = NULL, *jsonPayload = NULL;
  char* asset_id = NULL, *remaining_path = NULL;
  swi_av_Asset_t* asset = NULL;
  swi_dset_Iterator_t* parameters_set = NULL;
  rc_ReturnCode_t res = RC_OK;
  int64_t addr = 0;
  yajl_val yval;
  uint8_t condition;

  jsonPayload = strndup(payload, payloadsize);
  YAJL_TREE_PARSE(yval, jsonPayload);
  emp_freemessage(payload);
  free(jsonPayload);

  if (yval->type != yajl_t_array)
  {
    SWI_LOG("AV", ERROR, "%s: Invalid payload received from RA, expected array got type=%u\n",
        __FUNCTION__, yval->type);
    return res = RC_BAD_FORMAT;
  }

  if (yval->u.array.len < 3)
  {
    SWI_LOG("AV", ERROR, "%s: Invalid array received from RA, expected an array of at least 3 elements\n",
        __FUNCTION__);
    return res = RC_BAD_FORMAT;
  }

  condition = (yval->u.array.values[0]->type == yval->u.array.values[1]->type)
      && (yval->u.array.values[1]->type == yval->u.array.values[2]->type)
      && (yval->u.array.values[2]->type == yajl_t_string);
  if (condition == 0)
  {
    SWI_LOG("AV", ERROR, "%s: Invalid array from RA, expected an array of strings\n", __FUNCTION__);
    return res = RC_BAD_FORMAT;
  }
  componentName = yval->u.array.values[0]->u.string;
  componentVersion = yval->u.array.values[1]->u.string;
  componentFile = yval->u.array.values[2]->u.string;

  /*get asset_id and remaining_path from path*/
  CHECK_RETURN(get_path_element(1, componentName, &remaining_path, &asset_id));

  pthread_mutex_lock(&assets_lock);
  CHECK_RETURN(swi_dset_GetIntegerByName(assetList, asset_id, &addr));
  pthread_mutex_unlock(&assets_lock);
  asset = (swi_av_Asset_t *) (intptr_t) addr;

  if (asset->updCb)
  {
    res = swi_dset_Create(&parameters_set);
    if (res != RC_OK)
    {
      SWI_LOG("AV", ERROR, "%s: can't create dset to forward parameters field to user callback\n", __FUNCTION__);
      return res;
    }

    /*process `parameters` field only if user callback is defined and only if the field parameter is given*/
    if (yval->u.array.len > 3 && yval->u.array.values[3]->type != yajl_t_null)
    {
      yajl_val yval_params = yval->u.array.values[3];

      /*process `parameters` field only if it is object type (a map) or an array (a list)*/
      if (yval_params->type == yajl_t_object || yval_params->type == yajl_t_array)
        res = process(yval_params, parameters_set);

      else
      {
        SWI_LOG("AV", WARNING, "%s: can't not identify the parameter field to forward to user callback\n",
            __FUNCTION__);
        parameters_set = NULL;
      }
    }

    /*finally call user callback!*/
    asset->updCb(asset, remaining_path, componentVersion, componentFile, parameters_set, asset->updCbUd);
    swi_dset_Destroy(parameters_set);
  }

  free(asset_id);
  free(remaining_path);
  yajl_tree_free(yval);
  return res;
}
