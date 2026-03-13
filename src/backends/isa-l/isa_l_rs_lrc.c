/*
 * Copyright 2025 aitassou
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
 * isa_l_rs_lrc backend implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdlib.h>
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "isa_l_common.h"

#define ISA_L_RS_LRC_LIB_MAJOR 1
#define ISA_L_RS_LRC_LIB_MINOR 0
#define ISA_L_RS_LRC_LIB_REV   0
#define ISA_L_RS_LRC_LIB_VER_STR "1.0"
#define ISA_L_RS_LRC_LIB_NAME "isa_l_rs_vand"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define ISA_L_RS_LRC_SO_NAME "libisal" LIBERASURECODE_SO_SUFFIX ".dylib"
#else
#define ISA_L_RS_LRC_SO_NAME "libisal" LIBERASURECODE_SO_SUFFIX ".so.2"
#endif

/* Forward declarations */
struct ec_backend_common backend_isa_l_rs_lrc;

static int gen_encoding_matrix(isa_l_descriptor * desc, int m, int k) {
    int i, j, ret = 1;
    unsigned char p, gen = 2;
    unsigned char *tmp = NULL;
    unsigned char *tmp_inv_k = NULL;
    int n = m + k;
    int l = desc->l;   //local parities
    int r = m - l;     //global parities

    /* Build a (k+m)*k Vandermonde matrix, A */
    tmp = malloc(sizeof(char) * n * k);
    if (tmp == NULL) {
        goto error_free;
    }
    for (i = 0; i < n; i++) {
        p = 1;
        for (j = 0; j < k; j++) {
                tmp[k * i + j] = p;
                p = desc->gf_mul(p, gen);
        }
        if (i < k + r) {
            gen = desc->gf_mul(gen, 2);
        }
    }

    /* It starts with a k*k submatrix, A'; calculate inv(A') */
    tmp_inv_k = malloc(sizeof(char) * k * k);
    if (tmp_inv_k == NULL) {
        goto error_free;
    }
    int im_ret = desc->gf_invert_matrix(tmp, tmp_inv_k, k);
    if (im_ret < 0) {
        /**
         * Should never happen as it's a proper Vandermonde matrix,
         * but belt & bracers...
         */
        goto error_free;
    }

    /**
     * Now we're ready to build the encoding matrix: inv(A') * A.
     * Save some multiplies by going straight to I for the start.
     */
    memset(desc->matrix, 0, k * n);
    for (i = 0; i < k; i++)
        desc->matrix[k * i + i] = 1;

    /* Then multiply inv(A') by the rest of A for the parities */
    for (i = k; i < n; i++) {
        for (j = 0; j < k; j++) {
            p = 0;
            for (int u = 0; u < k; u++) {
                p ^= desc->gf_mul(tmp[(i*k)+u], tmp_inv_k[(u*k)+j]);
            }
            desc->matrix[(i*k)+j] = p;
        }
    }

    int group_offset = 0;
    for (i = 0; i < l; i++) {
        int frag_num = k + r + i;
        int group_size = local_group_size(k, l, i);
        for (j = 0; j < k; j++) {
            if ( j < group_offset || j >= group_offset + group_size)
                desc->matrix[k * frag_num + j] = 0;
        }
        group_offset += group_size;
    }
    ret = 0;
error_free:
    free(tmp_inv_k);
    free(tmp);

    return ret;
}

static void * isa_l_rs_lrc_init(struct ec_backend_args *args,
        void *backend_sohandle)
{
    isa_l_descriptor *desc = NULL;

    desc = (isa_l_descriptor *)malloc(sizeof(isa_l_descriptor));
    if (NULL == desc) {
        return NULL;
    }
    /* Set this early so we can have a single error path */
    desc->matrix = NULL;

    desc->k = args->uargs.k;
    desc->m = args->uargs.m;
    desc->l = args->uargs.priv_args1.lrc_args.l;
    if (desc->l < 1 || desc->l > desc->m || (2 * desc->l > desc->k)) {
        goto error;
    }
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

    if (0 != gen_encoding_matrix(desc, desc->m, desc->k)) {
        goto error;
    }

    /**
     * Generate the tables for encoding
     */
    desc->encode_tables = malloc(sizeof(unsigned char) *
                                 (desc->k * desc->m * 32));
    if (NULL == desc->encode_tables) {
        goto error;
    }

    desc->ec_init_tables(desc->k, desc->m,
                         &desc->matrix[desc->k * desc->k],
                         desc->encode_tables);

    return desc;

error:
    free(desc->matrix);
    free(desc);

    return NULL;
}


static int isa_l_rs_lrc_check_reconstruct_fragments(void *desc, int *missing_idxs, int destination_idx) {
    isa_l_descriptor *d = (isa_l_descriptor*)desc;
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);
    int missing_locals, local_group;
    if (destination_idx < d->k) {
        local_group = local_group_for_data(d->k, d->l, destination_idx);
        int local_parity_index = d->k + d->m - d->l + local_group;
        missing_locals = (1 << local_parity_index) & missing_bm;
        for (
            int i = local_group_data_lower(d->k, d->l, local_group);
            i < local_group_data_upper(d->k, d->l, local_group);
            i++
        ) {
            if (i != destination_idx)
                missing_locals |= (1 << i) & missing_bm;
        }
        if (!missing_locals) {
            return 0;
        }
    } else if (destination_idx >= d->k + d->m - d->l) {
        local_group = destination_idx - d->k - d->m + d->l;
        missing_locals = 0;
        for (
            int i = local_group_data_lower(d->k, d->l, local_group);
            i < local_group_data_upper(d->k, d->l, local_group);
            i++
        ) {
            missing_locals |= (1 << i) & missing_bm;
        }
        if (!missing_locals) {
            return 0;
        }
    }

    // if we haven't returned yet, we can't do local-only reconstruction
    int useful_frags = 0, can_use_local_parity = 0;
    for (int i = 0; i < d->k + d->m; i++) {
        if (i < d->k) {
            if ((1 << i) & missing_bm) {
                local_group = local_group_for_data(d->k, d->l, i);
                can_use_local_parity |= 1 << (d->k + d->m - d->l + local_group);
            } else {
                useful_frags++;
            }
        } else if (i >= d->k + d->m - d->l) {
            if ((1 << i) & can_use_local_parity && !((1 << i) & missing_bm)) {
                useful_frags++;
            }
        } else {
            if (!((1 << i) & missing_bm)) {
                useful_frags++;
            }
        }
    }

    if (useful_frags < d->k) {
        return -EINSUFFFRAGS;
    }
    return 0;
}


/*
 * For the time being, we only claim compatibility with versions that
 * match exactly
 */
static bool isa_l_rs_lrc_is_compatible_with(uint32_t version) {
    return version == backend_isa_l_rs_lrc.ec_backend_version;
}

static unsigned char* get_lrc_inverse_rows(int k,
                                       int m,
                                       int min_range,
                                       int max_range,
                                       int missing_local_parity,
                                       unsigned char *decode_inverse,
                                       unsigned char* encode_matrix,
                                       uint64_t missing_bm,
                                       gf_mul_func gf_mul)
{
    int num_missing_elements = 0;
    for (int i = 0; i < EC_MAX_FRAGMENTS; i++)
        if ((1LLU << i) & missing_bm)
            num_missing_elements++;
    unsigned char *inverse_rows = (unsigned char*)malloc(sizeof(unsigned
                                    char*) * k * num_missing_elements);
    int i, j, l = 0;

    if (NULL == inverse_rows) {
        return NULL;
    }

    int matrix_size = max_range - min_range;

    int encode_matrix_size  = (matrix_size == 0 ||matrix_size == k || missing_local_parity) ? k : matrix_size;

    int n = k + m;

    memset(inverse_rows, 0, sizeof(unsigned
            char*) * matrix_size * num_missing_elements);

    /*
     * Fill in rows for missing data
     */
    for (i = 0; i < matrix_size; i++) {
        if ((1 << i) & (missing_bm>>min_range)) {
            for (j = 0; j < matrix_size; j++) {
                inverse_rows[(l * matrix_size) + j] = decode_inverse[(i * matrix_size) + j];
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
        if ((1 << i) & (missing_bm)) {
            int d_idx_avail = 0;
            int d_idx_unavail = 0;
            for (j = min_range; j < max_range; j++) {
                // This data is available, so we can use the encode matrix
                if (((1 << j) & (missing_bm)) == 0) {
                    inverse_rows[(l * matrix_size) + d_idx_avail] ^= encode_matrix[(i * encode_matrix_size) + j];
                    d_idx_avail++;
                } else {
                    mult_and_xor_row(&inverse_rows[l * matrix_size],
                                     &inverse_rows[d_idx_unavail * matrix_size],
                                     encode_matrix[(i * encode_matrix_size) + j],
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

static unsigned char* isa_l_lrc_get_decode_matrix(int k, int m, unsigned local_parity, unsigned char *encode_matrix, uint64_t missing_bm, int *used_idxs, int *use_combined_parity)
{
    int i = 0, j, locate = 0;
    int n = k + m;
    int global_parity = m - local_parity;
    int group_offset = 0;
    uint64_t missing_local_parity = 0;
    uint64_t use_parity = 0;

    int total_missing = 0;

    unsigned char *decode_matrix = malloc(sizeof(unsigned char) * k * k);
    if( NULL == decode_matrix ) {
        return NULL;
    }

    for (int v = 0; v < local_parity; v++) {
        int group_size = local_group_size(k, local_parity, v);
        for (int u = group_offset; u < group_offset + group_size; u++) {
            if ((1 << u) & missing_bm) {
                use_parity |= (1 << (k + global_parity + v));
            }
        }
        group_offset += group_size;

        missing_local_parity |= (1 << (k + global_parity + v)) & missing_bm;
    }

    for (locate = 0; i < k && locate < k + global_parity; locate++) {
        if (((1 << locate) & missing_bm)) {
            total_missing++;
            continue;
        }
        for (j = 0; j < k; j++) {
            decode_matrix[(k * i) + j] = encode_matrix[(k * locate) + j];
        }
        used_idxs[locate] = 1;
        i++;
    }
    // we can simplify here as total_missing counts only missing data + global parity
    if (i < k && !missing_local_parity && (total_missing == global_parity + 1)) {
        // Set flag to indicate we can use combined parity in case of
        // g + 1 errors and no local parity is missing
        *use_combined_parity = 1;
        // We can combine all the local parities into a single global parity
        group_offset = 0;
        for (int v = 0; v < local_parity; v++) {
            int group_size = local_group_size(k, local_parity, v);
            for (int u = group_offset; u < group_offset + group_size; u++) {
                decode_matrix[(i * k) + u] = encode_matrix[((locate + v) * k) + u];
            }
            group_offset += group_size;
        }
        i++;
    }
    if (i < k) {
        // Still not enough? Well, let's add what local parities we have,
        // see if we can get lucky
        for (locate = k + global_parity; i < k && locate < n; locate++) {
            if (use_parity & (1 << locate) && !((1 << locate) & missing_bm)) {
                for (j = 0; j < k; j++) {
                    decode_matrix[(k * i) + j] = encode_matrix[(k * locate) + j];
                }
                used_idxs[locate] = 1;
                i++;
            }
        }
    }
    if (i != k) {
        free(decode_matrix);
        decode_matrix = NULL;
    }
    return decode_matrix;
}


static int isa_l_lrc_decode(void *desc, char **data, char **parity,
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
    int local_parity = isa_l_desc->l;
    int n = k + m;
    int ret = -1;
    int i, j;
    unsigned char *combined_local_parities = NULL;
    int use_combined_parity = 0;

    int num_missing_elements = get_num_missing_elements(missing_idxs);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);

    int *used_idxs = calloc(n, sizeof(int));
    if(NULL == used_idxs) {
        goto out;
    }
    decode_matrix = isa_l_lrc_get_decode_matrix(k, m, local_parity, isa_l_desc->matrix, missing_bm, used_idxs, &use_combined_parity);

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

    inverse_rows = get_lrc_inverse_rows(k, m, 0, k, 0, decode_inverse, isa_l_desc->matrix, missing_bm, isa_l_desc->gf_mul);

    decoded_elements = (unsigned char**)malloc(sizeof(unsigned char*)*num_missing_elements);
    if (NULL == decoded_elements) {
        goto out;
    }

    available_fragments = (unsigned char**)malloc(sizeof(unsigned char*)*k);
    if (NULL == available_fragments) {
        goto out;
    }

    uint64_t missing_local_parities = 0;
    for (j = n - local_parity; j < n; j++) {
        missing_local_parities |= (missing_bm & (1 << j));
    }

    for (i = 0, j = 0; i < n - local_parity && j < k; i++) {
        if (missing_bm & (1 << i)) {
            continue;
        }
        if (i < k) {
            available_fragments[j] = (unsigned char*)data[i];
            j++;
        } else {
            if (used_idxs[i]) {
                available_fragments[j] = (unsigned char*)parity[i - k];
                j++;
            }
        }
    }
    if (j < k && !missing_local_parities && use_combined_parity) {
        combined_local_parities = calloc(blocksize, sizeof(unsigned char));
        if (NULL == combined_local_parities) {
            goto out;
        }
        for (i = n - local_parity; i < n; i++) {
            for (int x = 0; x < blocksize; x++) {
                combined_local_parities[x] ^= parity[i - k][x];
            }
        }
        available_fragments[j] = combined_local_parities;
        j++;
    }
    for (i = n - local_parity; i < n && j < k; i++) {
        if (used_idxs[i]) {
            available_fragments[j] = (unsigned char*)parity[i-k];
            j++;
        }
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

    isa_l_desc->ec_encode_data(blocksize, k, num_missing_elements, g_tbls, available_fragments,
                               decoded_elements);

    ret = 0;

out:
    free(g_tbls);
    free(combined_local_parities);
    free(decode_matrix);
    free(decode_inverse);
    free(inverse_rows);
    free(decoded_elements);
    free(available_fragments);
    free(used_idxs);

    return ret;
}

static unsigned char* isa_l_lrc_get_reconstruct_matrix(
            int k, int m, unsigned local_parity, int destination_idx,
            unsigned char *encode_matrix, uint64_t *missing_bm, int *used_idxs,
            int *min_col, int *max_col, int *mx_size, int *missing_local_parity, int *use_combined_parity)
{
    unsigned char *decode_matrix = NULL;
    uint64_t useful_mask = 0;

    int min_range=0, max_range=0;
    if (destination_idx < k) {
        // reconstructing a data frag; see if we can stay local
        int local_group = local_group_for_data(k, local_parity, destination_idx);
        min_range = local_group_data_lower(k, local_parity, local_group);
        max_range = local_group_data_upper(k, local_parity, local_group);
        int local_parity_idx = k + m - local_parity + local_group;
        int missing_local = (1 << local_parity_idx) & *missing_bm;
        for (int i = min_range; !missing_local && i < max_range; i++) {
            useful_mask |= 1 << i;
            if (i == destination_idx) {
                // We already knew we were missing *that* one...
                continue;
            }
            missing_local |= (1 << i) & *missing_bm;
        }
        if (!missing_local) {
            // We have everything we need!
            useful_mask |= 1 << local_parity_idx;
            *missing_bm &= useful_mask;
            *mx_size = max_range - min_range;
            decode_matrix = malloc(sizeof(unsigned char) * (*mx_size) * (*mx_size));
            if (NULL == decode_matrix) {
                return NULL;
            }
            int col = 0;
            for (int enc_idx = min_range; enc_idx < max_range; enc_idx++) {
                if (enc_idx == destination_idx)
                    continue;
                for (int j = min_range; j < max_range; j++) {
                    decode_matrix[(*mx_size * col) + j - min_range] = encode_matrix[(k * enc_idx) + j];
                }
                used_idxs[enc_idx] = 1;
                col++;
            }
            // add local parity
            for (int j = min_range; j < max_range; j++) {
                decode_matrix[*mx_size * col + j - min_range] = encode_matrix[(k * local_parity_idx) + j];
            }
            used_idxs[local_parity_idx] = 1;
        }
    }
    if (destination_idx >= k + m - local_parity) {
        // reconstructing a local parity frag; see if we can use local data
        int local_group = destination_idx - k - m + local_parity;
        min_range = local_group_data_lower(k, local_parity, local_group);
        max_range = local_group_data_upper(k, local_parity, local_group);
        int missing_local = 0;
        for (int i = min_range; !missing_local && i < max_range; i++) {
            useful_mask |= 1 << i;
            missing_local |= (1 << i) & *missing_bm;
        }
        if (!missing_local) {
            // We have everything we need!
            useful_mask |= 1 << destination_idx;
            *missing_bm &= useful_mask;
            *missing_local_parity = 1;
            *mx_size = max_range - min_range;
            decode_matrix = malloc(sizeof(unsigned char) * (*mx_size) * (*mx_size));
            if (NULL == decode_matrix) {
                return NULL;
            }
            for (int i = min_range; i < max_range; i++) {
                for (int j = min_range; j < max_range; j++) {
                    decode_matrix[*mx_size * (i - min_range) + j - min_range] = encode_matrix[(k * i) + j];
                }
                used_idxs[i] = 1;
            }
        }
    }

    if (decode_matrix == NULL) {
        decode_matrix = isa_l_lrc_get_decode_matrix(k, m, local_parity,
            encode_matrix, *missing_bm, used_idxs, use_combined_parity);
        if (decode_matrix) {
            *mx_size = k;
        }
        min_range = 0;
        max_range = k;
    }

    *min_col = min_range;
    *max_col = max_range;
    return decode_matrix;
}

static int isa_l_lrc_reconstruct(void *desc, char **data, char **parity,
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
    int local_parity = isa_l_desc->l;
    int n = k + m;
    int ret = -1;
    int i, j;
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);
    int inverse_row = -1;
    int min_range = 0;
    int max_range = 0;
    int matrix_size = k;
    int *used_idxs = calloc(n, sizeof(int));
    int missing_local_parity = 0;
    unsigned char * combined_local_parities = NULL;
    int use_combined_parity = 0;

    if( NULL == used_idxs) {
        goto out;
    }
    /**
     * Get available elements and compute the inverse of their
     * corresponding rows.
     */
    decode_matrix = isa_l_lrc_get_reconstruct_matrix(k, m, local_parity, destination_idx, isa_l_desc->matrix, &missing_bm, used_idxs, &min_range, &max_range, &matrix_size, &missing_local_parity, &use_combined_parity);
    if (NULL == decode_matrix) {
        goto out;
    }

    decode_inverse = (unsigned char*)malloc(sizeof(unsigned char) * matrix_size * matrix_size);

    if (NULL == decode_inverse) {
        goto out;
    }

    int im_ret = isa_l_desc->gf_invert_matrix(decode_matrix, decode_inverse, matrix_size);
    if (im_ret < 0) {
        goto out;
    }

    int nb_parity = (matrix_size == k)? m: 1;
    unsigned char * encode = (matrix_size == k || missing_local_parity)?  isa_l_desc->matrix:decode_matrix;

    /**
     * Get the row needed to reconstruct
     */
    inverse_rows = get_lrc_inverse_rows(k, m, min_range, max_range, missing_local_parity, decode_inverse, encode, missing_bm, isa_l_desc->gf_mul);

    // Generate g_tbls from computed decode matrix (k x k) matrix
    g_tbls = malloc(sizeof(unsigned char) * (matrix_size * nb_parity * 32));
    if (NULL == g_tbls) {
        goto out;
    }

    /**
     * Fill in the available elements
     */
    available_fragments = (unsigned char**)malloc(sizeof(unsigned char*)*matrix_size);
    if (NULL == available_fragments) {
        goto out;
    }

    j = 0;
    if (matrix_size == k) {
        for (i = 0; i < n - local_parity && j < k; i++) {
            if (missing_bm & (1 << i)) {
                continue;
            }
            if (i < k) {
                available_fragments[j] = (unsigned char*)data[i];
                j++;
            } else {
                if (used_idxs[i]) {
                    available_fragments[j] = (unsigned char*)parity[i - k];
                    j++;
                }
            }
        }
        if (j < k && !missing_local_parity && use_combined_parity) {
            combined_local_parities = calloc(blocksize, sizeof(unsigned char));
            if (NULL == combined_local_parities) {
                goto out;
            }
            for (i = n - local_parity; i < n; i++) {
                for (int x = 0; x < blocksize; x++) {
                    combined_local_parities[x] ^= parity[i - k][x];
                }
            }
            available_fragments[j] = combined_local_parities;
            j++;
        }
        for (i = n - local_parity; i < n && j < k; i++) {
            if (missing_bm & (1 << i)) {
                continue;
            }
            if (used_idxs[i]) {
                available_fragments[j] = (unsigned char*)parity[i - k];
                j++;
            }
        }
    } else {
        for (i = 0; i < n; i++) {
            if (used_idxs[i]) {
                if (i <k) {
                    available_fragments[j] = (unsigned char*)data[i];
                } else {
                    available_fragments[j] = (unsigned char*)parity[i - k];
                }
                j++;
            }
        }
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
                    reconstruct_buf = (unsigned char*)parity[i - k];
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
    isa_l_desc->ec_init_tables(matrix_size, 1, &inverse_rows[inverse_row * matrix_size], g_tbls);
    isa_l_desc->ec_encode_data(blocksize, matrix_size, 1, g_tbls, available_fragments,
                               &reconstruct_buf);

    ret = 0;
out:
    free(g_tbls);
    free(combined_local_parities);
    free(decode_matrix);
    free(decode_inverse);
    free(inverse_rows);
    free(available_fragments);
    free(used_idxs);

    return ret;
}

static struct ec_backend_op_stubs isa_l_rs_lrc_op_stubs = {
    .INIT                       = isa_l_rs_lrc_init,
    .EXIT                       = isa_l_exit,
    .ISSYSTEMATIC               = 1,
    .ENCODE                     = isa_l_encode,
    .DECODE                     = isa_l_lrc_decode,
    .FRAGSNEEDED                = isa_l_min_fragments,
    .RECONSTRUCT                = isa_l_lrc_reconstruct,
    .ELEMENTSIZE                = isa_l_element_size,
    .ISCOMPATIBLEWITH           = isa_l_rs_lrc_is_compatible_with,
    .GETMETADATASIZE            = get_backend_metadata_size_zero,
    .GETENCODEOFFSET            = get_encode_offset_zero,
    .CHECKRECONSTRUCTFRAGMENTS  = isa_l_rs_lrc_check_reconstruct_fragments,
};

__attribute__ ((visibility ("internal")))
struct ec_backend_common backend_isa_l_rs_lrc = {
    .id                         = EC_BACKEND_ISA_L_RS_LRC,
    .name                       = ISA_L_RS_LRC_LIB_NAME,
    .soname                     = ISA_L_RS_LRC_SO_NAME,
    .soversion                  = ISA_L_RS_LRC_LIB_VER_STR,
    .ops                        = &isa_l_rs_lrc_op_stubs,
    .ec_backend_version         = _VERSION(ISA_L_RS_LRC_LIB_MAJOR,
                                           ISA_L_RS_LRC_LIB_MINOR,
                                           ISA_L_RS_LRC_LIB_REV),
};
