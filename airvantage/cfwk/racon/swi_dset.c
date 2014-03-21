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
#include "swi_dset.h"
#include "dset_internal.h"
#include "pointer_list.h"

#define DSET_MAGIC_ID 0xAFA1AFA1
#define CHECK_CONTEXT(ctx)  if (!ctx || ctx->magic != DSET_MAGIC_ID) return RC_BAD_FORMAT
#define CHECK_CONTEXT_N(ctx)  if (!ctx || ctx->magic != DSET_MAGIC_ID) return NULL

#define DSET_INIT_INDEX (UINT32_MAX)

struct swi_dset_Iterator
{
  unsigned int magic;
  unsigned int currentIndex;
  swi_dset_Element_t* currentElt;
  PointerList* list;
};

rc_ReturnCode_t dset_PushData(swi_dset_Iterator_t* set, const char* name, size_t nameLength, void *data, size_t dataLength, swi_dset_Type_t type)
{
  swi_dset_Element_t *t = NULL;

  CHECK_CONTEXT(set);
  if (NULL == name ||0 == nameLength)
    return RC_BAD_PARAMETER;

  t = malloc(sizeof(*t) + nameLength + 1 + dataLength);
  if (t == NULL)
    return RC_NO_MEMORY;

  t->name = (char*) (t + 1); //name space starts right after the struct space
  strncpy(t->name, name, nameLength);
  t->name[nameLength] = '\0';

  t->type = type;
  switch(type)
  {
    case SWI_DSET_INTEGER:
      t->val.ival = *(uint64_t *)data;
      break;
    case SWI_DSET_FLOAT:
      t->val.dval = *(double *)data;
      break;
    case SWI_DSET_STRING:
      t->val.sval = ((char*) (t + 1)) + nameLength + 1;
      strncpy(t->val.sval, (char *)data, dataLength - 1);
      t->val.sval[dataLength - 1] = '\0';
      break;
    case SWI_DSET_BOOL:
      t->val.bval = *(bool *)data;
      break;
    default:
      break;
  }

  return PointerList_PushLast(set->list, (void*) t);
}

/* Internal functions*/

/*
 * The caller of this function is responsible to feel the data in the set
 * and then initialize the current element (by calling next)
 * */
rc_ReturnCode_t swi_dset_Create(swi_dset_Iterator_t** set)
{
  swi_dset_Iterator_t* s = malloc(sizeof(*s));
  if (NULL == s)
  {
    return RC_NO_MEMORY;
  }
  rc_ReturnCode_t res = PointerList_Create(&(s->list), 0); //using 0 for default prealloc
  if (RC_OK != res)
  {
    swi_dset_Destroy(s);
    return res;
  }
  else
  {
    s->magic = DSET_MAGIC_ID;
    s->currentIndex = DSET_INIT_INDEX; //from pointer_list.h Peek fct: "Indexes start at 0",
    //we use DSET_INIT_INDEX to identify a dset with no data ready to be used/iterated over etc.
    s->currentElt=NULL;
    *set = s;
  }

  return res;
}

rc_ReturnCode_t swi_dset_PushInteger(swi_dset_Iterator_t* set, const char* name, size_t nameLength, uint64_t val)
{
  return dset_PushData(set, name, nameLength, &val, 0, SWI_DSET_INTEGER);
}

rc_ReturnCode_t swi_dset_PushFloat(swi_dset_Iterator_t* set, const char* name, size_t nameLength, double val)
{
  return dset_PushData(set, name, nameLength, &val, 0, SWI_DSET_FLOAT);
}

rc_ReturnCode_t swi_dset_PushString(swi_dset_Iterator_t* set, const char* name, size_t nameLength, const char* val, size_t valLength)
{
  return dset_PushData(set, name, nameLength, (void *)val, valLength+1, SWI_DSET_STRING);
}

rc_ReturnCode_t swi_dset_PushNull(swi_dset_Iterator_t* set, const char* name, size_t nameLength)
{
  return dset_PushData(set, name, nameLength, NULL, 0, SWI_DSET_NIL);
}

rc_ReturnCode_t swi_dset_PushBool(swi_dset_Iterator_t* set, const char* name, size_t nameLength, bool val)
{
  return dset_PushData(set, name, nameLength, &val, 0, SWI_DSET_BOOL);
}

rc_ReturnCode_t swi_dset_PushUnsupported(swi_dset_Iterator_t *set, const char *name, size_t nameLength){
  return dset_PushData(set, name, nameLength, NULL, 0, SWI_DSET_UNSUPPORTED);
}

rc_ReturnCode_t findByName(swi_dset_Iterator_t* data, const char* namePtr, swi_dset_Element_t ** elt,
    unsigned int* eltIndex)
{
  CHECK_CONTEXT(data);
  if (NULL == namePtr || 0 == strlen(namePtr))
    return RC_BAD_PARAMETER;

  unsigned int i = 0; //don't change current elt, we could have started from current elt, no reason for now
  unsigned int size = 0;
  rc_ReturnCode_t res = PointerList_GetSize(data->list, &size, NULL );
  if (RC_OK != res)
    return res;
  swi_dset_Element_t * e = NULL;
  for (i = 0; i < size; i++)
  {
    PointerList_Peek(data->list, i, (void**) &e);
    if (e)
    {
      if (!strcmp(e->name, namePtr))
      { //we found a match!
        if (elt)
          *elt = e;
        if (eltIndex)
          *eltIndex = i;
        return RC_OK;
      }
      //name doesn't match, discard output from pointer list
      e = NULL;
    }
    //Peek "returned" NULL, not good...
  }
  //if we get here, it means we found no match during iteration
  return RC_NOT_FOUND;
}

/*
 * This reset current element of dset!
 * */
rc_ReturnCode_t swi_dset_RemoveByName(swi_dset_Iterator_t* data, const char* namePtr, swi_dset_Element_t ** elt)
{
  unsigned int index = 0;

  rc_ReturnCode_t res = findByName(data, namePtr, NULL, &index);
  if (RC_OK != res)
    return res;
  //reset current element has the content is modified.
  //very likely to be used when dset is used as a map (only).
  data->currentElt = NULL;
  data->currentIndex = DSET_INIT_INDEX;

  swi_dset_Element_t * tmp = NULL;
  res = PointerList_Remove(data->list, index, (void**)&tmp);
  if (elt)
    *elt = tmp;
  else
    free(tmp);
  return res;
}

/* Public API*/

rc_ReturnCode_t swi_dset_Destroy(swi_dset_Iterator_t* data)
{
  CHECK_CONTEXT(data);
  data->magic = ~data->magic;
  rc_ReturnCode_t res = RC_OK;

  //free each swi_dset_Element_t!
  unsigned int i = 0;
  unsigned int size = 0;
  res = PointerList_GetSize(data->list, &size, NULL );
  if (res != RC_OK)
    return res;
  swi_dset_Element_t * elt = NULL;
  for (i = 0; i < size; i++)
  {
    PointerList_Peek(data->list, i, (void**) &elt);
    if (elt)
    {
      free(elt); //only one alloc per swi_dset_Element_t, to be sync with functions creating swi_dset_Element_t.
      elt = NULL;
    }
  }

  res = PointerList_Destroy(data->list);
  free(data);
  return res;
}

rc_ReturnCode_t swi_dset_Next(swi_dset_Iterator_t* data)
{
  CHECK_CONTEXT(data);
  unsigned int nbElts = 0;
  unsigned int nextIndex = 0;
  rc_ReturnCode_t res = PointerList_GetSize(data->list, &nbElts, NULL );
  if (RC_OK != res)
    return RC_UNSPECIFIED_ERROR;

  if (data->currentIndex == DSET_INIT_INDEX)
    nextIndex = 0;
  else
    nextIndex = data->currentIndex + 1;

  if (nextIndex >= nbElts)
    return RC_NOT_FOUND;
  else
  {
    res = PointerList_Peek(data->list, nextIndex, (void**) &(data->currentElt));
    if (RC_OK != res)
      return res;
    if (NULL == data->currentElt)
      return RC_UNSPECIFIED_ERROR;

    data->currentIndex = nextIndex;
    return RC_OK;
  }
}

rc_ReturnCode_t swi_dset_Rewind(swi_dset_Iterator_t* data)
{
  CHECK_CONTEXT(data);

  data->currentIndex = DSET_INIT_INDEX;
  return RC_OK;
}

const char* swi_dset_GetName(swi_dset_Iterator_t* data)
{
  CHECK_CONTEXT_N(data);
  if (NULL == data->currentElt)
    return NULL ;
  return data->currentElt->name;
}

swi_dset_Type_t swi_dset_GetType(swi_dset_Iterator_t* data)
{
  if (!data || data->magic != DSET_MAGIC_ID || NULL == data->currentElt)
    return SWI_DSET_UNSUPPORTED;
  return data->currentElt->type;
}

int64_t swi_dset_ToInteger(swi_dset_Iterator_t* data)
{
  if (!data || data->magic != DSET_MAGIC_ID || NULL == data->currentElt || SWI_DSET_INTEGER != data->currentElt->type)
    return 0;
  else
    return data->currentElt->val.ival;
}

double swi_dset_ToFloat(swi_dset_Iterator_t* data)
{
  if (!data || data->magic != DSET_MAGIC_ID || NULL == data->currentElt || SWI_DSET_FLOAT != data->currentElt->type)
    return 0;
  return data->currentElt->val.dval;
}

const char* swi_dset_ToString(swi_dset_Iterator_t* data)
{
  if (!data || data->magic != DSET_MAGIC_ID || NULL == data->currentElt || SWI_DSET_STRING != data->currentElt->type)
    return NULL;
  return data->currentElt->val.sval;
}

bool swi_dset_ToBool(swi_dset_Iterator_t* data)
{
   if (!data || data->magic != DSET_MAGIC_ID || NULL == data->currentElt || SWI_DSET_BOOL != data->currentElt->type)
    return false;
   return data->currentElt->val.bval;
}

/*
 * Internal function to factorize implementation of swi_dset_Get*ByName fcts
 *
 * Implementation detail: first match by name is used.
 * todo: define if (and how and where) to protect against identical keys in map.
 *
 * */
rc_ReturnCode_t getValByName(swi_dset_Iterator_t* data, //< as in  swi_dset_Get*ByName fcts
    const char* namePtr, //< as in  swi_dset_Get*ByName fcts
    void* valuePtr, //< casted using typeVal value and affected to element value.
    swi_dset_Type_t typeValue //< used to return value (in valuePtr), get is depending on real type and expected one.
    )
{
  CHECK_CONTEXT(data);
  if (NULL == namePtr || 0 == strlen(namePtr) || NULL == valuePtr)
    return RC_BAD_PARAMETER;
  swi_dset_Element_t* elt = NULL;
  rc_ReturnCode_t res = findByName(data, namePtr, &elt, NULL );
  if (RC_OK != res)
    return res;
  if(elt == NULL)
    return RC_UNSPECIFIED_ERROR;

  //before casting, check type is expected one!
  if (elt->type != typeValue)
    return RC_BAD_PARAMETER;

  switch (typeValue)
  {
    case SWI_DSET_INTEGER:
    {
      *((uint64_t*) valuePtr) = elt->val.ival;
      break;
    }
    case SWI_DSET_FLOAT:
    {
      *((double*) valuePtr) = elt->val.dval;
      break;
    }
    case SWI_DSET_STRING:
    {
      *((char**) valuePtr) = elt->val.sval;
      break;
    }
    default:
      //nil and unsupported types go here
      //very unlikely to here given previous type comparison.
      return RC_BAD_FORMAT;
  }

  return RC_OK;
}

rc_ReturnCode_t swi_dset_GetIntegerByName(swi_dset_Iterator_t* data, const char* namePtr, int64_t* valuePtr)
{
  return getValByName(data, namePtr, (void*) valuePtr, SWI_DSET_INTEGER);
}

rc_ReturnCode_t swi_dset_GetFloatByName(swi_dset_Iterator_t* data, const char* namePtr, double* valuePtr)
{
  return getValByName(data, namePtr, (void*) valuePtr, SWI_DSET_FLOAT);
}

rc_ReturnCode_t swi_dset_GetStringByName(swi_dset_Iterator_t* data, const char* namePtr, const char** valuePtr)
{
  return getValByName(data, namePtr, (void*) valuePtr, SWI_DSET_STRING);
}

rc_ReturnCode_t swi_dset_GetTypeByName(swi_dset_Iterator_t* data, const char* namePtr, swi_dset_Type_t* typePtr)
{
  CHECK_CONTEXT(data);
  if (NULL == namePtr || 0 == strlen(namePtr))
    return RC_BAD_PARAMETER;

  swi_dset_Element_t* elt = NULL;
  rc_ReturnCode_t res = findByName(data, namePtr, &elt, NULL );
  if (RC_OK != res)
    return res;

  *typePtr = elt->type;
  return RC_OK;
}

