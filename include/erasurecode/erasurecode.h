/* 
 * Copyright 2014, Tushar Gohad, Kevin Greenan, All rights reserved
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
    EC_BACKEND_NULL                 = 0,
    EC_BACKEND_JERASURE_RS_VAND     = 1,
    EC_BACKEND_JERASURE_RS_CAUCHY   = 2,
    EC_BACKEND_FLAT_XOR_HD          = 3,
    EC_BACKEND_ISA_L_RS_VAND        = 4,
    EC_BACKEND_SHSS                 = 5,
    EC_BACKENDS_MAX,
} ec_backend_id_t;

/* ~=*=~==~=*=~= EC Fragment metadata - supported checksum types ~=*=~==~=*=~ */

/* Checksum types supported for fragment metadata stored in each fragment */
typedef enum {
    CHKSUM_NONE                     = 1,
    CHKSUM_CRC32                    = 2,
    CHKSUM_MD5                      = 3,
    CHKSUM_TYPES_MAX,
} ec_checksum_type_t;

/* =~=*=~==~=*=~== EC Arguments - Common and backend-specific =~=*=~==~=*=~== */

/**
 * Common and backend-specific args
 * to be passed to liberasurecode_instance_create()
 */
struct ec_args {
    int k;                  /* number of data fragments */
    int m;                  /* number of parity fragments */
    int w;                  /* word size, in bits (optional) */
    int hd;                 /* hamming distance (=m for Reed-Solomon) */

    union {
        struct {
            uint64_t arg1;  /* sample arg */
        } null_args;        /* args specific to the null codes */
        struct {
            uint64_t x, y;  /* reserved for future expansion */
            uint64_t z, a;  /* reserved for future expansion */
        } reserved;
    } priv_args1;

    void *priv_args2;       /** flexible placeholder for
                              * future backend args */
    ec_checksum_type_t ct;  /* fragment checksum type */
};

/* =~=*=~==~=*=~== liberasurecode frontend API functions =~=*=~==~=~=*=~==~= */

/* liberasurecode frontend API functions */

/**
 * Create a liberasurecode instance and return a descriptor 
 * for use with EC operations (encode, decode, reconstruct)
 *
 * @param id - one of the supported backends as
 *        defined by ec_backend_id_t
 * @param ec_args - arguments to the EC backend
 *        arguments common to all backends
 *          k - number of data fragments
 *          m - number of parity fragments
 *          w - word size, in bits
 *          hd - hamming distance (=m for Reed-Solomon)
 *          ct - fragment checksum type (stored with the fragment metadata)
 *        backend-specific arguments
 *          null_args - arguments for the null backend
 *          flat_xor_hd, jerasure do not require any special args
 *      
 * @return liberasurecode instance descriptor (int > 0)
 */
int liberasurecode_instance_create(const ec_backend_id_t id,
                                   struct ec_args *args);

/**
 * Close a liberasurecode instance
 *
 * @param desc - liberasurecode descriptor to close
 *
 * @return 0 on success, otherwise non-zero error code
 */
int liberasurecode_instance_destroy(int desc);


/**
 * Erasure encode a data buffer
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param orig_data - data to encode
 * @param orig_data_size - length of data to encode
 * @param encoded_data - pointer to _output_ array (char **) of k data
 *        fragments (char *), allocated by the callee
 * @param encoded_parity - pointer to _output_ array (char **) of m parity
 *        fragments (char *), allocated by the callee
 * @param fragment_len - pointer to _output_ length of each fragment, assuming
 *        all fragments are the same length
 *
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_encode(int desc,
        const char *orig_data, uint64_t orig_data_size, /* input */
        char ***encoded_data, char ***encoded_parity,   /* output */
        uint64_t *fragment_len);                        /* output */

/**
 * Cleanup structures allocated by librasurecode_encode
 *
 * The caller has no context, so cannot safely free memory
 * allocated by liberasurecode, so it must pass the
 * deallocation responsibility back to liberasurecode.
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param encoded_data - (char **) array of k data
 *        fragments (char *), allocated by liberasurecode_encode
 * @param encoded_parity - (char **) array of m parity
 *        fragments (char *), allocated by liberasurecode_encode
 *
 * @return 0 in success; -error otherwise
 */
int liberasurecode_encode_cleanup(int desc, char **encoded_data,
        char **encoded_parity);

/**
 * Reconstruct original data from a set of k encoded fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragments - erasure encoded fragments (> = k)
 * @param num_fragments - number of fragments being passed in
 * @param fragment_len - length of each fragment (assume they are the same)
 * @param force_metadata_checks - force fragment metadata checks (default: 0)
 * @param out_data - _output_ pointer to decoded data
 * @param out_data_len - _output_ length of decoded output
 *          (both output data pointers are allocated by liberasurecode,
 *           caller invokes liberasurecode_decode_clean() after it has
 *           read decoded data in 'out_data')
 *
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        int force_metadata_checks,                      /* input */
        char **out_data, uint64_t *out_data_len);       /* output */

/**
 * Cleanup structures allocated by librasurecode_decode
 *
 * The caller has no context, so cannot safely free memory
 * allocated by liberasurecode, so it must pass the
 * deallocation responsibility back to liberasurecode.
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param data - (char *) buffer of data decoded by librasurecode_decode
 *
 * @return 0 on success; -error otherwise
 */
int liberasurecode_decode_cleanup(int desc, char *data);

/**
 * Reconstruct a missing fragment from a subset of available fragments
 *
 * @param desc - liberasurecode descriptor/handle 
 *        from liberasurecode_instance_create()
 * @param available_fragments - erasure encoded fragments
 * @param num_fragments - number of fragments being passed in
 * @param fragment_len - size in bytes of the fragments
 * @param destination_idx - missing idx to reconstruct
 * @param out_fragment - output of reconstruct
 *
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_reconstruct_fragment(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        int destination_idx,                            /* input */
        char* out_fragment);                            /* output */

/**
 * Return a list of lists with valid rebuild indexes given
 * a list of missing indexes.
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @fragments_to_reconstruct list of indexes to reconstruct
 * @fragments_to_exclude list of indexes to exclude from 
 *        reconstruction equation
 * @fragments_needed list of fragments needed to reconstruct
 *        fragments in fragments_to_reconstruct
 *
 * @return 0 on success, non-zero on error
 */
int liberasurecode_fragments_needed(int desc,
        int *fragments_to_reconstruct, 
        int *fragments_to_exclude,
        int *fragments_needed);


/* ==~=*=~==~=*=~== liberasurecode fragment metadata routines ==~*==~=*=~==~ */

#define LIBERASURECODE_MAX_CHECKSUM_LEN 8
typedef struct __attribute__((__packed__))
fragment_metadata
{
    uint32_t    idx;                /* 4 */
    uint32_t    size;               /* 4 */
    uint32_t    frag_backend_metadata_size;    /* 4 */
    uint64_t    orig_data_size;     /* 8 */
    uint8_t     chksum_type;        /* 1 */
    uint32_t    chksum[LIBERASURECODE_MAX_CHECKSUM_LEN]; /* 32 */
    uint8_t     chksum_mismatch;    /* 1 */
    uint8_t     backend_id;         /* 1 */
    uint32_t    backend_version;    /* 4 */
} fragment_metadata_t;

#define FRAGSIZE_2_BLOCKSIZE(fragment_size) \
    (fragment_size - sizeof(fragment_header_t))

/**
 * Get opaque metadata for a fragment.  The metadata is opaque to the
 * client, but meaningful to the underlying library.  It is used to verify
 * stripes in verify_stripe_metadata().
 *
 * @param fragment - fragment data pointer
 * @param fragment_metadata - pointer to allocated buffer of size at least
 *        sizeof(struct fragment_metadata) to hold fragment metadata struct
 *
 * @return 0 on success, non-zero on error
 */
//EDL: This needs to be implemented
int liberasurecode_get_fragment_metadata(char *fragment,
        fragment_metadata_t *fragment_metadata);

/**
* Verify that the specified pointer points to a well formed fragment that can
* be processed by both this instance of liberasurecode and the specified
* backend.
*
* @param desc - liberasurecode descriptor/handle
*        from liberasurecode_instance_create()
* @param fragment - fragment to verify
*
* @return 1 if fragment validation fails, 0 otherwise.
*/
int is_invalid_fragment(int desc, char *fragment);

/**
 * Verify a subset of fragments generated by encode()
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragments - fragments part of the EC stripe to verify
 * @param num_fragments - number of fragments part of the EC stripe
 *
 * @return 1 if stripe checksum verification is successful, 0 otherwise
 */
int liberasurecode_verify_stripe_metadata(int desc,
        char **fragments, int num_fragments);

/* ==~=*=~===~=*=~==~=*=~== liberasurecode Helpers ==~*==~=*=~==~=~=*=~==~= */

/**
 * This computes the aligned size of a buffer passed into 
 * the encode function.  The encode function must pad fragments
 * to be algined with the word size (w) and the last fragment also
 * needs to be aligned.  This computes the sum of the algined fragment
 * sizes for a given buffer to encode.
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param data_len - original data length in bytes
 *
 * @return aligned length, or -error code on error
 */
int liberasurecode_get_aligned_data_size(int desc, uint64_t data_len);
 
/**
 * This will return the minimum encode size, which is the minimum
 * buffer size that can be encoded.
 * 
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 *
 * @return minimum data length length, or -error code on error
 */
int liberasurecode_get_minimum_encode_size(int desc);

/**
 * This will return the fragment size, which is each fragment data
 * length the backend will allocate when encoding.
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param data_len - original data length in bytes
 *
 * @return fragment size - sizeof(fragment_header) + size
 *                         + frag_backend_metadata_size
 */
int liberasurecode_get_fragment_size(int desc, int data_len);

/* ==~=*=~===~=*=~==~=*=~== liberasurecode Error codes =~=*=~==~=~=*=~==~== */

/* Error codes */
typedef enum {
    EBACKENDNOTSUPP  = 200,
    EECMETHODNOTIMPL = 201,
    EBACKENDINITERR  = 202,
    EBACKENDINUSE    = 203,
    EBACKENDNOTAVAIL = 204,
    EBADCHKSUM       = 205,
    EINVALIDPARAMS   = 206,
    EBADHEADER       = 207,
    EINSUFFFRAGS     = 208,
} LIBERASURECODE_ERROR_CODES;

/* =~=*=~==~=*=~==~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~= */

#ifdef __cplusplus
}
#endif

#endif  // _ERASURECODE_H_
