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
 * jerasure_rs_cauchy backend implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"

/* Forward declarations */
struct ec_backend_op_stubs jerasure_rs_cauchy_ops;
struct ec_backend jerasure_rs_cauchy;

/*
 * ToDo (KMG): Should we make this a parameter, or is that
 * exposing too much b.s. to the client?
 */
#define PYECC_CAUCHY_PACKETSIZE sizeof(long) * 128

struct jerasure_rs_cauchy_descriptor {
    /* calls required for init */
    int * (*cauchy_original_coding_matrix)(int, int, int);
    int * (*jerasure_matrix_to_bitmatrix)(int, int, int, int*);
    int ** (*jerasure_smart_bitmatrix_to_schedule)(int, int, int, int*);
 
    /* calls required for encode */
    void (*jerasure_bitmatrix_encode)(int, int, int, int *, char **, char **, int, int);
                            
    
    /* calls required for decode */
    int (*jerasure_bitmatrix_decode)(int, int, int, int *, int, int *,char **, char **, int, int);
                            
    
    /* calls required for reconstruct */
    int * (*jerasure_erasures_to_erased)(int, int, int *);
    int (*jerasure_make_decoding_bitmatrix)(int, int, int, int *, int *, int *, int *);
    void (*jerasure_bitmatrix_dotprod)(int, int, int *, int *, int,char **, char **, int, int);

    /* fields needed to hold state */
    int *matrix;
    int *bitmatrix;
    int **schedule;
    int k;
    int m;
    int w;
};

static int jerasure_rs_cauchy_encode(void *desc, char **data, char **parity,
        int blocksize)
{
    struct jerasure_rs_cauchy_descriptor *jerasure_desc = 
        (struct jerasure_rs_cauchy_descriptor*) desc;

    /* FIXME - make jerasure_bitmatrix_encode return a value */
    jerasure_desc->jerasure_bitmatrix_encode(jerasure_desc->k, jerasure_desc->m,
                                jerasure_desc->w, jerasure_desc->bitmatrix,
                                data, parity, blocksize,
                                PYECC_CAUCHY_PACKETSIZE);

    return 0;
}

static int jerasure_rs_cauchy_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    struct jerasure_rs_cauchy_descriptor *jerasure_desc = 
        (struct jerasure_rs_cauchy_descriptor*)desc;

    /* FIXME - make jerasure_matrix_decode return a value */
    jerasure_desc->jerasure_bitmatrix_decode(jerasure_desc->k, 
                                             jerasure_desc->m, 
                                             jerasure_desc->w, 
                                             jerasure_desc->bitmatrix, 
                                             0, 
                                             missing_idxs,
                                             data, 
                                             parity, 
                                             blocksize, 
                                             PYECC_CAUCHY_PACKETSIZE); 
}

static int jerasure_rs_cauchy_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    int ret = 1;
    int *decoding_row = NULL;
    int *erased = NULL;

    struct jerasure_rs_cauchy_descriptor *jerasure_desc = 
        (struct jerasure_rs_cauchy_descriptor*) desc;
    int *dm_ids = (int *)
        alloc_zeroed_buffer(sizeof(int) * jerasure_desc->k);
    int *decoding_matrix = (int *)
        (int *) alloc_zeroed_buffer(sizeof(int*) * jerasure_desc->k * jerasure_desc->k * jerasure_desc->w * jerasure_desc->w); 

    if (NULL == decoding_matrix || NULL == dm_ids) {
        goto out;
    }

    if (destination_idx < jerasure_desc->k) {

        erased = jerasure_desc->jerasure_erasures_to_erased(jerasure_desc->k,
                jerasure_desc->m, missing_idxs);

        ret = jerasure_desc->jerasure_make_decoding_bitmatrix(jerasure_desc->k, jerasure_desc->m, jerasure_desc->w, jerasure_desc->bitmatrix,
                                               erased, decoding_matrix, dm_ids);
        decoding_row = decoding_matrix + (destination_idx * jerasure_desc->k * jerasure_desc->w * jerasure_desc->w);

    } else {
        ret = 0;
        decoding_row = jerasure_desc->bitmatrix + ((destination_idx - jerasure_desc->k) * jerasure_desc->k * jerasure_desc->w * jerasure_desc->w);
    }
      
    if (ret == 0) {
        jerasure_desc->jerasure_bitmatrix_dotprod(jerasure_desc->k, jerasure_desc->w, decoding_row, dm_ids, destination_idx,
                                   data, parity, blocksize,
                                   PYECC_CAUCHY_PACKETSIZE);
    }

out:
    if (NULL != decoding_matrix) {
        free(decoding_matrix);
    }
    if (NULL != dm_ids) {
        free(dm_ids);
    }

}

/*
 * Caller will allocate an array of size k for fragments_needed
 * 
 */
static int jerasure_rs_cauchy_min_fragments(void *desc, int *missing_idxs,
        int *fragments_needed)
{
    struct jerasure_rs_cauchy_descriptor *jerasure_desc = 
        (struct jerasure_rs_cauchy_descriptor*)desc;
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs);
    int i;
    int j = 0;
    int ret = -1;

    for (i = 0; i < (jerasure_desc->k + jerasure_desc->m); i++) {
        if (!(missing_bm & (1 << i))) {
            fragments_needed[j] = i;
            j++;
        }
        if (j == jerasure_desc->k) {
            ret = 0;
            break;
        }
    }

out:
    return ret;
}

#define DEFAULT_W 4
static void * jerasure_rs_cauchy_init(struct ec_backend_args *args,
        void *backend_sohandle)
{
    struct jerasure_rs_cauchy_descriptor *desc =
        (struct jerasure_rs_cauchy_descriptor *) malloc(
                sizeof(struct jerasure_rs_cauchy_descriptor));

    if (NULL == desc) {
        return NULL;
    }

    desc->k = args->uargs.k;
    desc->m = args->uargs.m;
    
    if (args->uargs.w <= 0)
        args->uargs.w = DEFAULT_W;

    /* store w back in args so upper layer can get to it */
    desc->w = args->uargs.w;

    /* validate EC arguments */
    {
        long long max_symbols;
        max_symbols = 1LL << desc->w;
        if ((desc->k + desc->m) > max_symbols) {
            goto error;
        }
    }
    
    /* fill in function addresses */
    desc->jerasure_bitmatrix_encode = dlsym(
            backend_sohandle, "jerasure_bitmatrix_encode");
    if (NULL == desc->jerasure_bitmatrix_encode) {
        goto error; 
    }
  
    desc->jerasure_bitmatrix_decode = dlsym(
            backend_sohandle, "jerasure_bitmatrix_decode");
    if (NULL == desc->jerasure_bitmatrix_decode) {
        goto error; 
    }
  
    desc->cauchy_original_coding_matrix = dlsym(
            backend_sohandle, "cauchy_original_coding_matrix");
    if (NULL == desc->cauchy_original_coding_matrix) {
        goto error; 
    }
    
    desc->jerasure_matrix_to_bitmatrix = dlsym(
            backend_sohandle, "jerasure_matrix_to_bitmatrix");
    if (NULL == desc->jerasure_matrix_to_bitmatrix) {
        goto error; 
    }
    
    desc->jerasure_smart_bitmatrix_to_schedule = dlsym(
            backend_sohandle, "jerasure_smart_bitmatrix_to_schedule");
    if (NULL == desc->jerasure_smart_bitmatrix_to_schedule) {
        goto error; 
    }
    
    desc->jerasure_make_decoding_bitmatrix = dlsym(
            backend_sohandle, "jerasure_make_decoding_bitmatrix");
    if (NULL == desc->jerasure_make_decoding_bitmatrix) {
        goto error; 
    }
    
    desc->jerasure_bitmatrix_dotprod = dlsym(
            backend_sohandle, "jerasure_bitmatrix_dotprod");
    if (NULL == desc->jerasure_bitmatrix_dotprod) {
        goto error; 
    }
  
    desc->jerasure_erasures_to_erased = dlsym(
            backend_sohandle, "jerasure_erasures_to_erased");
    if (NULL == desc->jerasure_erasures_to_erased) {
        goto error; 
    } 

    desc->matrix = desc->cauchy_original_coding_matrix(desc->k, desc->m, desc->w);
    desc->bitmatrix = desc->jerasure_matrix_to_bitmatrix(desc->k, desc->m, desc->w, desc->matrix);
    desc->schedule = desc->jerasure_smart_bitmatrix_to_schedule(desc->k, desc->m, desc->w, desc->bitmatrix);

    return desc;

error:
    if (NULL != desc) {
        free(desc);
    }
    return NULL;
}

/**
 * Return the element-size, which is the number of bits stored 
 * on a given device, per codeword.  
 * 
 * Returns the size in bits!
 */
static int
jerasure_rs_cauchy_element_size(void* desc)
{
    struct jerasure_rs_cauchy_descriptor *jerasure_desc = 
        (struct jerasure_rs_cauchy_descriptor*)desc;

    return jerasure_desc->w * PYECC_CAUCHY_PACKETSIZE * 8;
}

static int jerasure_rs_cauchy_exit(void *desc)
{
    struct jerasure_rs_cauchy_descriptor *jerasure_desc = 
        (struct jerasure_rs_cauchy_descriptor*)desc;

    if (NULL != desc) {
        free(desc);
    }
}

struct ec_backend_op_stubs jerasure_rs_cauchy_op_stubs = {
    .INIT                       = jerasure_rs_cauchy_init,
    .EXIT                       = jerasure_rs_cauchy_exit,
    .ENCODE                     = jerasure_rs_cauchy_encode,
    .DECODE                     = jerasure_rs_cauchy_decode,
    .FRAGSNEEDED                = jerasure_rs_cauchy_min_fragments,
    .RECONSTRUCT                = jerasure_rs_cauchy_reconstruct,
    .ELEMENTSIZE                = jerasure_rs_cauchy_element_size,
};

struct ec_backend_common backend_jerasure_rs_cauchy = {
    .id                         = EC_BACKEND_JERASURE_RS_CAUCHY,
    .name                       = "jerasure_rs_cauchy",
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
    .soname                     = "libJerasure.dylib",
#else
    .soname                     = "libJerasure.so",
#endif
    .soversion                  = "2.0",
    .ops                        = &jerasure_rs_cauchy_op_stubs,
};
