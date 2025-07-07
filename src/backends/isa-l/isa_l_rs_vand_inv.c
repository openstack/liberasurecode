/*
 * Copyright 2014 Kevin M Greenan
 * Copyright 2014 Tushar Gohad
 * Copyright 2025 Tim Burke
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

#include <stdlib.h>
#include "erasurecode_backend.h"
#include "isa_l_common.h"

#define ISA_L_RS_VAND_INV_LIB_MAJOR 1
#define ISA_L_RS_VAND_INV_LIB_MINOR 0
#define ISA_L_RS_VAND_INV_LIB_REV   0
#define ISA_L_RS_VAND_INV_LIB_VER_STR "1.0"
#define ISA_L_RS_VAND_INV_LIB_NAME "isa_l_rs_vand"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define ISA_L_RS_VAND_INV_SO_NAME "libisal" LIBERASURECODE_SO_SUFFIX ".dylib"
#else
#define ISA_L_RS_VAND_INV_SO_NAME "libisal" LIBERASURECODE_SO_SUFFIX ".so.2"
#endif

/* Forward declarations */
struct ec_backend_common backend_isa_l_rs_vand_inv;

static int gen_encoding_matrix(isa_l_descriptor * desc, int m, int k){
    int i, j, ret = 1, n = m + k;
    unsigned char p, gen = 2;
    unsigned char *tmp = NULL;
    unsigned char *tmp_inv_k = NULL;

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
        gen = desc->gf_mul(gen, 2);
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
    ret = 0;

error_free:
    free(tmp_inv_k);
    free(tmp);
    return ret;
}

static void * isa_l_rs_vand_inv_init(struct ec_backend_args *args,
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

/*
 * For the time being, we only claim compatibility with versions that
 * match exactly
 */
static bool isa_l_rs_vand_inv_is_compatible_with(uint32_t version) {
    return version == backend_isa_l_rs_vand_inv.ec_backend_version;
}

static struct ec_backend_op_stubs isa_l_rs_vand_inv_op_stubs = {
    .INIT                       = isa_l_rs_vand_inv_init,
    .EXIT                       = isa_l_exit,
    .ENCODE                     = isa_l_encode,
    .DECODE                     = isa_l_decode,
    .FRAGSNEEDED                = isa_l_min_fragments,
    .RECONSTRUCT                = isa_l_reconstruct,
    .ELEMENTSIZE                = isa_l_element_size,
    .ISCOMPATIBLEWITH           = isa_l_rs_vand_inv_is_compatible_with,
    .GETMETADATASIZE            = get_backend_metadata_size_zero,
    .GETENCODEOFFSET            = get_encode_offset_zero,
};

__attribute__ ((visibility ("internal")))
struct ec_backend_common backend_isa_l_rs_vand_inv = {
    .id                         = EC_BACKEND_ISA_L_RS_VAND_INV,
    .name                       = ISA_L_RS_VAND_INV_LIB_NAME,
    .soname                     = ISA_L_RS_VAND_INV_SO_NAME,
    .soversion                  = ISA_L_RS_VAND_INV_LIB_VER_STR,
    .ops                        = &isa_l_rs_vand_inv_op_stubs,
    .ec_backend_version         = _VERSION(ISA_L_RS_VAND_INV_LIB_MAJOR,
                                           ISA_L_RS_VAND_INV_LIB_MINOR,
                                           ISA_L_RS_VAND_INV_LIB_REV),
};
