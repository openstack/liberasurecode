/* 
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
 * liberasurecode null backend
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"
#define NULL_LIB_MAJOR 1
#define NULL_LIB_MINOR 0
#define NULL_LIB_REV   0
#define NULL_LIB_VER_STR "1.0"
#define NULL_LIB_NAME "null"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define NULL_SO_NAME "libnullcode.dylib"
#else
#define NULL_SO_NAME "libnullcode.so.1"
#endif
/* Forward declarations */
struct ec_backend null;
struct ec_backend_op_stubs null_ops;

typedef void* (*init_null_code_func)(int, int, int);
typedef int (*null_code_encode_func)(void *, char **, char **, int);
typedef int (*null_code_decode_func)(void *, char **, char **, int *, int, int);
typedef int (*null_reconstruct_func)(char  **, int, uint64_t, int, char *);
typedef int (*null_code_fragments_needed_func)(void *, int *, int *, int *);

struct null_descriptor {
    /* calls required for init */
    init_null_code_func init_null_code;

    /* calls required for encode */
    null_code_encode_func null_code_encode;

    /* calls required for decode */
    null_code_decode_func null_code_decode;

    /* calls required for reconstruct */
    null_reconstruct_func null_reconstruct;

    /* set of fragments needed to reconstruct at a minimum */
    null_code_fragments_needed_func null_code_fragments_needed;

    /* fields needed to hold state */
    int *matrix;
    int k;
    int m;
    int w;
    int arg1;
};

#define DEFAULT_W 32

static int null_encode(void *desc, char **data, char **parity, int blocksize)
{
    return 0;
}

static int null_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    return 0;
}

static int null_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    return 0;
}

static int null_min_fragments(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed)
{
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
    struct null_descriptor *xdesc = NULL;

    /* allocate and fill in null_descriptor */
    xdesc = (struct null_descriptor *) malloc(sizeof(struct null_descriptor));
    if (NULL == xdesc) {
        return NULL;
    }
    memset(xdesc, 0, sizeof(struct null_descriptor));

    xdesc->k = args->uargs.k;
    xdesc->m = args->uargs.m;
    xdesc->w = args->uargs.w;

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

    /*
     * ISO C forbids casting a void* to a function pointer.
     * Since dlsym return returns a void*, we use this union to
     * "transform" the void* to a function pointer.
     */
    union {
        init_null_code_func initp;
        null_code_encode_func encodep;
        null_code_decode_func decodep;
        null_reconstruct_func reconp;
        null_code_fragments_needed_func fragsneededp;
        void *vptr;
    } func_handle = {.vptr = NULL};

    /* fill in function addresses */
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "null_code_init");
    xdesc->init_null_code = func_handle.initp;
    if (NULL == xdesc->init_null_code) {
        goto error; 
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "null_code_encode");
    xdesc->null_code_encode = func_handle.encodep;
    if (NULL == xdesc->null_code_encode) {
        goto error; 
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "null_code_decode");
    xdesc->null_code_decode = func_handle.decodep;
    if (NULL == xdesc->null_code_decode) {
        goto error; 
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "null_reconstruct");
    xdesc->null_reconstruct = func_handle.reconp;
    if (NULL == xdesc->null_reconstruct) {
        goto error; 
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "null_code_fragments_needed");
    xdesc->null_code_fragments_needed = func_handle.fragsneededp;
    if (NULL == xdesc->null_code_fragments_needed) {
        goto error; 
    }

    return (void *) xdesc;

error:
    free (xdesc);

    return NULL;
}

static int null_exit(void *desc)
{
    struct null_descriptor *xdesc = (struct null_descriptor *) desc;

    free (xdesc);
    return 0;
}

static bool null_is_compatible_with(uint32_t version) {
    return true;
}
struct ec_backend_op_stubs null_op_stubs = {
    .INIT                       = null_init,
    .EXIT                       = null_exit,
    .ENCODE                     = null_encode,
    .DECODE                     = null_decode,
    .FRAGSNEEDED                = null_min_fragments,
    .RECONSTRUCT                = null_reconstruct,
    .ELEMENTSIZE                = null_element_size,
    .ISCOMPATIBLEWITH           = null_is_compatible_with,
};

struct ec_backend_common backend_null = {
    .id                         = EC_BACKEND_NULL,
    .name                       = NULL_LIB_NAME,
    .soname                     = NULL_SO_NAME,
    .soversion                  = NULL_LIB_VER_STR,
    .ops                        = &null_op_stubs,
    .backend_metadata_size      = 0,
    .ec_backend_version         = _VERSION(NULL_LIB_MAJOR, NULL_LIB_MINOR,
                                           NULL_LIB_REV),
};

