/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/
/**
 * @file pointer_list.h
 * @brief This common library provide a simple fifo-like auto growing easy to manipulate buffer.
 * @version 0.1
 * @defgroup common common
 *
 */

#ifndef POINTER_LIST_H_
#define POINTER_LIST_H_

#include "returncodes.h"

/**
 * Private structure of the pointer list object
 */
typedef struct PointerList_s PointerList;

/**
 * Create a PointerList object.
 *
 * @param list pointer on the PointerList object pointer. This will point to the newly created object
 * @param prealloc is the optional number of pre allocated entries at creation time. If zero is given as prealloc,
 * default value (8) is used.
 * @return status code.
 */
rc_ReturnCode_t PointerList_Create(PointerList** list, unsigned int prealloc);

/**
 * Destroy a PointerList object
 *
 * @param list PointerList object to destroy.
 * @return status code.
 */
rc_ReturnCode_t PointerList_Destroy(PointerList* list);

/**
 * Get the number of entries currently hold in a PointerList object.
 *
 * @param list PointerList object to use. Can be null if not interested in that value.
 * @param nbOfElements nb of entries actually used. Can be null if not interested in that value.
 * @param allocatedSize total nb of entries that the object can hold before reallocating space.
 * @return status code.
 */
rc_ReturnCode_t PointerList_GetSize(PointerList* list, unsigned int* nbOfElements, unsigned int* allocatedSize);

/**
 * Push a pointer entry on the tail of the PointerList object
 *
 * @param list PointerList object to use.
 * @param pointer pointer to push into the object.
 * @return status code.
 */
rc_ReturnCode_t PointerList_PushLast(PointerList* list, void* pointer);

/**
 * Pop a pointer entry from the PointerList object
 *
 * @param list PointerList object to use.
 * @param pointer will point on the pointer to retrieve, or null if no more entries are available.
 * @return status code.
 */
rc_ReturnCode_t PointerList_PopFirst(PointerList* list, void** pointer);

/**
 * Overwrite the entry at the given index with the given pointer value;
 * Indexes start at 0. If the given index is out of bound, an error is returned, and nothing is written into the PointerList.
 *
 * @param list PointerList object to use.
 * @param index index of where to write the pointer value.
 * @param pointer pointer to push into the object.
 * @return status code.
 */
rc_ReturnCode_t PointerList_Poke(PointerList* list, unsigned int index, void* pointer);

/**
 * Read an entry at the given index, without popping it from the PointerList.
 * Indexes start at 0. If the given index is out of bound, an error is returned, and pointer will point to null.
 *
 * @param list PointerList object to use.
 * @param index index of where to write the pointer value.
 * @param pointer will point on the pointer to retrieve, or null if no more entries are available.
 * @return status code.
 */
rc_ReturnCode_t PointerList_Peek(PointerList* list, unsigned int index, void** pointer);


/**
 * Read and Remove an entry at the given index. The PointerList size is decremented by 1.
 * Indexes start at 0. If the given index is out of bound, an error is returned, and pointer will point to null.
 *
 * @param list PointerList object to use.
 * @param index index of where to write the pointer value.
 * @param pointer will point on the pointer to retrieve, or null if no more entries are available.
 * @return status code.
 */
rc_ReturnCode_t PointerList_Remove(PointerList* list, unsigned int index, void** pointer);


#endif /* POINTER_LIST_H_ */

