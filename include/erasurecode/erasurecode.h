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

/* =~=*=~==~=*=~==~=*=~= Supported EC backends =~=*=~==~=*=~==~=*=~==~=*=~== */

typedef enum {
    EC_BACKEND_NULL                     = 0,
    EC_BACKEND_JERASURE_RS_VAND         = 1,
    EC_BACKEND_JERASURE_RS_CAUCHY       = 2,
    EC_BACKEND_FLAT_XOR_HD              = 3,
    EC_BACKENDS_MAX,
} ec_backend_id_t;

#ifndef EC_BACKENDS_SUPPORTED
#define EC_BACKENDS_SUPPORTED
/* Supported EC backends */
static const char *ec_backend_names[EC_BACKENDS_MAX] = {
    "null",
    "jerasure_rs_vand",
    "jerasure_rs_cauchy",
    "flat_xor_hd",
};
#endif // EC_BACKENDS_SUPPORTED

/* =~=*=~==~=*=~== EC Arguments - Common and backend-specific =~=*=~==~=*=~== */

/**
 * Common and backend-specific args
 * to be passed to liberasurecode_instance_create()
 */
struct ec_args {
    int k;                  /* number of data fragments */
    int m;                  /* number of parity fragments */
    int w;                  /* word size, in bits (optional) */

    union {
        struct {
            int hd;         /* hamming distance (3 or 4) */
        } flat_xor_hd_args; /* args specific to XOR codes */
        struct {
            uint64_t x, y;  /* reserved for future expansion */
            uint64_t z, a;  /* reserved for future expansion */
        } reserved;
    } priv_args1;

    void *priv_args2;       /** flexible placeholder for
                              * future backend args */

    int inline_chksum;      /* embedded fragment checksums (yes/no), type */
    int algsig_chksum;      /* use algorithmic signature checksums */
};

/* =~=*=~==~=*=~== liberasurecode frontend API functions =~=*=~==~=~=*=~==~= */

/* liberasurecode frontend API functions */

/**
 * Returns a list of supported EC backend names
 */
void liberasurecode_supported_backends(char **backend_names);

/**
 * Create a liberasurecode instance and return a descriptor 
 * for use with EC operations (encode, decode, reconstruct)
 *
 * @param backend_name - one of the supported backends as
 *        defined by ec_backend_names
 * @param ec_args - arguments to the EC backend
 *        arguments common to all backends
 *          k - number of data fragments
 *          m - number of parity fragments
 *          inline_checksum - 
 *          algsig_checksum -
 *        backend-specific arguments
 *          flat_xor_hd_args - arguments for the xor_hd backend
 *          jerasure_args - arguments for the Jerasure backend
 *      
 * @returns liberasurecode instance descriptor (int > 0)
 */
int liberasurecode_instance_create(const char *backend_name,
                                   struct ec_args *args);

/**
 * Close a liberasurecode instance
 *
 * @param liberasurecode descriptor to close
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
 * @param fragments - erasure encoded fragments (> = k)
 * @param num_fragments - number of fragments being passed in
 * @param fragment_len - length of each fragment (assume they are the same)
 * @param out_data - output of decode
 * @param out_data_len - length of decoded output
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc,
        char **available_fragments,
        int32_t num_fragments,
        uint64_t fragment_len,
        char *out_data,
        int32_t *out_data_len);

/**
 * Reconstruct a missing fragment from a subset of available fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragment_size - size in bytes of the fragments
 * @param available_fragments - erasure encoded fragments
 * @param destination_idx - missing idx to reconstruct
 * @param out_fragment - output of reconstruct
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_reconstruct_fragment(int desc,
        uint64_t fragment_size,
        char **available_fragments, char **encoded_parity,
        int destination_idx, char* out_fragment);

/**
 * Determine which fragments are needed to reconstruct some subset
 * of missing fragments.  Returns a list of lists (as bitmaps)
 * of fragments required to reconstruct missing indexes.
 */
int liberasurecode_fragments_needed(int desc,
        int *missing_idxs, int *fragments_needed);

/**
 * Get opaque metadata for a fragment.  The metadata is opaque to the
 * client, but meaningful to the underlying library.  It is used to verify
 * stripes in verify_stripe_metadata().
 */
int liberasurecode_get_fragment_metadata(char *fragment);


/**
 * Verify a subset of fragments generated by encode()
 */
int liberasurecode_verify_stripe_metadata(char **fragments);


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
