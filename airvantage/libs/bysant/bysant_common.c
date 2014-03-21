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
#include "bysant.h"

/* some shared data definitions */

const bs_coll_encoding_t BS_GLOBAL_LIST = {
    0x2a, 0x40, 0x35,                    // {empty,var_typed,var_untypes}_opcode
    BS_GSC_MAX, 0x36, 0x2b,         // small_{limit,typed_opcode,untyped_opcode}
    0x3f, 0x34,                                  // long_{typed,untypes}_opcode
    BS_FLIST, BS_FZLIST                                 // {fixed,variable}_kind
};
const bs_coll_encoding_t BS_GLOBAL_MAP = {
    0x41, 0x57, 0x4c,                    // {empty,var_typed,var_untypes}_opcode
    BS_GSC_MAX, 0x4d, 0x42,         // small_{limit,typed_opcode,untyped_opcode}
    0x56, 0x4b,                                   // long_{typed,untypes}_opcode
    BS_FMAP, BS_FZMAP                                   // {fixed,variable}_kind
};
const bs_coll_encoding_t BS_LISTMAP_LIST = {
    0x01, 0x7d, 0x3f,                    // {empty,var_typed,var_untypes}_opcode
    BS_LMSC_MAX, 0x40, 0x02,        // small_{limit,typed_opcode,untyped_opcode}
    0x7c, 0x3e,                                   // long_{typed,untypes}_opcode
    BS_FLIST, BS_FZLIST                                 // {fixed,variable}_kind
};
const bs_coll_encoding_t BS_LISTMAP_MAP = {
    0x83, 0xff, 0xc1,                    // {empty,var_typed,var_untypes}_opcode
    BS_LMSC_MAX, 0xc2, 0x84,        // small_{limit,typed_opcode,untyped_opcode}
    0xfe, 0xc0,                                   // long_{typed,untypes}_opcode
    BS_FMAP, BS_FZMAP                                   // {fixed,variable}_kind
};

const bs_string_encoding_t BS_GLOBAL_STRING = {
    32, 0x03,    /* small   */
    1056, 0x24,  /* medium  */
    66592, 0x28, /* large   */
    0x29,        /* chunked */
};
const bs_string_encoding_t BS_UIS_STRING = {
    47, 0x01,    /* small   */
    2095, 0x31,  /* medium  */
    67631, 0x39, /* large   */
    0x3A,        /* chunked */
};

const bs_integer_encoding_t BS_GLOBAL_INTEGER = {
    BS_GTI_MIN, BS_GTI_MAX, 0x9f,                  // tiny_{min,max,zero_opcode}
    BS_GSI_MIN, BS_GSI_MAX, 0xe8, 0xe0, // small_{min,max,neg_opcode,pos_opcode}
    BS_GMI_MIN, BS_GMI_MAX, 0xf4, 0xf0,// medium_{min,max,neg_opcode,pos_opcode}
    BS_GLI_MIN, BS_GLI_MAX, 0xfa, 0xf8, // large_{min,max,neg_opcode,pos_opcode}
    0xfb,                                               // last_large_neg_opcode
    0xfc, 0xfd                                              // int{32,64}_opcode
};

const bs_integer_encoding_t BS_NUMBER_INTEGER = {
    BS_NTI_MIN, BS_NTI_MAX, 0x62,                  // tiny_{min,max,zero_opcode}
    BS_NSI_MIN, BS_NSI_MAX, 0xd4, 0xc4, // small_{min,max,neg_opcode,pos_opcode}
    BS_NMI_MIN, BS_NMI_MAX, 0xec, 0xe4,// medium_{min,max,neg_opcode,pos_opcode}
    BS_NLI_MIN, BS_NLI_MAX, 0xf8, 0xf4, // large_{min,max,neg_opcode,pos_opcode}
    0xfb,                                               // last_large_neg_opcode
    0xfc, 0xfd                                              // int{32,64}_opcode
};
