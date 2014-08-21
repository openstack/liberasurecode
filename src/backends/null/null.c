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
 * liberasurecode null backend
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"

/* Forward declarations */
struct ec_backend null;
struct ec_backend_op_stubs null_ops;

struct null_descriptor {
    /* calls required for init */
    void* (*init_null_code)(int k, int m, int hd); 

    /* calls required for encode */
    int (*null_code_encode)(void *code_desc, char **data, char **parity,
            int blocksize); 

    /* calls required for decode */
    int (*null_code_decode)(void *code_desc, char **data, char **parity,
            int *missing_idxs, int blocksize, int decode_parity);

    /* calls required for reconstruct */
    int (*null_reconstruct)(char **available_fragments, int num_fragments,
            uint64_t fragment_len, int destination_idx, char* out_fragment);

    /* set of fragments needed to reconstruct at a minimum */
    int (*null_code_fragments_needed)(void *code_desc, int *missing_idxs,
            int *fragments_to_exclude, int *fragments_needed);

    /* fields needed to hold state */
    int *matrix;
    int k;
    int m;
    int w;
    int arg1;
};

#define DEFAULT_W 32

static int null_encode(void *desc, char **data, char **parity,
        int blocksize)
{
    struct null_descriptor *xdesc =
        (struct null_descriptor *) desc;

    return 0;
}

static int null_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    struct null_descriptor *xdesc =
        (struct null_descriptor *) desc;

    return 0;
}

static int null_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    struct null_descriptor *xdesc =
        (struct null_descriptor *) desc;

    return 0;
}

static int null_min_fragments(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed)
{
    struct null_descriptor *xdesc =
        (struct null_descriptor *) desc;

    return 0;
}

/**
 * Return the element-size, which is the number of bits stored 
 * on a given device, per codeword.  This is usually just 'w'.
 */
static int
null_element_size(void* desc)
{
    return DEFAULT_W;
}

static void * null_init(struct ec_backend_args *args, void *backend_sohandle)
{
    int k, m, arg1, w;
    struct null_descriptor *xdesc = NULL;

    /* allocate and fill in null_descriptor */
    xdesc = (struct null_descriptor *) malloc(sizeof(struct null_descriptor));
    if (NULL == xdesc) {
        return NULL;
    }

    xdesc->k = args->uargs.k;
    xdesc->m = args->uargs.m;

    if (xdesc->w <= 0)
        xdesc->w = DEFAULT_W;

    /* Sample on how to pass extra args to the backend */
    xdesc->arg1 = args->uargs.priv_args1.null_args.arg1;

    /* store w back in args so upper layer can get to it */
    args->uargs.w = DEFAULT_W;

    /* validate EC arguments */
    {
        long long max_symbols;
        if (xdesc->w != 8 && xdesc->w != 16 && xdesc->w != 32) {
            goto error;
        }
        max_symbols = 1LL << xdesc->w;
        if ((xdesc->k + xdesc->m) > max_symbols) {
            goto error;
        }
    }

    /* fill in function addresses */
    xdesc->init_null_code = dlsym(
            backend_sohandle, "null_code_init");
    if (NULL == xdesc->init_null_code) {
        goto error; 
    }

    xdesc->null_code_encode = dlsym(
            backend_sohandle, "null_code_encode");
    if (NULL == xdesc->null_code_encode) {
        goto error; 
    }

    xdesc->null_code_decode = dlsym(
            backend_sohandle, "null_code_decode");
    if (NULL == xdesc->null_code_decode) {
        goto error; 
    }

    xdesc->null_reconstruct = dlsym(
            backend_sohandle, "null_reconstruct");
    if (NULL == xdesc->null_reconstruct) {
        goto error; 
    }

    xdesc->null_code_fragments_needed = dlsym(
            backend_sohandle, "null_code_fragments_needed");
    if (NULL == xdesc->null_code_fragments_needed) {
        goto error; 
    }

    return (void *) xdesc;

error:
    if (NULL != xdesc) {
        free(xdesc);
        xdesc = NULL;
    }

    return NULL;
}

static int null_exit(void *desc)
{
    struct null_descriptor *xdesc = (struct null_descriptor *) desc;

    free (xdesc);
}

struct ec_backend_op_stubs null_op_stubs = {
    .INIT                       = null_init,
    .EXIT                       = null_exit,
    .ENCODE                     = null_encode,
    .DECODE                     = null_decode,
    .FRAGSNEEDED                = null_min_fragments,
    .RECONSTRUCT                = null_reconstruct,
    .ELEMENTSIZE                = null_element_size,
};

struct ec_backend_common backend_null = {
    .id                         = EC_BACKEND_NULL,
    .name                       = "null",
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
    .soname                     = "libnullcode.dylib",
#else
    .soname                     = "libnullcode.so",
#endif
    .soversion                  = "1.0",
    .ops                        = &null_op_stubs,
};

