/*
 * Copyright 2014, Kevin Greenan, Tushar Gohad, All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
 * THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * liberasurecode proprocssing helpers header
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#ifndef _ERASURECODE_HELPERS_H_
#define _ERASURECODE_HELPERS_H_

#include "erasurecode_stdinc.h"

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#define talloc(type, num)   (type *) malloc(sizeof(type) * (num))

/* Determine if an address is aligned to a particular boundary */
static inline
int is_addr_aligned(unsigned long addr, int align)
{
    return (addr & (align - 1)) == 0;
}

/*
 * Convert an int list into a bitmap
 * Assume the list is '-1' terminated.
 */
static inline
unsigned long long convert_list_to_bitmap(int *list)
{
    int i = 0;
    unsigned long long bm = 0;

    while (list[i] > -1) {
        /*
         * TODO: Assert list[i] < 64
         */
        bm |= (1 << list[i]);
        i++;
    }

    return bm;
}

/*
 * Convert an index list int list into a bitmap
 * is_idx_in_erasure[] needs to be allocated by the caller
 * @returns number of idxs in error
 */
static inline
int convert_idx_list_to_bitvalues(
        int *list_idxs,         // input idx_list
        int *is_idx_in_erasure, // output idx list as boolean values (1/0)
        int num_idxs)           // total number of indexes
{
    int i = 0, n = 0;

    for (i = 0; i < num_idxs; i++)
        is_idx_in_erasure[i] = 0;
    for (i = 0, n = 0; (list_idxs[i] > -1) && (n < num_idxs); i++, n++)
        is_idx_in_erasure[list_idxs[i]] = 1;

    return n;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

void *alloc_zeroed_buffer(int size);
void *alloc_and_set_buffer(int size, int value);
void *check_and_free_buffer(void *buf);
void *get_aligned_buffer16(int size);

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#ifndef bswap_32
static __inline uint32_t __libec_bswap_32(uint32_t __x)
{
    return __x>>24 | (__x>>8&0xff00) | (__x<<8&0xff0000) | __x<<24;
}
#define bswap_32(x) __libec_bswap_32(x)
#endif

#ifndef bswap_64
static __inline uint64_t __libec_bswap_64(uint64_t __x)
{
    return (__libec_bswap_32(__x)+0ULL)<<32 | __libec_bswap_32(__x>>32);
}
#define bswap_64(x) __libec_bswap_64(x)
#endif

#endif  // _ERASURECODE_HELPERS_H_

