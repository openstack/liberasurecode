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
#include "erasurecode_backend.h"

/* Forward declarations */
struct ec_backend_op_stubs flat_xor_3_ops;
struct ec_backend flat_xor_3;

static int flat_xor_3_encode(void * desc)
{
    xor_code_t *xor_desc = desc;
    // xor_desc->encode(xor_desc, data, parity, blocksize);
}

static int flat_xor_3_decode()
{
}

static int flat_xor_3_reconstruct()
{
}

static int flat_xor_3_min_fragments()
{
}

static int flat_xor_3_fragment_metadata()
{
    return 0;
}

static int flat_xor_3_verify_frag_metadata()
{
    return 0;
}

static int flat_xor_3_verify_stripe_metadata()
{
    return 0;
}

static void * flat_xor_3_init(struct ec_backend_args args)
{
    /* hd = 3 for flat_xor_3 */
    const int hd = 3;

    return (void *) init_xor_hd_code(args.k, args.m, hd);
}

static int flat_xor_3_exit(void *desc)
{
    free((xor_code_t *) desc);
}

struct ec_backend_op_stubs flat_xor_3_op_stubs = {
    .INIT                       = flat_xor_3_init,
    .EXIT                       = flat_xor_3_exit,
    .ENCODE                     = flat_xor_3_encode,
    .DECODE                     = flat_xor_3_decode,
    .FRAGSNEEDED                = flat_xor_3_min_fragments,
    .RECONSTRUCT                = flat_xor_3_reconstruct,
};

struct ec_backend_fnmap flat_xor_3_fn_map[] = {
    { FN_NAME(INIT),            "init_xor_hd_code" },
    { FN_NAME(EXIT),            NULL, },
    { FN_NAME(ENCODE),          "encode" },
    { FN_NAME(DECODE),          "decode" },
    { FN_NAME(FRAGSNEEDED),     "fragments_needed", },
    { FN_NAME(RECONSTRUCT),     "xor_reconstruct_one" },
};

struct ec_backend_common backend_flat_xor_3 = {
    .id                         = EC_BACKEND_FLAT_XOR_3,
    .name                       = "flat_xor_3",
    .soname                     = "libXorcode.so",
    .soversion                  = "1.0",
    .ops                        = &flat_xor_3_op_stubs,
    .fnmap                      = flat_xor_3_fn_map,
    .users                      = 0,
};

