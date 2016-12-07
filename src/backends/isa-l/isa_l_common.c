/*
 * Copyright 2014 Kevin M Greenan
 * Copyright 2014 Tushar Gohad
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
 * isa_l_rs_vand backend implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"
#include "isa_l_common.h"

int isa_l_encode(void *desc, char **data, char **parity,
        int blocksize)
{
    isa_l_descriptor *isa_l_desc = (isa_l_descriptor*) desc;

    unsigned char *g_tbls = NULL;
    int k = isa_l_desc->k;
    int m = isa_l_desc->m;

    // Generate g_tbls from encode matrix encode_matrix
    g_tbls = malloc(sizeof(unsigned char) * (k * m * 32));
    if (NULL == g_tbls) {
        return -1;
    }

    isa_l_desc->ec_init_tables(k, m, &isa_l_desc->matrix[k * k], g_tbls);

    /* FIXME - make ec_encode_data return a value */
    isa_l_desc->ec_encode_data(blocksize, k, m, g_tbls, (unsigned char**)data,
                               (unsigned char**)parity);
    free(g_tbls);
    return 0;
}

static unsigned char* isa_l_get_decode_matrix(int k, int m, unsigned char *encode_matrix, int *missing_idxs)
{
    int i = 0, j = 0, l = 0;
    int n = k + m;
    unsigned char *decode_matrix = malloc(sizeof(unsigned char) * k * k);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);

    while (i < k && l < n) {
        if (((1 << l) & missing_bm) == 0) {
            for (j = 0; j < k; j++) {
                decode_matrix[(k * i) + j] = encode_matrix[(k * l) + j];
            }
            i++;
        }
        l++;
    }

    if (i != k) {
        free(decode_matrix);
        decode_matrix = NULL;
    }

    return decode_matrix;
}

static int get_num_missing_elements(int *missing_idxs)
{
    int i = 0;

    while (missing_idxs[i] > -1) {
        i++;
    }

    return i;
}

static void mult_and_xor_row(unsigned char *to_row,
                             unsigned char *from_row,
                             unsigned char val,
                             int num_elems,
                             gf_mul_func gf_mul)
{
    int i;

    for (i = 0; i < num_elems; i++) {
        to_row[i] ^= gf_mul(val, from_row[i]);
    }
}

/*
 * TODO: Add in missing parity rows and adjust the inverse_rows to
 * be used for parity.
 */
static unsigned char* get_inverse_rows(int k,
                                       int m,
                                       unsigned char *decode_inverse,
                                       unsigned char* encode_matrix,
                                       int *missing_idxs,
                                       gf_mul_func gf_mul)
{
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);
    int num_missing_elements = get_num_missing_elements(missing_idxs);
    unsigned char *inverse_rows = (unsigned char*)malloc(sizeof(unsigned
                                    char*) * k * num_missing_elements);
    int i, j, l = 0;
    int n = k + m;

    if (NULL == inverse_rows) {
        return NULL;
    }

    memset(inverse_rows, 0, sizeof(unsigned
                                    char*) * k * num_missing_elements);

    /*
     * Fill in rows for missing data
     */
    for (i = 0; i < k; i++) {
        if ((1 << i) & missing_bm) {
            for (j = 0; j < k; j++) {
                inverse_rows[(l * k) + j] = decode_inverse[(i * k) + j];
            }
            l++;
        }
    }

    /*
     * Process missing parity.
     *
     * Start with an all-zero row.
     *
     * For each data element, if the data element is:
     *
     * Available: XOR the corresponding coefficient from the
     * encoding matrix.
     *
     * Unavailable: multiply corresponding coefficient with
     * the row that corresponds to the missing data in inverse_rows
     * and XOR the resulting row with this row.
     */
    for (i = k; i < n; i++) {
        // Parity is missing
        if ((1 << i) & missing_bm) {
            int d_idx_avail = 0;
            int d_idx_unavail = 0;
            for (j = 0; j < k; j++) {
                // This data is available, so we can use the encode matrix
                if (((1 << j) & missing_bm) == 0) {
                    inverse_rows[(l * k) + d_idx_avail] ^= encode_matrix[(i * k) + j];
                    d_idx_avail++;
                } else {
                    mult_and_xor_row(&inverse_rows[l * k],
                                     &inverse_rows[d_idx_unavail * k],
                                     encode_matrix[(i * k) + j],
                                     k,
                                     gf_mul);
                    d_idx_unavail++;
                }
            }
            l++;
        }
    }
    return inverse_rows;
}

int isa_l_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    isa_l_descriptor *isa_l_desc = (isa_l_descriptor*)desc;

    unsigned char *g_tbls = NULL;
    unsigned char *decode_matrix = NULL;
    unsigned char *decode_inverse = NULL;
    unsigned char *inverse_rows = NULL;
    unsigned char **decoded_elements = NULL;
    unsigned char **available_fragments = NULL;
    int k = isa_l_desc->k;
    int m = isa_l_desc->m;
    int n = k + m;
    int ret = -1;
    int i, j;

    int num_missing_elements = get_num_missing_elements(missing_idxs);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);

    decode_matrix = isa_l_get_decode_matrix(k, m, isa_l_desc->matrix, missing_idxs);

    if (NULL == decode_matrix) {
        goto out;
    }

    decode_inverse = (unsigned char*)malloc(sizeof(unsigned char) * k * k);

    if (NULL == decode_inverse) {
        goto out;
    }

    int im_ret = isa_l_desc->gf_invert_matrix(decode_matrix, decode_inverse, k);
    if (im_ret < 0) {
        goto out;
    }

    // Generate g_tbls from computed decode matrix (k x k) matrix
    g_tbls = malloc(sizeof(unsigned char) * (k * m * 32));
    if (NULL == g_tbls) {
        goto out;
    }

    inverse_rows = get_inverse_rows(k, m, decode_inverse, isa_l_desc->matrix, missing_idxs, isa_l_desc->gf_mul);

    decoded_elements = (unsigned char**)malloc(sizeof(unsigned char*)*num_missing_elements);
    if (NULL == decoded_elements) {
        goto out;
    }

    available_fragments = (unsigned char**)malloc(sizeof(unsigned char*)*k);
    if (NULL == available_fragments) {
        goto out;
    }

    j = 0;
    for (i = 0; i < n; i++) {
        if (missing_bm & (1 << i)) {
            continue;
        }
        if (j == k) {
            break;
        }
        if (i < k) {
            available_fragments[j] = (unsigned char*)data[i];
        } else {
            available_fragments[j] = (unsigned char*)parity[i-k];
        }
        j++;
    }

    // Grab pointers to memory needed for missing data fragments
    j = 0;
    for (i = 0; i < k; i++) {
        if (missing_bm & (1 << i)) {
            decoded_elements[j] = (unsigned char*)data[i];
            j++;
        }
    }
    for (i = k; i < n; i++) {
        if (missing_bm & (1 << i)) {
            decoded_elements[j] = (unsigned char*)parity[i - k];
            j++;
        }
    }

    isa_l_desc->ec_init_tables(k, num_missing_elements, inverse_rows, g_tbls);

    isa_l_desc->ec_encode_data(blocksize, k, num_missing_elements, g_tbls, (unsigned char**)available_fragments,
                               (unsigned char**)decoded_elements);

    ret = 0;

out:
    free(g_tbls);
    free(decode_matrix);
    free(decode_inverse);
    free(inverse_rows);
    free(decoded_elements);
    free(available_fragments);

    return ret;
}

int isa_l_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    isa_l_descriptor *isa_l_desc = (isa_l_descriptor*) desc;
    unsigned char *g_tbls = NULL;
    unsigned char *decode_matrix = NULL;
    unsigned char *decode_inverse = NULL;
    unsigned char *inverse_rows = NULL;
    unsigned char *reconstruct_buf = NULL;
    unsigned char **available_fragments = NULL;
    int k = isa_l_desc->k;
    int m = isa_l_desc->m;
    int n = k + m;
    int ret = -1;
    int i, j;
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);
    int inverse_row = -1;

    /**
     * Get available elements and compute the inverse of their
     * corresponding rows.
     */
    decode_matrix = isa_l_get_decode_matrix(k, m, isa_l_desc->matrix, missing_idxs);

    if (NULL == decode_matrix) {
        goto out;
    }

    decode_inverse = (unsigned char*)malloc(sizeof(unsigned char) * k * k);

    if (NULL == decode_inverse) {
        goto out;
    }

    int im_ret = isa_l_desc->gf_invert_matrix(decode_matrix, decode_inverse, k);
    if (im_ret < 0) {
        goto out;
    }

    /**
     * Get the row needed to reconstruct
     */
    inverse_rows = get_inverse_rows(k, m, decode_inverse, isa_l_desc->matrix, missing_idxs, isa_l_desc->gf_mul);

    // Generate g_tbls from computed decode matrix (k x k) matrix
    g_tbls = malloc(sizeof(unsigned char) * (k * m * 32));
    if (NULL == g_tbls) {
        goto out;
    }

    /**
     * Fill in the available elements
     */
    available_fragments = (unsigned char**)malloc(sizeof(unsigned char*)*k);
    if (NULL == available_fragments) {
        goto out;
    }

    j = 0;
    for (i = 0; i < n; i++) {
        if (missing_bm & (1 << i)) {
            continue;
        }
        if (j == k) {
          break;
        }
        if (i < k) {
            available_fragments[j] = (unsigned char*)data[i];
        } else {
            available_fragments[j] = (unsigned char*)parity[i-k];
        }
        j++;
    }

    /**
     * Copy pointer of buffer to reconstruct
     */
    j = 0;
    for (i = 0; i < n; i++) {
        if (missing_bm & (1 << i)) {
            if (i == destination_idx) {
                if (i < k) {
                    reconstruct_buf = (unsigned char*)data[i];
                } else {
                    reconstruct_buf = (unsigned char*)parity[i-k];
                }
                inverse_row = j;
                break;
            }
            j++;
        }
    }

    /**
     * Do the reconstruction
     */
    isa_l_desc->ec_init_tables(k, 1, &inverse_rows[inverse_row * k], g_tbls);

    isa_l_desc->ec_encode_data(blocksize, k, 1, g_tbls, (unsigned char**)available_fragments,
                               (unsigned char**)&reconstruct_buf);

    ret = 0;
out:
    free(g_tbls);
    free(decode_matrix);
    free(decode_inverse);
    free(inverse_rows);
    free(available_fragments);

    return ret;
}

int isa_l_min_fragments(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed)
{
    isa_l_descriptor *isa_l_desc = (isa_l_descriptor*)desc;

    uint64_t exclude_bm = convert_list_to_bitmap(fragments_to_exclude);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs) | exclude_bm;
    int i;
    int j = 0;
    int ret = -1;

    for (i = 0; i < (isa_l_desc->k + isa_l_desc->m); i++) {
        if (!(missing_bm & (1 << i))) {
            fragments_needed[j] = i;
            j++;
        }
        if (j == isa_l_desc->k) {
            ret = 0;
            fragments_needed[j] = -1;
            break;
        }
    }

    return ret;
}

/**
 * Return the element-size, which is the number of bits stored
 * on a given device, per codeword.  This is always 8 in ISA-L
 *
 * Returns the size in bits!
 */
int isa_l_element_size(void* desc)
{
  return 8;
}

int isa_l_exit(void *desc)
{
    isa_l_descriptor *isa_l_desc = NULL;

    isa_l_desc = (isa_l_descriptor*) desc;

    free(isa_l_desc);

    return 0;
}


void * isa_l_common_init(struct ec_backend_args *args, void *backend_sohandle,
        const char* gen_matrix_func_name)
{
    isa_l_descriptor *desc = NULL;

    desc = (isa_l_descriptor *)malloc(sizeof(isa_l_descriptor));
    if (NULL == desc) {
        return NULL;
    }

    desc->k = args->uargs.k;
    desc->m = args->uargs.m;
    if (args->uargs.w <= 0)
        args->uargs.w = ISA_L_W;
    desc->w = args->uargs.w;

    /* validate EC arguments */
    {
        long long max_symbols = 1LL << desc->w;
        if ((desc->k + desc->m) > max_symbols) {
            goto error;
        }
     }

     /*
     * ISO C forbids casting a void* to a function pointer.
     * Since dlsym return returns a void*, we use this union to
     * "transform" the void* to a function pointer.
     */
    union {
        ec_encode_data_func encodep;
        ec_init_tables_func init_tablesp;
        gf_gen_encoding_matrix_func gen_matrixp;
        gf_invert_matrix_func invert_matrixp;
        gf_mul_func gf_mulp;
        void *vptr;
    } func_handle = {.vptr = NULL};

    /* fill in function addresses */
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "ec_encode_data");
    desc->ec_encode_data = func_handle.encodep;
    if (NULL == desc->ec_encode_data) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "ec_init_tables");
    desc->ec_init_tables = func_handle.init_tablesp;
    if (NULL == desc->ec_init_tables) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, gen_matrix_func_name);
    desc->gf_gen_encoding_matrix = func_handle.gen_matrixp;
    if (NULL == desc->gf_gen_encoding_matrix) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "gf_invert_matrix");
    desc->gf_invert_matrix = func_handle.invert_matrixp;
    if (NULL == desc->gf_invert_matrix) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "gf_mul");
    desc->gf_mul = func_handle.gf_mulp;
    if (NULL == desc->gf_mul) {
        goto error;
    }

    desc->matrix = malloc(sizeof(char) * desc->k * (desc->k + desc->m));
    if (NULL == desc->matrix) {
        goto error;
    }

    /**
     * Generate ISA-L encoding matrix
     * Note that this is an abstract func from each backend
     */
    desc->gf_gen_encoding_matrix(desc->matrix, desc->k + desc->m, desc->k);

    return desc;

error:
    free(desc);

    return NULL;
}
