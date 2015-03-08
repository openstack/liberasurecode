/*
 * Copyright 2014 Tushar Gohad, Kevin M Greenan, Eric Lambert, Mark Storer
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
 * liberasurecode proprocssing helpers implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_preprocessing.h"
#include "erasurecode_stdinc.h"

int prepare_fragments_for_encode(ec_backend_t instance,
        int k, int m,
        const char *orig_data, uint64_t orig_data_size, /* input */
        char **encoded_data, char **encoded_parity,     /* output */
        int *blocksize)
{
    int i, ret = 0;
    int data_len;           /* data len to write to fragment headers */
    int aligned_data_len;   /* EC algorithm compatible data length */
    int buffer_size, payload_size = 0;

    /* Calculate data sizes, aligned_data_len guaranteed to be divisible by k*/
    data_len = orig_data_size;
    aligned_data_len = get_aligned_data_size(instance, orig_data_size);
    *blocksize = payload_size = (aligned_data_len / k);
    buffer_size = payload_size + instance->common.backend_metadata_size;

    for (i = 0; i < k; i++) {
        int copy_size = data_len > payload_size ? payload_size : data_len;
        char *fragment = (char *) alloc_fragment_buffer(buffer_size);
        if (NULL == fragment) {
            ret = -ENOMEM;
            goto out_error;
        }
      
        /* Copy existing data into clean, zero'd out buffer */
        encoded_data[i] = get_data_ptr_from_fragment(fragment);
      
        if (data_len > 0) {
            memcpy(encoded_data[i], orig_data, copy_size);
        }

        orig_data += copy_size;
        data_len -= copy_size;
    }

    for (i = 0; i < m; i++) {
        char *fragment = (char *) alloc_fragment_buffer(buffer_size);
        if (NULL == fragment) {
            ret = -ENOMEM;
            goto out_error;
        }

        encoded_parity[i] = get_data_ptr_from_fragment(fragment);
    }

out:
    return ret;

out_error:
    printf ("ERROR in encode\n");
    if (encoded_data) {
        for (i = 0; i < k; i++) {
            if (encoded_data[i])
                free_fragment_buffer(encoded_data[i]);
        }
        check_and_free_buffer(encoded_data);
    }

    if (encoded_parity) {
        for (i = 0; i < m; i++) {
            if (encoded_parity[i])
                free_fragment_buffer(encoded_parity[i]);
        }
        check_and_free_buffer(encoded_parity);
    }

    goto out;
}

/* 
 * Note that the caller should always check realloc_bm during success or
 * failure to free buffers allocated here.  We could free up in this function,
 * but it is internal to this library and only used in a few places.  In any
 * case, the caller has to free up in the success case, so it may as well do
 * so in the failure case.
 */
int prepare_fragments_for_decode(
        int k, int m,
        char **data, char **parity,
        int  *missing_idxs,
        int *orig_size, int *fragment_payload_size, int fragment_size,
        uint64_t *realloc_bm)
{
    int i;                          /* a counter */
    unsigned long long missing_bm;  /* bitmap form of missing indexes list */
    int orig_data_size = -1;
    int payload_size = -1;
 
    missing_bm = convert_list_to_bitmap(missing_idxs);

    /*
     * Determine if each data fragment is:
     * 1.) Alloc'd: if not, alloc new buffer (for missing fragments)
     * 2.) Aligned to 16-byte boundaries: if not, alloc a new buffer
     *     memcpy the contents and free the old buffer
     */
    for (i = 0; i < k; i++) {
        /*
         * Allocate or replace with aligned buffer if the buffer was not
         * aligned.
         * DO NOT FREE: the python GC should free the original when cleaning up
         * 'data_list'
         */
        if (NULL == data[i]) {
            data[i] = alloc_fragment_buffer(fragment_size - sizeof(fragment_header_t));
            if (NULL == data[i]) {
                log_error("Could not allocate data buffer!");
                return -1;
            }
            *realloc_bm = *realloc_bm | (1 << i);
        } else if (!is_addr_aligned((unsigned long)data[i], 16)) {
            char *tmp_buf = alloc_fragment_buffer(fragment_size - sizeof(fragment_header_t));
            if (NULL == tmp_buf) {
                log_error("Could not allocate temp buffer!");
                return -1;
            }
            memcpy(tmp_buf, data[i], fragment_size);
            data[i] = tmp_buf;
            *realloc_bm = *realloc_bm | (1 << i);
        }

        /* Need to determine the size of the original data */
       if (((missing_bm & (1 << i)) == 0) && orig_data_size < 0) {
            orig_data_size = get_orig_data_size(data[i]);
            if (orig_data_size < 0) {
                log_error("Invalid orig_data_size in fragment header!");
                return -1;
            }
            payload_size = get_fragment_payload_size(data[i]);
            if (orig_data_size < 0) {
                log_error("Invalid fragment_size in fragment header!");
                return -1;
            }
       }
    }

    /* Perform the same allocation, alignment checks on the parity fragments */
    for (i = 0; i < m; i++) {
        /*
         * Allocate or replace with aligned buffer, if the buffer was not aligned.
         * DO NOT FREE: the python GC should free the original when cleaning up 'data_list'
         */
        if (NULL == parity[i]) {
            parity[i] = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
            if (NULL == parity[i]) {
                log_error("Could not allocate parity buffer!");
                return -1;
            }
            *realloc_bm = *realloc_bm | (1 << (k + i));
        } else if (!is_addr_aligned((unsigned long)parity[i], 16)) {
            char *tmp_buf = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
            if (NULL == tmp_buf) {
                log_error("Could not allocate temp buffer!");
                return -1;
            }
            memcpy(tmp_buf, parity[i], fragment_size);
            parity[i] = tmp_buf;
            *realloc_bm = *realloc_bm | (1 << (k + i));
        }

    }

    *orig_size = orig_data_size;
    *fragment_payload_size = payload_size;

    return 0;
}

int get_fragment_partition(
        int k, int m,
        char **fragments, int num_fragments,
        char **data, char **parity, int *missing)
{
    int i = 0;
    int num_missing = 0;
    int index;

    /*
     * Set all data and parity entries to NULL
     */
    for (i = 0; i < k; i++) {
        data[i] = NULL;
    }
    for (i = 0; i < m; i++) {
        parity[i] = NULL;
    }

    /*
     * Fill in data and parity with available fragments
     */ 
    for (i = 0; i < num_fragments; i++) {
        index = get_fragment_idx(fragments[i]);
        if (index < k) {
            data[index] = fragments[i];
        } else {
            parity[index - k] = fragments[i];
        }
    }

    /*
     * Fill in missing array with missing indexes
     */
    for (i = 0; i < k; i++) {
        if (NULL == data[i]) {
            missing[num_missing] = i;
            num_missing++;
        }
    }
    for (i = 0; i < m; i++) {
        if (NULL == parity[i]) {
            missing[num_missing] = i + k;
            num_missing++;
        }
    }
    // TODO: In general, it is possible to reconstruct one or more fragments
    // when more than m fragments are missing (e.g. flat XOR codes)
    return (num_missing > m) ? -EINSUFFFRAGS : 0;
}

int fragments_to_string(int k, int m,
        char **fragments, int num_fragments,
        char **orig_payload, uint64_t *payload_len)
{
    char *internal_payload = NULL;
    char **data = NULL;
    int orig_data_size = -1;
    int i;
    int index;
    int data_size;
    int num_data = 0;
    int string_off = 0;
    int ret = -1;

    if (num_fragments < k) {
        /*
         * This is not necessarily an error condition, so *do not log here*
         * We can maybe debug log, if necessary.
         */
        goto out; 
    }

    data = (char **) get_aligned_buffer16(sizeof(char *) * k);

    if (NULL == data) {
        log_error("Could not allocate buffer for data!!");
        ret = -ENOMEM;
        goto out; 
    }
    
    for (i = 0; i < num_fragments; i++) {
        index = get_fragment_idx(fragments[i]);
        data_size = get_fragment_payload_size(fragments[i]);
        if ((index < 0) || (data_size < 0)) {
            log_error("Invalid fragment header information!");
            ret = -EINVALIDPARAMS;
            goto out;
        }

        /* Validate the original data size */
        if (orig_data_size < 0) {
            orig_data_size = get_orig_data_size(fragments[i]);
        } else {
            if (get_orig_data_size(fragments[i]) != orig_data_size) {
                log_error("Inconsistent orig_data_size in fragment header!");
                ret = -EINVALIDPARAMS;
                goto out;
            }
        }
   
        /* Skip parity fragments, put data fragments in index order */
        if (index >= k) {
            continue;
        } else {
            /* Make sure we account for duplicates */
            if (NULL != fragments[index]) {
                data[index] = fragments[i];
                num_data++;
            }
        }
    }

    /* We do not have enough data fragments to do this! */
    if (num_data != k) {
        /*
         * This is not necessarily an error condition, so *do not log here*
         * We can maybe debug log, if necessary.
         */
        goto out;
    }

    /* Create the string to return */
    internal_payload = (char *) get_aligned_buffer16(orig_data_size);
    if (NULL == internal_payload) {
        log_error("Could not allocate buffer for decoded string!");
        ret = -ENOMEM;
        goto out;
    }
    
    /* Pass the original data length back */
    *payload_len = orig_data_size;

    /* Copy fragment data into cstring (fragments should be in index order) */
    for (i = 0; i < num_data && orig_data_size > 0; i++) {
        char* fragment_data = get_data_ptr_from_fragment(data[i]);
        int fragment_size = get_fragment_payload_size(data[i]);
        int payload_size = orig_data_size > fragment_size ? fragment_size : orig_data_size;

        memcpy(internal_payload + string_off, fragment_data, payload_size);
        orig_data_size -= payload_size;
        string_off += payload_size;
    }

    /* Everything worked just fine */
    ret = 0;

out:
    if (NULL != data) {
        free(data);
    }

    *orig_payload = internal_payload;
    return ret;
}


