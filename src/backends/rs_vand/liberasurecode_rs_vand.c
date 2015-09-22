/* 
 * Copyright 2015 Kevin M Greenan
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
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"

#define LIBERASURECODE_RS_VAND_LIB_MAJOR 1
#define LIBERASURECODE_RS_VAND_LIB_MINOR 0
#define LIBERASURECODE_RS_VAND_LIB_REV   0
#define LIBERASURECODE_RS_VAND_LIB_VER_STR "1.0"
#define LIBERASURECODE_RS_VAND_LIB_NAME "liberasurecode_rs_vand"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define LIBERASURECODE_RS_VAND_SO_NAME "liberasurecode_rs_vand.dylib"
#else
#define LIBERASURECODE_RS_VAND_SO_NAME "liberasurecode_rs_vand.so"
#endif

/* Forward declarations */
struct ec_backend_op_stubs liberasurecode_rs_vand_ops;
struct ec_backend liberasurecode_rs_vand;
struct ec_backend_common backend_liberasurecode_rs_vand;

typedef int (*liberasurecode_rs_vand_encode_func)(int *, char **, char **, int, int, int);
typedef int (*liberasurecode_rs_vand_decode_func)(int *, char **, char **, int, int, int *, int, int);
typedef int (*liberasurecode_rs_vand_reconstruct_func)(int *, char **, char **, int, int, int *, int, int);
typedef void (*init_liberasurecode_rs_vand_func)(int, int);
typedef void (*deinit_liberasurecode_rs_vand_func)();
typedef void (*free_systematic_matrix_func)(int *);
typedef int* (*make_systematic_matrix_func)(int, int);


struct liberasurecode_rs_vand_descriptor {
    /* calls required for init */
    init_liberasurecode_rs_vand_func init_liberasurecode_rs_vand; 
    deinit_liberasurecode_rs_vand_func deinit_liberasurecode_rs_vand;
    free_systematic_matrix_func free_systematic_matrix;
    make_systematic_matrix_func make_systematic_matrix;
 
    /* calls required for encode */
    liberasurecode_rs_vand_encode_func liberasurecode_rs_vand_encode;
     
    /* calls required for decode */
    liberasurecode_rs_vand_decode_func liberasurecode_rs_vand_decode; 
    
    /* calls required for reconstruct */
    liberasurecode_rs_vand_reconstruct_func liberasurecode_rs_vand_reconstruct;

    /* fields needed to hold state */
    int *matrix;
    int k;
    int m;
    int w;
};

static int liberasurecode_rs_vand_encode(void *desc, char **data, char **parity,
        int blocksize)
{
    struct liberasurecode_rs_vand_descriptor *rs_vand_desc = 
        (struct liberasurecode_rs_vand_descriptor*) desc;

    /* FIXME: Should this return something? */
    rs_vand_desc->liberasurecode_rs_vand_encode(rs_vand_desc->matrix, data, parity,
        rs_vand_desc->k, rs_vand_desc->m, blocksize);
    return 0;
}

static int liberasurecode_rs_vand_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    struct liberasurecode_rs_vand_descriptor *rs_vand_desc = 
        (struct liberasurecode_rs_vand_descriptor*) desc;

    /* FIXME: Should this return something? */
    rs_vand_desc->liberasurecode_rs_vand_decode(rs_vand_desc->matrix, data, parity, 
        rs_vand_desc->k, rs_vand_desc->m, missing_idxs, blocksize, 1);

    return 0;
}

static int liberasurecode_rs_vand_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    struct liberasurecode_rs_vand_descriptor *rs_vand_desc = 
        (struct liberasurecode_rs_vand_descriptor*) desc;

    /* FIXME: Should this return something? */
    rs_vand_desc->liberasurecode_rs_vand_reconstruct(rs_vand_desc->matrix, data, parity, 
        rs_vand_desc->k, rs_vand_desc->m, missing_idxs, destination_idx, blocksize);

    return 0;
}

static int liberasurecode_rs_vand_min_fragments(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed)
{
    struct liberasurecode_rs_vand_descriptor *rs_vand_desc = 
        (struct liberasurecode_rs_vand_descriptor*)desc;

    uint64_t exclude_bm = convert_list_to_bitmap(fragments_to_exclude);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs) | exclude_bm;
    int i;
    int j = 0;
    int ret = -1;

    for (i = 0; i < (rs_vand_desc->k + rs_vand_desc->m); i++) {
        if (!(missing_bm & (1 << i))) {
            fragments_needed[j] = i;
            j++;
        }
        if (j == rs_vand_desc->k) {
            ret = 0;
            fragments_needed[j] = -1;
            break;
        }
    }

    return ret;
}

static void * liberasurecode_rs_vand_init(struct ec_backend_args *args,
        void *backend_sohandle)
{
    struct liberasurecode_rs_vand_descriptor *desc = NULL;
    
    desc = (struct liberasurecode_rs_vand_descriptor *)
           malloc(sizeof(struct liberasurecode_rs_vand_descriptor));
    if (NULL == desc) {
        return NULL;
    }

    desc->k = args->uargs.k;
    desc->m = args->uargs.m;

    /* store w back in args so upper layer can get to it */
    args->uargs.w = desc->w = 16; // w is currently hard-coded at 16

    // This check should not matter, since 64K is way higher
    // than anyone should ever use
    if ((desc->k + desc->m) > 65536) {
        goto error;
    }

     /*
     * ISO C forbids casting a void* to a function pointer.
     * Since dlsym return returns a void*, we use this union to
     * "transform" the void* to a function pointer.
     */
    union {
        init_liberasurecode_rs_vand_func initp;
        deinit_liberasurecode_rs_vand_func deinitp;
        free_systematic_matrix_func freematrixp;
        make_systematic_matrix_func makematrixp;
        liberasurecode_rs_vand_encode_func encodep;
        liberasurecode_rs_vand_decode_func decodep;
        liberasurecode_rs_vand_reconstruct_func reconstructp;
        void *vptr;
    } func_handle = {.vptr = NULL};


    /* fill in function addresses */
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "init_liberasurecode_rs_vand");
    desc->init_liberasurecode_rs_vand = func_handle.initp;
    if (NULL == desc->init_liberasurecode_rs_vand) {
        goto error; 
    }
    
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "deinit_liberasurecode_rs_vand");
    desc->deinit_liberasurecode_rs_vand = func_handle.deinitp;
    if (NULL == desc->deinit_liberasurecode_rs_vand) {
        goto error; 
    }
    
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "make_systematic_matrix");
    desc->make_systematic_matrix = func_handle.makematrixp;
    if (NULL == desc->make_systematic_matrix) {
        goto error; 
    }
    
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "free_systematic_matrix");
    desc->free_systematic_matrix = func_handle.freematrixp;
    if (NULL == desc->free_systematic_matrix) {
        goto error; 
    }
    
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "liberasurecode_rs_vand_encode");
    desc->liberasurecode_rs_vand_encode = func_handle.encodep;
    if (NULL == desc->liberasurecode_rs_vand_encode) {
        goto error; 
    }
    
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "liberasurecode_rs_vand_decode");
    desc->liberasurecode_rs_vand_decode = func_handle.decodep;
    if (NULL == desc->liberasurecode_rs_vand_decode) {
        goto error; 
    }
    
    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "liberasurecode_rs_vand_reconstruct");
    desc->liberasurecode_rs_vand_reconstruct = func_handle.reconstructp;
    if (NULL == desc->liberasurecode_rs_vand_reconstruct) {
        goto error; 
    }
  
    desc->init_liberasurecode_rs_vand(desc->k, desc->m);

    desc->matrix = desc->make_systematic_matrix(desc->k, desc->m);
            
    if (NULL == desc->matrix) {
        goto error; 
    }

    return desc;

error:
    free(desc);
    
    return NULL;
}

/**
 * Return the element-size, which is the number of bits stored 
 * on a given device, per codeword.  For Vandermonde, this is 
 * 'w'.  For somthing like cauchy, this is packetsize * w. 
 * 
 * Returns the size in bits!
 */
static int
liberasurecode_rs_vand_element_size(void* desc)
{
    struct liberasurecode_rs_vand_descriptor *rs_vand_desc = NULL;
    
    rs_vand_desc = (struct liberasurecode_rs_vand_descriptor*) desc;

    return rs_vand_desc->w;
}

static int liberasurecode_rs_vand_exit(void *desc)
{
    struct liberasurecode_rs_vand_descriptor *rs_vand_desc = NULL;
    
    rs_vand_desc = (struct liberasurecode_rs_vand_descriptor*) desc;

    rs_vand_desc->free_systematic_matrix(rs_vand_desc->matrix);
    rs_vand_desc->deinit_liberasurecode_rs_vand();
    free(rs_vand_desc);

    return 0;
}

/*
 * For the time being, we only claim compatibility with versions that
 * match exactly
 */
static bool liberasurecode_rs_vand_is_compatible_with(uint32_t version) {
    return version == backend_liberasurecode_rs_vand.ec_backend_version;
}

struct ec_backend_op_stubs liberasurecode_rs_vand_op_stubs = {
    .INIT                       = liberasurecode_rs_vand_init,
    .EXIT                       = liberasurecode_rs_vand_exit,
    .ENCODE                     = liberasurecode_rs_vand_encode,
    .DECODE                     = liberasurecode_rs_vand_decode,
    .FRAGSNEEDED                = liberasurecode_rs_vand_min_fragments,
    .RECONSTRUCT                = liberasurecode_rs_vand_reconstruct,
    .ELEMENTSIZE                = liberasurecode_rs_vand_element_size,
    .ISCOMPATIBLEWITH           = liberasurecode_rs_vand_is_compatible_with,
};

struct ec_backend_common backend_liberasurecode_rs_vand = {
    .id                         = EC_BACKEND_LIBERASURECODE_RS_VAND,
    .name                       = LIBERASURECODE_RS_VAND_LIB_NAME,
    .soname                     = LIBERASURECODE_RS_VAND_SO_NAME,
    .soversion                  = LIBERASURECODE_RS_VAND_LIB_VER_STR,
    .ops                        = &liberasurecode_rs_vand_op_stubs,
    .backend_metadata_size      = 0,
    .ec_backend_version         = _VERSION(LIBERASURECODE_RS_VAND_LIB_MAJOR,
                                           LIBERASURECODE_RS_VAND_LIB_MINOR,
                                           LIBERASURECODE_RS_VAND_LIB_REV),
};
