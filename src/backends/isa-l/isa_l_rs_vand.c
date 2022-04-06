/*
 * Copyright 2014 Kevin M Greenan
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
 * isa_l_rs_vand backend implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdlib.h>
#include "erasurecode_backend.h"
#include "isa_l_common.h"

#define ISA_L_RS_VAND_LIB_MAJOR 2
#define ISA_L_RS_VAND_LIB_MINOR 13
#define ISA_L_RS_VAND_LIB_REV   0
#define ISA_L_RS_VAND_LIB_VER_STR "2.13"
#define ISA_L_RS_VAND_LIB_NAME "isa_l_rs_vand"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define ISA_L_RS_VAND_SO_NAME "libisal" LIBERASURECODE_SO_SUFFIX ".dylib"
#else
#define ISA_L_RS_VAND_SO_NAME "libisal" LIBERASURECODE_SO_SUFFIX ".so.2"
#endif

/* Forward declarations */
struct ec_backend_op_stubs isa_l_rs_vand_ops;
struct ec_backend isa_l_rs_vand;
struct ec_backend_common backend_isa_l_rs_vand;

static void * isa_l_rs_vand_init(struct ec_backend_args *args,
        void *backend_sohandle)
{
    return isa_l_common_init(args, backend_sohandle, "gf_gen_rs_matrix");
}

/*
 * For the time being, we only claim compatibility with versions that
 * match exactly
 */
static bool isa_l_rs_vand_is_compatible_with(uint32_t version) {
    return version == backend_isa_l_rs_vand.ec_backend_version;
}

struct ec_backend_op_stubs isa_l_rs_vand_op_stubs = {
    .INIT                       = isa_l_rs_vand_init,
    .EXIT                       = isa_l_exit,
    .ENCODE                     = isa_l_encode,
    .DECODE                     = isa_l_decode,
    .FRAGSNEEDED                = isa_l_min_fragments,
    .RECONSTRUCT                = isa_l_reconstruct,
    .ELEMENTSIZE                = isa_l_element_size,
    .ISCOMPATIBLEWITH           = isa_l_rs_vand_is_compatible_with,
    .GETMETADATASIZE            = get_backend_metadata_size_zero,
    .GETENCODEOFFSET            = get_encode_offset_zero,
};

struct ec_backend_common backend_isa_l_rs_vand = {
    .id                         = EC_BACKEND_ISA_L_RS_VAND,
    .name                       = ISA_L_RS_VAND_LIB_NAME,
    .soname                     = ISA_L_RS_VAND_SO_NAME,
    .soversion                  = ISA_L_RS_VAND_LIB_VER_STR,
    .ops                        = &isa_l_rs_vand_op_stubs,
    .ec_backend_version         = _VERSION(ISA_L_RS_VAND_LIB_MAJOR,
                                           ISA_L_RS_VAND_LIB_MINOR,
                                           ISA_L_RS_VAND_LIB_REV),
};
