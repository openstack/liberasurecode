/*
 * Copyright 2016 Phazr.IO Inc
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
 * Phazr.IO libphazr backend
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"

#define LIBPHAZR_LIB_MAJOR 1
#define LIBPHAZR_LIB_MINOR 0
#define LIBPHAZR_LIB_REV   0
#define LIBPHAZR_LIB_VER_STR "1.0.0"
#define LIBPHAZR_LIB_NAME "libphazr"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define LIBPHAZR_SO_NAME "libphazr" LIBERASURECODE_SO_SUFFIX ".dylib"
#else
#define LIBPHAZR_SO_NAME "libphazr" LIBERASURECODE_SO_SUFFIX ".so.1"
#endif

/* Forward declarations */
struct ec_backend_common backend_libphazr;

typedef int (*pio_matrix_encode_func)(char *, char *, char **, int, int, int, int, int, int);
typedef int (*pio_matrix_decode_func)(char *, char *, char **, int *, int, int, int, int, int, int);
typedef int (*pio_matrix_reconstruct_func)(char *, char **, int *, int, int, int, int, int, int);
typedef char* (*pio_create_precoding_matrix_func)(int);
typedef char* (*pio_create_inverse_precoding_matrix_func)(int);
typedef char* (*pio_create_kmux_matrix_func)(int, int, int);

struct libphazr_descriptor {
    /* calls required for init */
    pio_create_precoding_matrix_func create_precoding_matrix;
    pio_create_inverse_precoding_matrix_func create_inverse_precoding_matrix;
    pio_create_kmux_matrix_func create_kmux_matrix;

    /* calls required for encode */
    pio_matrix_encode_func matrix_encode;

    /* calls required for decode */
    pio_matrix_decode_func matrix_decode;

    /* calls required for reconstruct */
    pio_matrix_reconstruct_func matrix_reconstruct;

    /* fields needed to hold state */
    char *matrix;
    char *precoding_matrix;
    char *inverse_precoding_matrix;
    int k;
    int m;
    int w;
    int hd;
};

#define DEFAULT_W 64

#define DEFAULT_HD 1

static int get_padded_blocksize(int w, int hd, int blocksize)
{
    int word_size = w / 8;
    return ((blocksize + ((word_size - hd) - 1)) / (word_size - hd)) * word_size;
}

static int pio_matrix_encode(void *desc, char **data, char **parity, int blocksize)
{
    int i, ret = 0;
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor *) desc;
    int padding_size = get_padded_blocksize(xdesc->w, xdesc->hd, blocksize) - blocksize;
    char **encoded = malloc(sizeof(char*) * (xdesc->k + xdesc->m));

    if (NULL == encoded) {
        ret = -ENOMEM;
        goto out;
    }

    for (i = 0; i < xdesc->k; i++) {
        encoded[i] = data[i];
    }

    for (i = 0; i < xdesc->m; i++) {
        encoded[i + xdesc->k] = parity[i];
    }

    ret = xdesc->matrix_encode(xdesc->precoding_matrix, xdesc->matrix, encoded,
        xdesc->k, xdesc->m, xdesc->w, xdesc->hd, blocksize, padding_size);

out:
    free(encoded);

    return ret;
}

static int pio_matrix_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    int i, ret = 0;
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor *) desc;
    int padding_size = get_padded_blocksize(xdesc->w, xdesc->hd, blocksize) - blocksize;
    char **decoded = malloc(sizeof(char*) * (xdesc->k + xdesc->m));

    if (NULL == decoded) {
        ret = -ENOMEM;
        goto out;
    }

    for (i = 0; i < xdesc->k; i++) {
        decoded[i] = data[i];
    }

    for (i = 0; i < xdesc->m; i++) {
        decoded[i + xdesc->k] = parity[i];
    }

    ret = xdesc->matrix_decode(xdesc->inverse_precoding_matrix, xdesc->matrix, decoded,
        missing_idxs, xdesc->k, xdesc->m, xdesc->w, xdesc->hd, blocksize, padding_size);

out:
    free(decoded);

    return ret;
}

static int pio_matrix_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    int i, ret = 0;
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor *) desc;
    int padding_size = get_padded_blocksize(xdesc->w, xdesc->hd, blocksize) - blocksize;
    char **encoded = malloc(sizeof(char*) * (xdesc->k + xdesc->m));

    if (NULL == encoded) {
        ret = -ENOMEM;
        goto out;
    }

    for (i = 0; i < xdesc->k; i++) {
        encoded[i] = data[i];
    }

    for (i = 0; i < xdesc->m; i++) {
        encoded[i + xdesc->k] = parity[i];
    }

    ret = xdesc->matrix_reconstruct(xdesc->matrix, encoded, missing_idxs,
        destination_idx, xdesc->k, xdesc->m, xdesc->w, blocksize, padding_size);

out:
    free(encoded);

    return ret;
}

static int pio_min_fragments(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed)
{
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor *)desc;
    uint64_t exclude_bm = convert_list_to_bitmap(fragments_to_exclude);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs) | exclude_bm;
    int i;
    int j = 0;
    int ret = -1;

    for (i = 0; i < (xdesc->k + xdesc->m); i++) {
        if (!(missing_bm & (1 << i))) {
            fragments_needed[j] = i;
            j++;
        }
        if (j == xdesc->k) {
            ret = 0;
            fragments_needed[j] = -1;
            break;
        }
    }

    return ret;
}

/**
 * Return the element-size, which is the number of bits stored
 * on a given device, per codeword.
 */
static int pio_element_size(void *desc)
{
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor *)desc;

    return xdesc->w;
}

static void * pio_init(struct ec_backend_args *args, void *backend_sohandle)
{
    struct libphazr_descriptor *desc = NULL;

    /* allocate and fill in libphazr_descriptor */
    desc = (struct libphazr_descriptor *)malloc(sizeof(struct libphazr_descriptor));
    if (NULL == desc) {
        return NULL;
    }
    memset(desc, 0, sizeof(struct libphazr_descriptor));

    desc->k = args->uargs.k;
    desc->m = args->uargs.m;
    desc->w = args->uargs.w;
    desc->hd = args->uargs.hd;

    if (desc->w <= 0)
        desc->w = DEFAULT_W;
    args->uargs.w = desc->w;

    if (desc->hd <= 0)
        desc->hd = DEFAULT_HD;
    args->uargs.hd = desc->hd;

    /*
     * ISO C forbids casting a void* to a function pointer.
     * Since dlsym return returns a void*, we use this union to
     * "transform" the void* to a function pointer.
     */
    union {
        pio_create_precoding_matrix_func create_precoding_matrix_ptr;
        pio_create_inverse_precoding_matrix_func create_inverse_precoding_matrix_ptr;
        pio_create_kmux_matrix_func create_kmux_matrix_ptr;
        pio_matrix_encode_func matrix_encode_ptr;
        pio_matrix_decode_func matrix_decode_ptr;
        pio_matrix_reconstruct_func matrix_reconstruct_ptr;
        void *vptr;
    } func_handle = {.vptr = NULL};

    /* fill in function addresses */
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "create_precoding_matrix");
    desc->create_precoding_matrix = func_handle.create_precoding_matrix_ptr;
    if (NULL == desc->create_precoding_matrix) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "create_inverse_precoding_matrix");
    desc->create_inverse_precoding_matrix = func_handle.create_inverse_precoding_matrix_ptr;
    if (NULL == desc->create_inverse_precoding_matrix) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "create_kmux_matrix");
    desc->create_kmux_matrix = func_handle.create_kmux_matrix_ptr;
    if (NULL == desc->create_kmux_matrix) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "matrix_encode");
    desc->matrix_encode = func_handle.matrix_encode_ptr;
    if (NULL == desc->matrix_encode) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "matrix_decode");
    desc->matrix_decode = func_handle.matrix_decode_ptr;
    if (NULL == desc->matrix_decode) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "matrix_reconstruct");
    desc->matrix_reconstruct = func_handle.matrix_reconstruct_ptr;
    if (NULL == desc->matrix_reconstruct) {
        goto error;
    }

    if (NULL == desc->precoding_matrix) {
        desc->precoding_matrix = desc->create_precoding_matrix(desc->k);
        if (NULL == desc->precoding_matrix) {
            goto error;
        }
    }

    if (NULL == desc->inverse_precoding_matrix) {
        desc->inverse_precoding_matrix = desc->create_inverse_precoding_matrix(desc->k);
        if (NULL == desc->inverse_precoding_matrix) {
            goto error;
        }
    }

    if (NULL == desc->matrix) {
        desc->matrix = desc->create_kmux_matrix(desc->k, desc->m, desc->w);
        if (NULL == desc->create_kmux_matrix) {
            goto error;
        }
    }

    return (void *) desc;

error:
    free(desc->matrix);

    free(desc->precoding_matrix);

    free(desc->inverse_precoding_matrix);

    free(desc);

    return NULL;
}

static int pio_exit(void *desc)
{
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor*)desc;

    free(xdesc->matrix);

    free(xdesc->precoding_matrix);

    free(xdesc->inverse_precoding_matrix);

    free(xdesc);

    return 0;
}

static bool pio_is_compatible_with(uint32_t version)
{
    return version == backend_libphazr.ec_backend_version;
}

static size_t pio_get_backend_metadata_size(void *desc, int blocksize)
{
    struct libphazr_descriptor *xdesc = (struct libphazr_descriptor *) desc;
    int padded_blocksize = get_padded_blocksize(xdesc->w, xdesc->hd, blocksize);
    return padded_blocksize - blocksize;
}

static size_t pio_get_encode_offset(void *desc, int metadata_size)
{
    return metadata_size;
}


static struct ec_backend_op_stubs libphazr_op_stubs = {
    .INIT                       = pio_init,
    .EXIT                       = pio_exit,
    .ISSYSTEMATIC               = 0,
    .ENCODE                     = pio_matrix_encode,
    .DECODE                     = pio_matrix_decode,
    .FRAGSNEEDED                = pio_min_fragments,
    .RECONSTRUCT                = pio_matrix_reconstruct,
    .ELEMENTSIZE                = pio_element_size,
    .ISCOMPATIBLEWITH           = pio_is_compatible_with,
    .GETMETADATASIZE            = pio_get_backend_metadata_size,
    .GETENCODEOFFSET            = pio_get_encode_offset,
};

__attribute__ ((visibility ("internal")))
struct ec_backend_common backend_libphazr = {
    .id                         = EC_BACKEND_LIBPHAZR,
    .name                       = LIBPHAZR_LIB_NAME,
    .soname                     = LIBPHAZR_SO_NAME,
    .soversion                  = LIBPHAZR_LIB_VER_STR,
    .ops                        = &libphazr_op_stubs,
    .ec_backend_version         = _VERSION(LIBPHAZR_LIB_MAJOR, LIBPHAZR_LIB_MINOR,
                                           LIBPHAZR_LIB_REV),
};

