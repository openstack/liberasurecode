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
 */

#include <stdio.h>
#include <stdlib.h>
#include <xor_code.h>

#include "erasurecode.h"
#include "erasurecode_backend.h"

/* Forward declarations */
struct ec_backend_op_stubs flat_xor_hd_ops;
struct ec_backend flat_xor_hd;

static int flat_xor_hd_encode(void *desc, int (*fptr)(),
                              struct ec_backend_args *args,
                              char **data, char **parity, int blocksize)
{
    xor_code_t *xor_desc = (xor_code_t *) desc;
    xor_desc->encode(xor_desc, data, parity, blocksize);
}

static int flat_xor_hd_decode(void *desc, int (*fptr)(),
                              struct ec_backend_args *args,
                              char **data, char **parity, int *missing_idxs,
                              int blocksize)
{
    xor_code_t *xor_desc = (xor_code_t *) desc;
    xor_desc->decode(xor_desc, data, parity, missing_idxs, blocksize, 1);
}

static int flat_xor_hd_reconstruct(void *desc, int (*fptr)(),
                                   struct ec_backend_args *args,
                                   char **data, char **parity, int *missing_idxs,
                                   int destination_idx, int blocksize)
{
    xor_code_t *xor_desc = (xor_code_t *) desc;
    (*fptr)(xor_desc, data, parity, missing_idxs, destination_idx, blocksize);
}

static int flat_xor_hd_min_fragments(void *desc, int (*fptr)(),
                                     struct ec_backend_args *args,
                                     int *missing_idxs, int *fragments_needed)
{
    xor_code_t *xor_desc = (xor_code_t *) desc;
    xor_desc->fragments_needed(xor_desc, missing_idxs, fragments_needed);
}

static void * flat_xor_hd_init(struct ec_backend_args *args)
{
    int k = args->uargs->k;
    int m = args->uargs->m;
    int hd = args->uargs->priv_args1.flat_xor_hd_args.hd;
    void *desc = (void *) init_xor_hd_code(k, m, hd);

    return desc;
}

static int flat_xor_hd_exit(void *desc)
{
    free((xor_code_t *) desc);
}

struct ec_backend_op_stubs flat_xor_hd_op_stubs = {
    .INIT                       = flat_xor_hd_init,
    .EXIT                       = flat_xor_hd_exit,
    .ENCODE                     = flat_xor_hd_encode,
    .DECODE                     = flat_xor_hd_decode,
    .FRAGSNEEDED                = flat_xor_hd_min_fragments,
    .RECONSTRUCT                = flat_xor_hd_reconstruct,
};

struct ec_backend_fnmap flat_xor_hd_fn_map[MAX_FNS] = {
    { FN_NAME(INIT),            "init_xor_hd_code" },
    { FN_NAME(EXIT),            NULL, },
    { FN_NAME(ENCODE),          "encode" },
    { FN_NAME(DECODE),          "decode" },
    { FN_NAME(FRAGSNEEDED),     "fragments_needed", },
    { FN_NAME(RECONSTRUCT),     "xor_reconstruct_one" },
};

struct ec_backend_common backend_flat_xor_hd = {
    .id                         = EC_BACKEND_FLAT_XOR_HD,
    .name                       = "flat_xor_hd",
    .soname                     = "libXorcode.so",
    .soversion                  = "1.0",
    .ops                        = &flat_xor_hd_op_stubs,
    .fnmap                      = flat_xor_hd_fn_map,
    .users                      = 0,
};

