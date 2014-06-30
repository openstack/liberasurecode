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

#include "erasurecode.h"

struct ec_backend_ops flat_xor_3_ops = {
    .init                       = flat_xor_3_init,
    .exit                       = flat_xor_3_exit,
    .open                       = flat_xor_3_open,
    .close                      = flat_xor_3_close,
    .encode                     = flat_xor_3_encode,
    .decode                     = flat_xor_3_decode,
    .reconstruct                = flat_xor_3_reconstruct,
    .get_fragments_needed       = flat_xor_3_min_fragments,
    .get_fragment_metadata      = flat_xor_3_fragment_metadata,
    .verify_fragment_metadata   = flat_xor_3_verify_frag_metadata,
    .verify_stripe_metadata     = flat_xor_3_verify_stripe_metadata,
};

struct ec_backend flat_xor_3 = {
    .id                 = EC_BACKEND_FLAT_XOR_3,
    .name               = "flat_xor_3",
    .soname             = "Xorcode",
    .soversion          = "1.0",
    .handle             = NULL,
    .ops                = &flat_xor_3_ops,
};

static ec_backend_handle_t flat_xor_3_open(void)
{
    
}

static void flat_xor_3_close(ec_backend_handle_t handle)
{
    
}

static int flat_xor_3_init(void)
{
    return liberasurecode_backend_register(&flat_xor_3);
}

static int flat_xor_3_exit(void)
{
    return liberasurecode_backend_unregister(&flat_xor_3);
}
