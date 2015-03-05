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

/**
 * liberasurecode fragment header definition
 *
 * Prevent the compiler from padding this by using the __packed__ keyword
 */

#define LIBERASURECODE_FRAG_HEADER_MAGIC    0xb0c5ecc
#define LIBERASURECODE_MAX_CHECKSUM_LEN     8   /* quad words */

typedef struct __attribute__((__packed__)) fragment_header_s
{
    fragment_metadata_t meta;   /* 59 bytes */
    uint32_t            magic;  /*  4 bytes */
    uint32_t            libec_version; /* 4 bytes */
    // We must be aligned to 16-byte boundaries
    // So, size this array accordingly
    uint8_t             aligned_padding[13];
} fragment_header_t;

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
void *alloc_and_set_buffer(int size, int value);
void *check_and_free_buffer(void *buf);
char *alloc_fragment_buffer(int size);
int free_fragment_buffer(char *buf);
void *get_aligned_buffer16(int size);
int get_aligned_data_size(ec_backend_t instance, int data_len);
char *get_data_ptr_from_fragment(char *buf);
int get_data_ptr_array_from_fragments(char **data_array, char **fragments,
        int num_fragments);
int get_fragment_ptr_array_from_data(char **frag_array, char **data,
        int num_data);
char *get_fragment_ptr_from_data_novalidate(char *buf);
char *get_fragment_ptr_from_data(char *buf);
uint64_t get_fragment_size(char *buf);
int set_fragment_idx(char *buf, int idx);
int get_fragment_idx(char *buf);
int set_fragment_payload_size(char *buf, int size);
int get_fragment_payload_size(char *buf);
int set_fragment_backend_metadata_size(char *buf, int size);
int get_fragment_backend_metadata_size(char *buf);
int get_fragment_buffer_size(char *buf);
int set_orig_data_size(char *buf, int orig_data_size);
int get_orig_data_size(char *buf);
int set_checksum(ec_checksum_type_t ct, char *buf, int blocksize);
int get_checksum(char *buf); //TODO implement this
int set_libec_version(char *fragment);
int get_libec_version(char *fragment, uint32_t *ver);
int set_backend_id(char *buf, ec_backend_id_t id);
int get_backend_id(char *buf, ec_backend_id_t *id);
int set_backend_version(char *buf, uint32_t version);
int get_backend_version(char *buf, uint32_t *version);

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#endif  // _ERASURECODE_HELPERS_H_

