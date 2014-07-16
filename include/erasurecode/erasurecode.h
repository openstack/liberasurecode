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
 * liberasurecode frontend API header
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#ifndef _ERASURECODE_H_
#define _ERASURECODE_H_

#include "list.h"
#include "erasurecode_stdinc.h"
#include "erasurecode_version.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__GNUC__) && __GNUC__ > 3
#define dl_restrict __restrict
#else
#define dl_restrict
#endif

/* =~=*=~==~=*=~==~=*=~= Supported EC backends =~=*=~==~=*=~==~=*=~==~=*=~== */

/* Supported EC backends */
typedef enum {
    EC_BACKEND_NULL                     = 0,
    EC_BACKEND_JERASURE_RS_VAND         = 1,
    EC_BACKEND_JERASURE_RS_CAUCHY       = 2,
    EC_BACKEND_FLAT_XOR_HD              = 3,
    EC_BACKENDS_MAX,
} ec_backend_id_t;

const char *ec_backend_names[EC_BACKENDS_MAX] =
    "null",
    "jerasure_rs_vand",
    "jerasure_rs_cauchy",
    "flat_xor_hd";

/* =~=*=~==~=*=~== EC Arguments - Common and backend-specific =~=*=~==~=*=~== */

/**
 * Common and backend-specific args
 * to be passed to liberasurecode_instance_create()
 */
struct ec_args {
    int k;
    int m;
    union {
        struct {
            int hd;
        } flat_xor_hd_args;
        struct {
            int w;
        } jerasure_args;
    }
    int inline_chksum;
    int algsig_chksum;
}

/* =~=*=~==~=*=~== liberasurecode frontend API functions =~=*=~==~=~=*=~==~= */

/* liberasurecode frontend API functions */

/**
 */
void liberasurecode_supported_backends(char **backend_names);

/**
 */
int liberasurecode_instance_create(const char *backend_name,
                                   sturct ec_args *args);

/**
 */
int liberasurecode_instance_destroy(int desc);


/**
 * Erasure encode a data buffer
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param orig_data - data to encode
 * @param orig_data_size - length of data to encode
 * @param encoded_data - to return k data fragments
 * @param encoded_parity - to return m parity fragments
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_encode(int desc,
        const char *orig_data, uint64_t orig_data_size,
        char **encoded_data, char **encoded_parity);

/**
 * Reconstruct original data from a set of k encoded fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragment_size - size in bytes of the fragments
 * @param encoded_data - erasure encoded data fragments (k)
 * @param encoded_parity - erasure encoded parity fragments (m)
 * @param out_data - output of decode
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc, uint64_t fragment_size, 
        char **encoded_data, char **encoded_parity,
        char *out_data);

/**
 * Reconstruct a missing fragment from a subset of available fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragment_size - size in bytes of the fragments
 * @param encoded_data - erasure encoded data fragments (k)
 * @param encoded_parity - erasure encoded parity fragments (m)
 * @param destination_idx - missing idx to reconstruct
 * @param out_fragment - output of reconstruct
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_reconstruct_fragment(int desc,
        uint64_t fragment_size,
        char **encoded_data, char **encoded_parity,
        int destination_idx, char* out_fragment);

/**
 * Determine which fragments are needed to reconstruct some subset
 * of missing fragments.
 */
int liberasurecode_fragments_needed(int desc,
        int *missing_idxs, int *fragments_needed);

/**
 * Get opaque metadata for a fragment.  The metadata is opaque to the
 * client, but meaningful to the underlying library.  It is used to verify
 * stripes in verify_stripe_metadata().
 */
// int liberasurecode_get_fragment_metadata()


/**
 * Verify a subset of fragments generated by encode()
 */
// int liberasurecode_verify_stripe_metadata()


/* ==~=*=~===~=*=~==~=*=~== liberasurecode Error codes =~=*=~==~=~=*=~==~== */

/* Error codes */
typedef enum {
    EBACKENDNOTSUPP  = 200,
    EECMETHODNOTIMPL = 201,
    EBACKENDINUSE    = 203,
    EBACKENDNOTAVAIL = 204,
} LIBERASURECODE_ERROR_CODES;

/* =~=*=~==~=*=~==~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~= */

#ifdef __cplusplus
}
#endif

#endif  // _ERASURECODE_H_
