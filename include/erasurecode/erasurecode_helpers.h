/*
 * <Copyright>
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

#include "erasurecode_backend.h"
#include "erasurecode_stdinc.h"

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#if __STDC_VERSION__ < 199901L
    #if __GNUC__ >= 2
        #define __func__ __FUNCTION__
    #else
        #define __func__ "<unknown>"
    #endif
#endif

#define log_error(str) \
    fprintf(stderr, "%s:%d (%s): %s\n", __FILE__, __LINE__, __func__, str)

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

/**
 * liberasurecode fragment header definition
 *
 * Prevent the compiler from padding this by using the __packed__ keyword
 */
typedef struct __attribute__((__packed__)) fragment_header_s
{
    uint32_t magic;
    uint32_t idx;
    uint32_t size;
    uint32_t orig_data_size;
    // FIXME - reserve 16-bytes for md5
    uint32_t chksum;
    // We must be aligned to 16-byte boundaries
    // So, size this array accordingly
    uint32_t aligned_padding[3];
} fragment_header_t;

#define LIBERASURECODE_FRAG_HEADER_MAGIC  0xb0c5ecc

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

static inline
void init_fragment_header(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    header->magic = LIBERASURECODE_FRAG_HEADER_MAGIC;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

void *alloc_zeroed_buffer(int size);
void *check_and_free_buffer(void *buf);
char *alloc_fragment_buffer(int size);
int free_fragment_buffer(char *buf);
void *get_aligned_buffer16(int size);
int get_aligned_data_size(ec_backend_t instance, int data_len);
char *get_data_ptr_from_fragment(char *buf);
char *get_fragment_ptr_from_data_novalidate(char *buf);
char *get_fragment_ptr_from_data(char *buf);
int set_fragment_idx(char *buf, int idx);
int get_fragment_idx(char *buf);
int set_fragment_size(char *buf, int size);
int get_fragment_size(char *buf);
int set_orig_data_size(char *buf, int orig_data_size);
int get_orig_data_size(char *buf);
int validate_fragment(char *buf);

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#endif  // _ERASURECODE_HELPERS_H_

