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
#include "rc_endian.h"
//#include <stdio.h>

//static void pprint(unsigned char *p, size_t n) {
//  int i;
//  printf("\r\n");
//  for (i = 0; i < n; i++) {
//      printf("[%02X]", p[i]);
//  }
//  printf("\r\n");
//}

void check_endian(SEndian* sendian) {
    if (sendian == NULL) {
        return;
    }
    unsigned char* p;

    int16_t lint16 = 1;
    p = (unsigned char*)&lint16;
    //pprint(p, sizeof(int16_t));
    if (p[0] == 0x01) {
        sendian->int16_ = TLITTLE_ENDIAN;
    } else if (p[1] == 0x01) {
        sendian->int16_ = TBIG_ENDIAN;
    } else if (p[sizeof(int16_t)/2 - 1] == 0x01) {
        sendian->int16_ = TMIDDLE_BIG_ENDIAN;
    } else if (p[sizeof(int16_t)/2] == 0x01) {
        sendian->int16_ = TMIDDLE_LITTLE_ENDIAN;
    } else {
        // warning exotic endianness
        // printf("\r\nint warning exotic endianness");
        sendian->int16_ = TLITTLE_ENDIAN;
    }

    int32_t lint32 = 1;
    p = (unsigned char*)&lint32;
    //pprint(p, sizeof(int32_t));
    if (p[0] == 0x01) {
        sendian->int32_ = TLITTLE_ENDIAN;
    } else if (p[sizeof(int32_t) - 1] == 0x01) {
        sendian->int32_ = TBIG_ENDIAN;
    } else if (p[sizeof(int32_t)/2 - 1] == 0x01) {
        sendian->int32_ = TMIDDLE_BIG_ENDIAN;
    } else if (p[sizeof(int32_t)/2] == 0x01) {
        sendian->int32_ = TMIDDLE_LITTLE_ENDIAN;
    } else {
        // warning exotic endianness
        // printf("\r\nint warning exotic endianness");
        sendian->int32_ = TLITTLE_ENDIAN;
    }

    int64_t lint64 = 1;
    p = (unsigned char*)&lint64;
    //pprint(p, sizeof(int64_t));
    if (p[0] == 0x01) {
        sendian->int64_ = TLITTLE_ENDIAN;
    } else if (p[sizeof(int64_t) - 1] == 0x01) {
        sendian->int64_ = TBIG_ENDIAN;
    } else if (p[sizeof(int64_t)/2 - 1] == 0x01) {
        sendian->int64_ = TMIDDLE_BIG_ENDIAN;
    } else if (p[sizeof(int64_t)/2] == 0x01) {
        sendian->int64_ = TMIDDLE_LITTLE_ENDIAN;
    } else {
        // warning exotic endianness
        // printf("\r\nint warning exotic endianness");
        sendian->int64_ = TLITTLE_ENDIAN;
    }

    float lfloat = 1;
    p = (unsigned char*)&lfloat;
    //pprint(p, sizeof(float));
    if (p[sizeof(float) - 1] == 0x3F && p[sizeof(float) - 2] == 0xF0) {
        sendian->float_ = TLITTLE_ENDIAN;
    } else if (p[0] == 0x3F && p[1] == 0xF0 ) {
        sendian->float_ = TBIG_ENDIAN;
    } else if (p[sizeof(float)/2] == 0x3F && p[sizeof(float)/2 + 1] == 0xF0) {
        sendian->float_ = TMIDDLE_BIG_ENDIAN;
    } else if (p[sizeof(float)/2 - 1] == 0x3F && p[sizeof(float)/2 - 2] == 0xF0) {
        sendian->float_ = TMIDDLE_LITTLE_ENDIAN;
    } else {
        // warning exotic endianness
        // printf("\r\ndouble warning exotic endianness");
        sendian->float_ = TLITTLE_ENDIAN;
    }

    double ldouble = 1;
    p = (unsigned char*)&ldouble;
    //pprint(p, sizeof(double));
    if (p[sizeof(double) - 1] == 0x3F && p[sizeof(double) - 2] == 0xF0) {
        sendian->double_ = TLITTLE_ENDIAN;
    } else if (p[0] == 0x3F && p[1] == 0xF0 ) {
        sendian->double_ = TBIG_ENDIAN;
    } else if (p[sizeof(double)/2] == 0x3F && p[sizeof(double)/2 + 1] == 0xF0) {
        sendian->double_ = TMIDDLE_BIG_ENDIAN;
    } else if (p[sizeof(double)/2 - 1] == 0x3F && p[sizeof(double)/2 - 2] == 0xF0) {
        sendian->double_ = TMIDDLE_LITTLE_ENDIAN;
    } else {
        // warning exotic endianness
        // printf("\r\ndouble warning exotic endianness");
        sendian->double_ = TLITTLE_ENDIAN;
    }
}

void hton(void* value, size_t value_size, int value_endianness) {
    char *a = value;
    int i, j;
    size_t mid = value_size/2;
    switch (value_endianness) {
    case TLITTLE_ENDIAN:
        for (i = 0, j = value_size-1, value_size = mid; value_size--; i++, j--) {
            char t = a[i]; a[i] = a[j]; a[j] = t;
        }
        break;
    case TMIDDLE_BIG_ENDIAN:
        for (i = 0, j = mid, value_size = mid; value_size--; i++, j++) {
            char t = a[i]; a[i] = a[j]; a[j] = t;
        }
        break;
    case TMIDDLE_LITTLE_ENDIAN:
        for (i = 0, j = mid-1, value_size = value_size/4; value_size--; i++, j--) {
            char t = a[i]; a[i] = a[j]; a[j] = t;
            t = a[i+mid]; a[i+mid] = a[j+mid]; a[j+mid] = t;
        }
        break;
    }
}
