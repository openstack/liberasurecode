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
 * liberasurecode flat_xor_hd backend
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>
#include <xor_code.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"

#define FLAT_XOR_LIB_MAJOR 1
#define FLAT_XOR_LIB_MINOR 0
#define FLAT_XOR_LIB_REV   0
#define FLAT_XOR_LIB_VER_STR "1.0"
#define FLAT_XOR_LIB_NAME "flat_xor_hd"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define FLAT_XOR_SO_NAME "libXorcode.dylib"
#else
#define FLAT_XOR_SO_NAME "libXorcode.so.1"
#endif
#define DEFAULT_W 32

/* Forward declarations */
struct ec_backend_op_stubs flat_xor_hd_ops;
struct ec_backend flat_xor_hd;
struct ec_backend_common backend_flat_xor_hd;

typedef xor_code_t* (*init_xor_hd_code_func)(int, int, int);
typedef void (*xor_code_encode_func)(xor_code_t *, char **, char **, int);
typedef int (*xor_code_decode_func)(xor_code_t *, char **, char **, int *, int, int);
typedef int (*xor_hd_fragments_needed_func)(xor_code_t *, int *, int *, int *);

struct flat_xor_hd_descriptor {
    xor_code_t *xor_desc;
    init_xor_hd_code_func init_xor_hd_code;
    xor_code_encode_func  xor_code_encode;
    xor_code_decode_func xor_code_decode;
    xor_hd_fragments_needed_func  xor_hd_fragments_needed;
};

static int flat_xor_hd_encode(void *desc,
                              char **data, char **parity, int blocksize)
{
    struct flat_xor_hd_descriptor *xdesc =
        (struct flat_xor_hd_descriptor *) desc;

    xor_code_t *xor_desc = (xor_code_t *) xdesc->xor_desc;
    xor_desc->encode(xor_desc, data, parity, blocksize);
    return 0;
}

static int flat_xor_hd_decode(void *desc,
                              char **data, char **parity, int *missing_idxs,
                              int blocksize)
{
    struct flat_xor_hd_descriptor *xdesc =
        (struct flat_xor_hd_descriptor *) desc;

    xor_code_t *xor_desc = (xor_code_t *) xdesc->xor_desc;
    return xor_desc->decode(xor_desc, data, parity, missing_idxs, blocksize, 1);
}

static int flat_xor_hd_reconstruct(void *desc,
                                   char **data, char **parity, int *missing_idxs,
                                   int destination_idx, int blocksize)
{
    struct flat_xor_hd_descriptor *xdesc =
        (struct flat_xor_hd_descriptor *) desc;

    xor_code_t *xor_desc = (xor_code_t *) xdesc->xor_desc;
    xor_reconstruct_one(xor_desc, data, parity, 
                          missing_idxs, destination_idx, blocksize);
    return 0;
}

static int flat_xor_hd_min_fragments(void *desc,
                                     int *missing_idxs, int *fragments_to_exclude, 
                                     int *fragments_needed)
{
    struct flat_xor_hd_descriptor *xdesc =
        (struct flat_xor_hd_descriptor *) desc;

    xor_code_t *xor_desc = (xor_code_t *) xdesc->xor_desc;
    xor_desc->fragments_needed(xor_desc, missing_idxs, fragments_to_exclude, fragments_needed);
    return 0;
}

/**
 * Return the element-size, which is the number of bits stored 
 * on a given device, per codeword.  This is usually just 'w'.
 */
static int
flar_xor_hd_element_size(void* desc)
{
    return DEFAULT_W;
}

static void * flat_xor_hd_init(struct ec_backend_args *args, void *sohandle)
{
    int k = args->uargs.k;
    int m = args->uargs.m;
    int hd = args->uargs.hd;

    xor_code_t *xor_desc = NULL;
    struct flat_xor_hd_descriptor *bdesc = NULL;

    /* store w back in args so upper layer can get to it */
    args->uargs.w = DEFAULT_W;

    /* init xor_code_t descriptor */
    xor_desc = init_xor_hd_code(k, m, hd);
    if (NULL == xor_desc) {
        return NULL;
    }

    /* fill in flat_xor_hd_descriptor */
    bdesc = (struct flat_xor_hd_descriptor *)
        malloc(sizeof(struct flat_xor_hd_descriptor));
    if (NULL == bdesc) {
        free (xor_desc);
        return NULL;
    }

    bdesc->xor_desc = xor_desc;

    return (void *) bdesc;
}

static int flat_xor_hd_exit(void *desc)
{
    struct flat_xor_hd_descriptor *bdesc =
        (struct flat_xor_hd_descriptor *) desc;

    free (bdesc->xor_desc);
    free (bdesc);
    return 0;
}

/*
 * For the time being, we only claim compatibility with versions that
 * match exactly
 */
static bool flat_xor_is_compatible_with(uint32_t version) {
    return version == backend_flat_xor_hd.ec_backend_version;
}

struct ec_backend_op_stubs flat_xor_hd_op_stubs = {
    .INIT                       = flat_xor_hd_init,
    .EXIT                       = flat_xor_hd_exit,
    .ENCODE                     = flat_xor_hd_encode,
    .DECODE                     = flat_xor_hd_decode,
    .FRAGSNEEDED                = flat_xor_hd_min_fragments,
    .RECONSTRUCT                = flat_xor_hd_reconstruct,
    .ELEMENTSIZE                = flar_xor_hd_element_size,
    .ISCOMPATIBLEWITH           = flat_xor_is_compatible_with,
};

struct ec_backend_common backend_flat_xor_hd = {
    .id                         = EC_BACKEND_FLAT_XOR_HD,
    .name                       = FLAT_XOR_LIB_NAME,
    .soname                     = FLAT_XOR_SO_NAME,
    .soversion                  = FLAT_XOR_LIB_VER_STR,
    .ops                        = &flat_xor_hd_op_stubs,
    .backend_metadata_size      = 0,
    .ec_backend_version         = _VERSION(FLAT_XOR_LIB_MAJOR,
                                           FLAT_XOR_LIB_MINOR,
                                           FLAT_XOR_LIB_REV),
};

