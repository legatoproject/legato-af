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

#ifndef SWI_DSETINTERNAL_INCLUDE_GUARD
#define SWI_DSETINTERNAL_INCLUDE_GUARD

#include <stdlib.h>
#include <stdbool.h>
#include "swi_dset.h"

/*
 * swi_dset.h will contain only public API for client of other swi_* libs.
 * This header contains API to create/modify dset object in order
 * to provide swi_* libs.
 */

typedef struct swi_dset_Element{
  swi_dset_Type_t type;
  char* name;
  union u_value{
    uint64_t ival;
    double dval;
    char* sval;
    bool bval;
  } val;
}swi_dset_Element_t;

//rc_ReturnCode_t swi_dset_CreateElement(swi_dset_Element_t** elt);
//rc_ReturnCode_t swi_dset_DestroyElement(swi_dset_Element_t* elt);

rc_ReturnCode_t swi_dset_Create(swi_dset_Iterator_t** set);
rc_ReturnCode_t swi_dset_Rewind(swi_dset_Iterator_t *set);
/*
 * Push fct: to be used to populate dset with a new
 * swi_dset_Element_t malloc'ed in each fct.
 * those fcts don't change current element.
 */
rc_ReturnCode_t swi_dset_PushInteger(swi_dset_Iterator_t* set, const char* name,  size_t nameLength, uint64_t val);
rc_ReturnCode_t swi_dset_PushFloat(swi_dset_Iterator_t* set, const char* name,  size_t nameLength, double val);
rc_ReturnCode_t swi_dset_PushString(swi_dset_Iterator_t* set, const char* name, size_t nameLength, const char* val, size_t valLength);
rc_ReturnCode_t swi_dset_PushNull(swi_dset_Iterator_t *set, const char *name, size_t nameLength);
rc_ReturnCode_t swi_dset_PushBool(swi_dset_Iterator_t *set, const char *name, size_t nameLength, bool val);
rc_ReturnCode_t swi_dset_PushUnsupported(swi_dset_Iterator_t *set, const char *name, size_t nameLength);
/*
 * To remove an element
 * The current element is discarded, dset iteration will restart from scratch.
 */
rc_ReturnCode_t swi_dset_RemoveByName(swi_dset_Iterator_t* set, const char* name, swi_dset_Element_t ** elt);

#endif /* SWI_DSETINTERNAL_INCLUDE_GUARD */
