/*
 * Copyright 2014 Tushar Gohad, Kevin M Greenan, Eric Lambert
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
 * liberasurecode API helpers implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"
#include "erasurecode_stdinc.h"
#include "erasurecode_version.h"

#include "alg_sig.h"
#include "erasurecode_log.h"

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

static bool is_fragment(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic == LIBERASURECODE_FRAG_HEADER_MAGIC) {
        return true;
    }

    return false;
}

/**
 * Memory Management Methods
 * 
 * The following methods provide wrappers for allocating and deallocating
 * memory.  
 */
void *get_aligned_buffer16(int size)
{
    void *buf;

    /**
     * Ensure all memory is aligned to 16-byte boundaries
     * to support 128-bit operations
     */
    if (posix_memalign(&buf, 16, size) != 0) {
        return NULL;
    }

    memset(buf, 0, size);

    return buf;
}

/**
 * Allocate a zero-ed buffer of a specific size.
 *
 * @param size integer size in bytes of buffer to allocate
 * @return pointer to start of allocated buffer or NULL on error
 */
void * alloc_zeroed_buffer(int size)
{
    return alloc_and_set_buffer(size, 0);
}

/**
 * Allocate a buffer of a specific size and set its' contents
 * to the specified value.
 *
 * @param size integer size in bytes of buffer to allocate
 * @param value
 * @return pointer to start of allocated buffer or NULL on error
 */
void * alloc_and_set_buffer(int size, int value) {
    void * buf = NULL;  /* buffer to allocate and return */
  
    /* Allocate and zero the buffer, or set the appropriate error */
    buf = malloc((size_t) size);
    if (buf) {
        buf = memset(buf, value, (size_t) size);
    }
    return buf;
}

/**
 * Deallocate memory buffer if it's not NULL.  This methods returns NULL so 
 * that you can free and reset a buffer using a single line as follows:
 *
 * my_ptr = check_and_free_buffer(my_ptr);
 *
 * @return NULL
 */
void * check_and_free_buffer(void * buf)
{
    if (buf)
        free(buf);
    return NULL;
}

char *alloc_fragment_buffer(int size)
{
    char *buf;
    fragment_header_t *header = NULL;

    size += sizeof(fragment_header_t);
    buf = get_aligned_buffer16(size);

    if (buf) {
        header = (fragment_header_t *) buf;
        header->magic = LIBERASURECODE_FRAG_HEADER_MAGIC;
    }

    return buf;
}

int free_fragment_buffer(char *buf)
{
    fragment_header_t *header;

    if (NULL == buf) {
        return -1;
    }

    buf -= sizeof(fragment_header_t);

    header = (fragment_header_t *) buf;
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (free fragment)!");
        return -1;
    }

    free(buf);
    return 0;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

/**
 * Return total fragment length (on-disk, on-wire)
 *
 * @param buf - pointer to fragment buffer
 *
 * @return fragment size on disk
 */
uint64_t get_fragment_size(char *buf)
{

    if (NULL == buf)
        return -1;

    return get_fragment_buffer_size(buf) + sizeof(fragment_header_t);
 }

/**
 * Compute a size aligned to the number of data and the underlying wordsize 
 * of the EC algorithm.
 * 
 * @param instance, ec_backend_t instance (to extract args)
 * @param data_len, integer length of data in bytes
 * @return integer data length aligned with wordsize of EC algorithm
 */
int get_aligned_data_size(ec_backend_t instance, int data_len)
{
    int k = instance->args.uargs.k;
    int w = instance->args.uargs.w;
    int word_size = w / 8;
    int alignment_multiple;
    int aligned_size = 0;

    /*
     * For Cauchy reed-solomon align to k*word_size*packet_size
     * For Vandermonde reed-solomon and flat-XOR, align to k*word_size
     */
    if (EC_BACKEND_JERASURE_RS_CAUCHY == instance->common.id) {
        alignment_multiple = k * w * (sizeof(long) * 128);
    } else {
        alignment_multiple = k * word_size;
    }

    aligned_size = (int) 
        ceill((double) data_len / alignment_multiple) * alignment_multiple;

    return aligned_size;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

char *get_data_ptr_from_fragment(char *buf)
{
    buf += sizeof(fragment_header_t);

    return buf;
}

int get_data_ptr_array_from_fragments(char **data_array, char **fragments,
                                      int num_fragments)
{
    int i = 0, num = 0;
    for (i = 0; i < num_fragments; i++) {
        char *frag = fragments[i];
        if (frag == NULL) {
            data_array[i] = NULL;
            continue;
        }
        data_array[i] = get_data_ptr_from_fragment(frag);
        num++;
    }
    return num;
}

int get_fragment_ptr_array_from_data(char **frag_array, char **data,
                                     int num_data)
{
    int i = 0, num = 0;
    for (i = 0; i < num_data; i++) {
        char *data_ptr = frag_array[i];
        if (data_ptr == NULL) {
            data[i] = NULL;
            continue;
        }
        data[i] = get_fragment_ptr_from_data(data_ptr);
        num++;
    }
    return num;
}

char *get_fragment_ptr_from_data_novalidate(char *buf)
{
    buf -= sizeof(fragment_header_t);

    return buf;
}

char *get_fragment_ptr_from_data(char *buf)
{
    fragment_header_t *header;

    buf -= sizeof(fragment_header_t);

    header = (fragment_header_t *) buf;

    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get header ptr)!\n");
        return NULL;
    }

    return buf;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

int set_fragment_idx(char *buf, int idx)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (idx check)!\n");
        return -1;
    }

    header->meta.idx = idx;

    return 0;
}

int get_fragment_idx(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get idx)!");
        return -1;
    }

    return header->meta.idx;
}

int set_fragment_payload_size(char *buf, int size)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (size check)!");
        return -1;
    }

    header->meta.size = size;

    return 0;
}

int get_fragment_payload_size(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get size)!");
        return -1;
    }

    return header->meta.size;
}

int set_fragment_backend_metadata_size(char *buf, int size)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (set fragment backend metadata size)!");
        return -1;
    }

    header->meta.frag_backend_metadata_size = size;

    return 0;
}

int get_fragment_backend_metadata_size(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get fragment backend metadata size)!");
        return -1;
    }

    return header->meta.frag_backend_metadata_size;
}

int get_fragment_buffer_size(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get size)!");
        return -1;
    }

    return header->meta.size + header->meta.frag_backend_metadata_size;
}

int set_orig_data_size(char *buf, int orig_data_size)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (set orig data check)!");
        return -1;
    }

    header->meta.orig_data_size = orig_data_size;

    return 0;
}

int get_orig_data_size(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get orig data check)!");
        return -1;
    }

    return header->meta.orig_data_size;
}

int set_libec_version(char *buf)
{
    if (!is_fragment(buf)) {
            return -1;
    }
    fragment_header_t *header = (fragment_header_t *) buf;
    header->libec_version = (uint32_t)LIBERASURECODE_VERSION;
    return 0;
}

int get_libec_version(char *buf, uint32_t *ver)
{
    if (!is_fragment(buf)) {
            return -1;
    }
    fragment_header_t *header = (fragment_header_t *) buf;
    *ver = header->libec_version;
    return 0;
}

int set_backend_id(char *buf, ec_backend_id_t id)
{
    if (!is_fragment(buf)) {
            return -1;
    }
    fragment_header_t *header = (fragment_header_t *) buf;
    header->meta.backend_id = (uint8_t)id;
    return 0;
}

int get_backend_id(char *buf, ec_backend_id_t *id) 
{
    if (!is_fragment(buf)) {
            return -1;
    }
    fragment_header_t *header = (fragment_header_t *) buf;
    *id = header->meta.backend_id;
    return 0;
}

int set_backend_version(char *buf, uint32_t version) 
{
    if (!is_fragment(buf)) {
            return -1;
    }
    fragment_header_t *header = (fragment_header_t *) buf;
    header->meta.backend_version = version;
    return 0;
}

int get_backend_version(char *buf, uint32_t *version) 
{
    if (!is_fragment(buf)) {
            return -1;
    }
    fragment_header_t *header = (fragment_header_t *) buf;
    *version = header->meta.backend_version;
    return 0;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

inline int set_checksum(ec_checksum_type_t ct, char *buf, int blocksize)
{
    fragment_header_t* header = (fragment_header_t*) buf;
    char *data = get_data_ptr_from_fragment(buf);

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (set chksum)!\n");
        return -1; 
    }

    header->meta.chksum_type = ct;
    header->meta.chksum_mismatch = 0;

    switch(header->meta.chksum_type) {
        case CHKSUM_CRC32:
            header->meta.chksum[0] = crc32(0, data, blocksize);
            break;
        case CHKSUM_MD5:
            break;
        case CHKSUM_NONE:
        default:
            break;
    }
    
    return 0;
}

inline uint32_t* get_chksum(char *buf)
{
    fragment_header_t* header = (fragment_header_t*) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get chksum)!");
        return NULL;
    }

    return (uint32_t *) header->meta.chksum;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#if LIBERASURECODE_VERSION >= _VERSION(1,2,0)
inline int set_metadata_chksum(char *buf)
{
    fragment_header_t* header = (fragment_header_t*) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (set meta chksum)!\n");
        return -1;
    }

    header->metadata_chksum = crc32(0, &header->meta,
                                    sizeof(fragment_metadata_t));
    return 0;
}

inline uint32_t* get_metadata_chksum(char *buf)
{
    fragment_header_t* header = (fragment_header_t*) buf;

    assert(NULL != header);
    if (header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
        log_error("Invalid fragment header (get meta chksum)!");
        return NULL;
    }

    return (uint32_t *) &header->metadata_chksum;
}
#else
inline int set_metadata_chksum(char *buf)
{
    return 0;
}

inline uint32_t* get_metadata_chksum(char *buf)
{
    return (uint32_t *) 0;
}
#endif

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

