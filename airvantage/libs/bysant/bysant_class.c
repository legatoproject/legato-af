/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Julien Desgats for Sierra Wireless - initial API and implementation
 *******************************************************************************/
/*
 * Class collection handling
 */

#include <stdlib.h>
#include <string.h> // strcmp()
#include "bysant.h"

int bs_classcoll_init( bs_classcoll_t *coll) {
  coll->nclasses = 0;
  coll->classes = NULL;
  return 0;
}

void bs_classcoll_reset( bs_classcoll_t *coll) {
  int i;
  for( i=0; i<coll->nclasses; i++) {
    if( BS_CLASS_MANAGED == coll->classes[i]->mode) {
      free( (void*)coll->classes[i]);
    }
  }
  free( coll->classes);
  coll->classes = NULL;
}

int bs_classcoll_set( bs_classcoll_t *coll, const bs_class_t* classdef) {
  const bs_class_t* existing = bs_classcoll_get( coll, classdef->classid);
  int slot;
  if( NULL != existing) {
    /* overwrite class definition */
    slot = existing - coll->classes[0];
    if( BS_CLASS_MANAGED == existing->mode) {
      free((void *)existing);
    }
  } else {
    /* insert new class at correct place */
    int i;
    coll->classes = realloc( coll->classes, (coll->nclasses + 1) * sizeof( bs_class_t *));
    if( NULL == coll->classes) return -1;
    for( slot = 0; slot < coll->nclasses && coll->classes[slot]->classid < classdef->classid; slot++);
    for( i=coll->nclasses; i > slot; i--) coll->classes[i] = coll->classes[i-1];
    coll->nclasses += 1;
  }
  coll->classes[slot] = classdef;
  return 0;
}

static int compareClassesById( const void *key, const void *member) {
  const bs_classid_t classid = *(const bs_classid_t*) key;
  const bs_class_t *classdef = *(const bs_class_t **)member;
  return classid - classdef->classid;
}

const bs_class_t* bs_classcoll_get( bs_classcoll_t *coll, bs_classid_t classid) {
  const bs_class_t **classdef = bsearch( &classid, coll->classes, coll->nclasses, sizeof( void*), compareClassesById);
  return (NULL == classdef) ? NULL : *classdef;
}

const bs_class_t* bs_classcoll_byname( bs_classcoll_t *coll, const char *name) {
    int i;
    const bs_class_t **classes = coll->classes;
    for( i=0; i<coll->nclasses; i++) {
        const bs_class_t *cls = classes[i];
        if( ! strcmp( name, cls->classname)) {
            return cls;
        }
    }
    return NULL;
}
