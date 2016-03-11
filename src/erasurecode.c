/*
 * Copyright 2014 Tushar Gohad, Kevin M Greenan, Eric Lambert, Mark Storer
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
 * liberasurecode API implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include "assert.h"
#include "list.h"
#include "erasurecode.h"
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"
#include "erasurecode_preprocessing.h"
#include "erasurecode_postprocessing.h"
#include "erasurecode_stdinc.h"

#include "alg_sig.h"
#include "erasurecode_log.h"

/* =~=*=~==~=*=~==~=*=~= Supported EC backends =~=*=~==~=*=~==~=*=~==~=*=~== */

/* EC backend references */
extern struct ec_backend_common backend_null;
extern struct ec_backend_common backend_flat_xor_hd;
extern struct ec_backend_common backend_jerasure_rs_vand;
extern struct ec_backend_common backend_jerasure_rs_cauchy;
extern struct ec_backend_common backend_isa_l_rs_vand;
extern struct ec_backend_common backend_shss;
extern struct ec_backend_common backend_liberasurecode_rs_vand;

ec_backend_t ec_backends_supported[] = {
    (ec_backend_t) &backend_null,
    (ec_backend_t) &backend_jerasure_rs_vand,
    (ec_backend_t) &backend_jerasure_rs_cauchy,
    (ec_backend_t) &backend_flat_xor_hd,
    (ec_backend_t) &backend_isa_l_rs_vand,
    (ec_backend_t) &backend_shss,
    (ec_backend_t) &backend_liberasurecode_rs_vand,
    NULL,
};

/* backend list to return to the caller */
int num_supported_backends = 0;
char *ec_backends_supported_str[EC_BACKENDS_MAX];

/* =~=*=~==~=*=~==~=*=~= EC backend instance management =~=*=~==~=*=~==~=*= */

/* Registered erasure code backend instances */
SLIST_HEAD(backend_list, ec_backend) active_instances =
    SLIST_HEAD_INITIALIZER(active_instances);
rwlock_t active_instances_rwlock = RWLOCK_INITIALIZER;

/* Backend instance id */
int next_backend_desc = 0;

/**
 * Look up a backend instance by descriptor
 *
 * @returns pointer to a registered liberasurecode instance
 * The caller must hold active_instances_rwlock
 */
ec_backend_t liberasurecode_backend_instance_get_by_desc(int desc)
{
    struct ec_backend *b = NULL;
    SLIST_FOREACH(b, &active_instances, link) {
        if (b->idesc == desc)
            break;
    }
    return b;
}

/**
 * Allocated backend instance descriptor
 *
 * Returns a unique descriptor for a new backend.
 * The caller must hold active_instances_rwlock
 */
int liberasurecode_backend_alloc_desc(void)
{
    for (;;) {
        if (++next_backend_desc <= 0)
            next_backend_desc = 1;
        if (!liberasurecode_backend_instance_get_by_desc(next_backend_desc))
            return next_backend_desc;
    }
}

/**
 * Register a backend instance with liberasurecode
 *
 * @param instance - backend enum
 *
 * @returns new backend descriptor
 */
int liberasurecode_backend_instance_register(ec_backend_t instance)
{
    int desc = -1;  /* descriptor to return */
    int rc = 0;     /* return call value */

    rc = rwlock_wrlock(&active_instances_rwlock);
    if (rc == 0) {
        SLIST_INSERT_HEAD(&active_instances, instance, link);
        desc = liberasurecode_backend_alloc_desc();
        if (desc <= 0)
            goto register_out;
        instance->idesc = desc;
    } else {
        goto exit;
    }

register_out:
    rwlock_unlock(&active_instances_rwlock);
exit:
    return desc;
}

/**
 * Unregister a backend instance
 *
 * @returns 0 on success, non-0 on error
 */
int liberasurecode_backend_instance_unregister(ec_backend_t instance)
{
    int rc = 0;  /* return call value */

    rc = rwlock_wrlock(&active_instances_rwlock);
    if (rc == 0) {
        SLIST_REMOVE(&active_instances, instance, ec_backend, link);
    }  else {
        goto exit;
    }
    rwlock_unlock(&active_instances_rwlock);

exit:
    return rc;
}

/* =~=*=~==~=*=~== liberasurecode backend API helpers =~=*=~==~=*=~== */

static void print_dlerror(const char *caller)
{
    char *msg = dlerror();
    if (NULL == msg)
        log_error("%s: unknown dynamic linking error\n", caller);
    else
        log_error("%s: dynamic linking error %s\n", caller, msg);
}

/* Generic dlopen/dlclose routines */
void* liberasurecode_backend_open(ec_backend_t instance)
{
    if (NULL == instance)
        return NULL;
    /* Use RTLD_LOCAL to avoid symbol collisions */
    return dlopen(instance->common.soname, RTLD_LAZY | RTLD_LOCAL);
}

int liberasurecode_backend_close(ec_backend_t instance)
{
    if (NULL == instance || NULL == instance->desc.backend_sohandle)
        return 0;

    dlclose(instance->desc.backend_sohandle);
    dlerror();    /* Clear any existing errors */

    instance->desc.backend_sohandle = NULL;
    return 0;
}

/* =*=~==~=*=~==~=*=~= liberasurecode init/exit routines =~=*=~==~=*=~==~=*= */

void __attribute__ ((constructor))
liberasurecode_init(void) {
    /* init logging */
    openlog("liberasurecode", LOG_PID | LOG_CONS, LOG_USER);

    /* populate supported backends list as a string */
    {
        int i;
        for (i = 0; ec_backends_supported[i]; ++i) {
            ec_backends_supported_str[i] = strdup(
                    ec_backends_supported[i]->common.name);
        }
        num_supported_backends = i;
    }
}

void __attribute__ ((destructor))
liberasurecode_exit(void) {
    int i;
    for (i = 0; i < num_supported_backends; ++i)
        free(ec_backends_supported_str[i]);
    closelog();
}

/* =~=*=~==~=*=~= liberasurecode frontend API implementation =~=*=~==~=*=~== */

/**
 * Checks if a given backend is available.
 *
 * @param backend_id - one of the supported backends.
 *
 * @returns 1 if a backend is available; 0 otherwise
 */
int liberasurecode_backend_available(const ec_backend_id_t backend_id) {
    struct ec_backend backend;
    if (backend_id >= EC_BACKENDS_MAX)
        return 0;

    backend.desc.backend_sohandle = liberasurecode_backend_open(
            ec_backends_supported[backend_id]);
    if (!backend.desc.backend_sohandle) {
        return 0;
    }

    liberasurecode_backend_close(&backend);
    return 1;
}

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
 * @returns liberasurecode instance descriptor (int > 0)
 */
int liberasurecode_instance_create(const ec_backend_id_t id,
                                   struct ec_args *args)
{
    ec_backend_t instance = NULL;
    struct ec_backend_args bargs;
    if (!args)
        return -EINVALIDPARAMS;

    if (id >= EC_BACKENDS_MAX)
        return -EBACKENDNOTSUPP;

    if ((args->k + args->m) > EC_MAX_FRAGMENTS) {
        log_error("Total number of fragments (k + m) must be less than %d\n",
                  EC_MAX_FRAGMENTS);
        return -EINVALIDPARAMS;
    }

    /* Allocate memory for ec_backend instance */
    instance = calloc(1, sizeof(*instance));
    if (NULL == instance)
        return -ENOMEM;

    /* Copy common backend, args struct */
    instance->common = ec_backends_supported[id]->common;
    memcpy(&(bargs.uargs), args, sizeof (struct ec_args));
    instance->args = bargs;

    /* Open backend .so if not already open */
    /* .so handle is returned in instance->desc.backend_sohandle */
    if (!instance->desc.backend_sohandle) {
        instance->desc.backend_sohandle = liberasurecode_backend_open(instance);
        if (!instance->desc.backend_sohandle) {
            /* ignore during init, return the same handle */
            print_dlerror(__func__);
            free(instance);
            return -EBACKENDNOTAVAIL;
        }
    }

    /* Call private init() for the backend */
    instance->desc.backend_desc = instance->common.ops->init(
            &instance->args, instance->desc.backend_sohandle);
    if (NULL == instance->desc.backend_desc) {
        free (instance);
        return -EBACKENDINITERR;
    }

    /* Register instance and return a descriptor/instance id */
    instance->idesc = liberasurecode_backend_instance_register(instance);

    return instance->idesc;
}

/**
 * Close a liberasurecode instance
 *
 * @param liberasurecode descriptor to close
 */
int liberasurecode_instance_destroy(int desc)
{
    ec_backend_t instance = NULL;  /* instance to destroy */
    int rc = 0;                    /* return code */

    instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance)
        return -EBACKENDNOTAVAIL;

    /* Call private exit() for the backend */
    instance->common.ops->exit(instance->desc.backend_desc);

    /* dlclose() backend library */
    liberasurecode_backend_close(instance);

    /* Remove instace from registry */
    rc = liberasurecode_backend_instance_unregister(instance);
    if (rc == 0) {
        free(instance);
    }

    return rc;
}

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
 * @return 0 in success; -error otherwise
 */
int liberasurecode_encode_cleanup(int desc,
                                  char **encoded_data,
                                  char **encoded_parity)
{
    int i, k, m;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        return -EBACKENDNOTAVAIL;
    }

    k = instance->args.uargs.k;
    m = instance->args.uargs.m;

    if (encoded_data) {
        for (i = 0; i < k; i++) {
            free(encoded_data[i]);
        }

        free(encoded_data);
    }

    if (encoded_parity) {
        for (i = 0; i < m; i++) {
            free(encoded_parity[i]);
        }
        free(encoded_parity);
    }

    return 0;
}

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
        uint64_t *fragment_len)                         /* output */
{
    int k, m;
    int ret = 0;            /* return code */

    int blocksize = 0;      /* length of each of k data elements */

    if (orig_data == NULL) {
        log_error("Pointer to data buffer is null!");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    if (encoded_data == NULL) {
        log_error("Pointer to encoded data buffers is null!");
        return -EINVALIDPARAMS;
    }

    if (encoded_parity == NULL) {
        log_error("Pointer to encoded parity buffers is null!");
        return -EINVALIDPARAMS;
    }

    if (fragment_len == NULL) {
        log_error("Pointer to fragment length is null!");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out;
    }

    k = instance->args.uargs.k;
    m = instance->args.uargs.m;

    /*
     * Allocate arrays for data, parity and missing_idxs
     */
    *encoded_data = (char **) alloc_zeroed_buffer(sizeof(char *) * k);
    if (NULL == *encoded_data) {
        log_error("Could not allocate data buffer!");
        goto out;
    }

    *encoded_parity = (char **) alloc_zeroed_buffer(sizeof(char *) * m);
    if (NULL == *encoded_parity) {
        log_error("Could not allocate parity buffer!");
        goto out;
    }

    ret = prepare_fragments_for_encode(instance, k, m, orig_data, orig_data_size,
                                       *encoded_data, *encoded_parity, &blocksize);
    if (ret < 0) {
        // ensure encoded_data/parity point the head of fragment_ptr
        get_fragment_ptr_array_from_data(*encoded_data, *encoded_data, k);
        get_fragment_ptr_array_from_data(*encoded_parity, *encoded_parity, m);
        goto out;
    }

    /* call the backend encode function passing it desc instance */
    ret = instance->common.ops->encode(instance->desc.backend_desc,
                                       *encoded_data, *encoded_parity, blocksize);
    if (ret < 0) {
        // ensure encoded_data/parity point the head of fragment_ptr
        get_fragment_ptr_array_from_data(*encoded_data, *encoded_data, k);
        get_fragment_ptr_array_from_data(*encoded_parity, *encoded_parity, m);
        goto out;
    }

    ret = finalize_fragments_after_encode(instance, k, m, blocksize, orig_data_size,
                                          *encoded_data, *encoded_parity);

    *fragment_len = get_fragment_size((*encoded_data)[0]);

out:
    if (ret) {
        /* Cleanup the allocations we have done */
        liberasurecode_encode_cleanup(desc, *encoded_data, *encoded_parity);
        log_error("Error in liberasurecode_encode %d", ret);
    }
    return ret;
}

/**
 * Cleanup structures allocated by librasurecode_decode
 *
 * The caller has no context, so cannot safely free memory
 * allocated by liberasurecode, so it must pass the
 * deallocation responsibility back to liberasurecode.
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param data - (char *) buffer of data decoded by
 *        librasurecode_decode
 * @return 0 in success; -error otherwise
 */
int liberasurecode_decode_cleanup(int desc, char *data)
{
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        return -EBACKENDNOTAVAIL;
    }

    free(data);

    return 0;
}

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
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        int force_metadata_checks,                      /* input */
        char **out_data, uint64_t *out_data_len)        /* output */
{
    int i, j;
    int ret = 0;

    int k = -1, m = -1;
    int orig_data_size = 0;

    int blocksize = 0;
    char **data = NULL;
    char **parity = NULL;
    char **data_segments = NULL;
    char **parity_segments = NULL;
    int *missing_idxs = NULL;

    uint64_t realloc_bm = 0;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out;
    }

    if (NULL == available_fragments) {
        log_error("Pointer to encoded fragments buffer is null!");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    if (NULL == out_data) {
        log_error("Pointer to decoded data buffer is null!");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    if (NULL == out_data_len) {
        log_error("Pointer to decoded data length variable is null!");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    k = instance->args.uargs.k;
    m = instance->args.uargs.m;

    if (num_fragments < k) {
        log_error("Not enough fragments to decode, got %d, need %d!",
                  num_fragments, k);
        ret = -EINSUFFFRAGS;
        goto out;
    }

    for (i = 0; i < num_fragments; ++i) {
        /* Verify metadata checksum */
        if (is_invalid_fragment_header(
                (fragment_header_t *) available_fragments[i])) {
            log_error("Invalid fragment header information!");
            ret = -EBADHEADER;
            goto out;
        }
    }

    if (instance->common.id != EC_BACKEND_SHSS) {
        /* shss (ntt_backend) must force to decode */
        // TODO: Add a frag and function to handle whether the backend want to decode or not.
        /*
         * Try to re-assebmle the original data before attempting a decode
         */
        ret = fragments_to_string(k, m,
                                  available_fragments, num_fragments,
                                  out_data, out_data_len);

        if (ret == 0) {
            /* We were able to get the original data without decoding! */
            goto out;
        }
    }

    /*
     * Allocate arrays for data, parity and missing_idxs
     */
    data = alloc_zeroed_buffer(sizeof(char*) * k);
    if (NULL == data) {
        log_error("Could not allocate data buffer!");
        goto out;
    }

    parity = alloc_zeroed_buffer(sizeof(char*) * m);
    if (NULL == parity) {
        log_error("Could not allocate parity buffer!");
        goto out;
    }

    missing_idxs = alloc_and_set_buffer(sizeof(char*) * (k + m), -1);
    if (NULL == missing_idxs) {
        log_error("Could not allocate missing_idxs buffer!");
        goto out;
    }

    /* If metadata checks requested, check fragment integrity upfront */
    if (force_metadata_checks) {
        int num_invalid_fragments = 0;
        for (i = 0; i < num_fragments; ++i) {
            if (is_invalid_fragment(desc, available_fragments[i])) {
                ++num_invalid_fragments;
            }
        }
        if ((num_fragments - num_invalid_fragments) < k) {
            ret = -EINSUFFFRAGS;
            log_error("Not enough valid fragments available for decode!");
            goto out;
        }
    }

    /*
     * Separate the fragments into data and parity.  Also determine which
     * pieces are missing.
     */
    ret = get_fragment_partition(k, m, available_fragments, num_fragments,
            data, parity, missing_idxs);

    if (ret < 0) {
        log_error("Could not properly partition the fragments!");
        goto out;
    }

    /*
     * Preparing the fragments for decode.  This will alloc aligned buffers
     * when unaligned buffers were passed in available_fragments.  It passes
     * back a bitmap telling us which buffers need to be freed by us
     * (realloc_bm).
     *
     */
    ret = prepare_fragments_for_decode(k, m,
                                       data, parity, missing_idxs, 
                                       &orig_data_size, &blocksize,
                                       fragment_len, &realloc_bm);
    if (ret < 0) {
        log_error("Could not prepare fragments for decode!");
        goto out;
    }

    data_segments = alloc_zeroed_buffer(k * sizeof(char *));
    parity_segments = alloc_zeroed_buffer(m * sizeof(char *));
    get_data_ptr_array_from_fragments(data_segments, data, k);
    get_data_ptr_array_from_fragments(parity_segments, parity, m);

    /* call the backend decode function passing it desc instance */
    ret = instance->common.ops->decode(instance->desc.backend_desc,
                                       data_segments, parity_segments,
                                       missing_idxs, blocksize);

    if (ret < 0) {
        log_error("Encountered error in backend decode function!");
        goto out;
    }

    /*
     * Need to fill in the missing data headers so we can generate
     * the original string.
     */
    j = 0;
    while (missing_idxs[j] >= 0) {
        int set_chksum = 1;
        int missing_idx = missing_idxs[j];
        if (missing_idx < k) {
            /* Generate headers */
            char *fragment_ptr = data[missing_idx];
            init_fragment_header(fragment_ptr);
            add_fragment_metadata(instance, fragment_ptr, missing_idx,
                    orig_data_size, blocksize, instance->args.uargs.ct,
                    !set_chksum);
        }
        j++;
    }

    /* Try to generate the original string */
    ret = fragments_to_string(k, m, data, k, out_data, out_data_len);

    if (ret < 0) {
        log_error("Could not convert decoded fragments to a string!");
    }

out:
    /* Free the buffers allocated in prepare_fragments_for_decode */
    if (realloc_bm != 0) {
        for (i = 0; i < k; i++) {
            if (realloc_bm & (1 << i)) {
                free(data[i]);
            }
        }

        for (i = 0; i < m; i++) {
            if (realloc_bm & (1 << (i + k))) {
                free(parity[i]);
            }
        }
    }

    free(data);
    free(parity);
    free(missing_idxs);
    free(data_segments);
    free(parity_segments);

    return ret;
}

/**
 * Reconstruct a missing fragment from a subset of available fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragment_len - size in bytes of the fragments
 * @param available_fragments - erasure encoded fragments
 * @param num_fragments - number of fragments being passed in
 * @param destination_idx - missing idx to reconstruct
 * @param out_fragment - output of reconstruct
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_reconstruct_fragment(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        int destination_idx,                            /* input */
        char* out_fragment)                             /* output */
{
    int ret = 0;
    int blocksize = 0;
    int orig_data_size = 0;
    char **data = NULL;
    char **parity = NULL;
    int *missing_idxs = NULL;
    char *fragment_ptr = NULL;
    int is_destination_missing = 0;
    int k = -1;
    int m = -1;
    int i;
    uint64_t realloc_bm = 0;
    char **data_segments = NULL;
    char **parity_segments = NULL;
    int set_chksum = 1;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out;
    }

    if (NULL == available_fragments) {
        log_error("Can not reconstruct fragment, available fragments pointer is NULL");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    if (NULL == out_fragment) {
        log_error("Can not reconstruct fragment, output fragment pointer is NULL");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    k = instance->args.uargs.k;
    m = instance->args.uargs.m;

    for (i = 0; i < num_fragments; i++) {
        /* Verify metadata checksum */
        if (is_invalid_fragment_header(
                (fragment_header_t *) available_fragments[i])) {
            log_error("Invalid fragment header information!");
            ret = -EBADHEADER;
            goto out;
        }
    }

    /*
     * Allocate arrays for data, parity and missing_idxs
     */
    data = alloc_zeroed_buffer(sizeof(char*) * k);
    if (NULL == data) {
        log_error("Could not allocate data buffer!");
        goto out;
    }

    parity = alloc_zeroed_buffer(sizeof(char*) * m);
    if (NULL == parity) {
        log_error("Could not allocate parity buffer!");
        goto out;
    }

    missing_idxs = alloc_and_set_buffer(sizeof(int*) * (k + m), -1);
    if (NULL == missing_idxs) {
        log_error("Could not allocate missing_idxs buffer!");
        goto out;
    }

    /*
     * Separate the fragments into data and parity.  Also determine which
     * pieces are missing.
     */
    ret = get_fragment_partition(k, m, available_fragments, num_fragments,
                                 data, parity, missing_idxs);

    if (ret < 0) {
        log_error("Could not properly partition the fragments!");
        goto out;
    }

    /*
     * Odd corner-case: If the caller passes in a destination_idx that
     * is also included in the available fragments list, we should *not*
     * try to reconstruct.
     *
     * For now, we will log a warning and do nothing.  In the future, we
     * should probably log and return an error.
     *
     */
    i = 0;
    while (missing_idxs[i] > -1) {
        if (missing_idxs[i] == destination_idx) {
            is_destination_missing = 1;
        }
        i++;
    }

    if (!is_destination_missing) {
        if (destination_idx < k) {
            fragment_ptr = data[destination_idx];
        } else {
            fragment_ptr = parity[destination_idx - k];
        }
        log_warn("Dest idx for reconstruction was supplied as available buffer!");
        goto destination_available;
    }

    /*
     * Preparing the fragments for reconstruction.  This will alloc aligned
     * buffers when unaligned buffers were passed in available_fragments.
     * It passes back a bitmap telling us which buffers need to be freed by
     * us (realloc_bm).
     */
    ret = prepare_fragments_for_decode(k, m, data, parity, missing_idxs,
                                       &orig_data_size, &blocksize,
                                       fragment_len, &realloc_bm);
    if (ret < 0) {
        log_error("Could not prepare fragments for reconstruction!");
        goto out;
    }
    data_segments = alloc_zeroed_buffer(k * sizeof(char *));
    parity_segments = alloc_zeroed_buffer(m * sizeof(char *));
    get_data_ptr_array_from_fragments(data_segments, data, k);
    get_data_ptr_array_from_fragments(parity_segments, parity, m);


    /* call the backend reconstruct function passing it desc instance */
    ret = instance->common.ops->reconstruct(instance->desc.backend_desc,
                                            data_segments, parity_segments,
                                            missing_idxs, destination_idx,
                                            blocksize);
    if (ret < 0) {
        log_error("Could not reconstruct fragment!");
        goto out;
    }

    /*
     * Update the header to reflect the newly constructed fragment
     */
    if (destination_idx < k) {
        fragment_ptr = data[destination_idx];
    } else {
        fragment_ptr = parity[destination_idx - k];
    }
    init_fragment_header(fragment_ptr);
    add_fragment_metadata(instance, fragment_ptr, destination_idx,
                          orig_data_size, blocksize, instance->args.uargs.ct,
                          set_chksum);

destination_available:
    /*
     * Copy the reconstructed fragment to the output buffer
     *
     * Note: the address stored in fragment_ptr will be freed below
     */
    memcpy(out_fragment, fragment_ptr, fragment_len);

out:
    /* Free the buffers allocated in prepare_fragments_for_decode */
    if (realloc_bm != 0) {
        for (i = 0; i < k; i++) {
            if (realloc_bm & (1 << i)) {
                free(data[i]);
            }
        }

        for (i = 0; i < m; i++) {
            if (realloc_bm & (1 << (i + k))) {
                free(parity[i]);
            }
        }
    }

    free(data);
    free(parity);
    free(missing_idxs);
    free(data_segments);
    free(parity_segments);

    return ret;
}

/**
 * Return a list of lists with valid rebuild indexes given
 * a list of missing indexes.
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @fragments_to_reconstruct list of indexes to reconstruct
 * @fragments_to_exclude list of indexes to exclude from
          reconstruction equation
 * @fragments_needed list of fragments needed to reconstruct
          fragments in fragments_to_reconstruct
 *
 * @return a list of lists (bitmaps) of indexes to rebuild data
 *        from (in 'fragments_needed')
 */
int liberasurecode_fragments_needed(int desc,
                                    int *fragments_to_reconstruct,
                                    int *fragments_to_exclude,
                                    int *fragments_needed)
{
    int ret = 0;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out_error;
    }
    if (NULL == fragments_to_reconstruct) {
        log_error("Unable to determine list of fragments needed, pointer to list of indexes to reconstruct is NULL.");
        ret = -EINVALIDPARAMS;
        goto out_error;
    }

    if (NULL == fragments_to_exclude) {
        log_error("Unable to determine list of fragments needed, pointer to list of fragments to exclude is NULL.");
        ret = -EINVALIDPARAMS;
        goto out_error;
    }

    if (NULL == fragments_needed) {
        log_error("Unable to determine list of fragments needed, pointer to list of fragments to reconstruct is NULL.");
        ret = -EINVALIDPARAMS;
        goto out_error;
    }

    /* FIXME preprocessing */

    /* call the backend fragments_needed function passing it desc instance */
    ret = instance->common.ops->fragments_needed(
            instance->desc.backend_desc,
            fragments_to_reconstruct, fragments_to_exclude, fragments_needed);

out_error:
    return ret;
}

/* =~=*=~==~=*=~==~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~= */

/**
 * Get opaque metadata for a fragment.  The metadata is opaque to the
 * client, but meaningful to the underlying library.  It is used to verify
 * stripes in verify_stripe_metadata().
 *
 * @param fragment - fragment pointer
 *
 * @param fragment_metadata - pointer to output fragment metadata struct
 *          (reference passed by the user)
 */
int liberasurecode_get_fragment_metadata(char *fragment,
        fragment_metadata_t *fragment_metadata)
{
    int ret = 0;
    fragment_header_t *fragment_hdr = NULL;

    if (NULL == fragment) {
        log_error("Need valid fragment object to get metadata for");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    if (NULL == fragment_metadata) {
        log_error("Need valid fragment_metadata object for return value");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    /* Verify metadata checksum */
    if (is_invalid_fragment_header(
            (fragment_header_t *) fragment)) {
        log_error("Invalid fragment header information!");
        ret = -EBADHEADER;
        goto out;
    }

    memcpy(fragment_metadata, fragment, sizeof(struct fragment_metadata));
    fragment_hdr = (fragment_header_t *) fragment;
    if (LIBERASURECODE_FRAG_HEADER_MAGIC != fragment_hdr->magic) {
        log_error("Invalid fragment, illegal magic value");
        ret = -EINVALIDPARAMS;
        goto out;
    }

    switch(fragment_hdr->meta.chksum_type) {
        case CHKSUM_CRC32: {
            uint32_t computed_chksum = 0;
            uint32_t stored_chksum = fragment_hdr->meta.chksum[0];
            char *fragment_data = get_data_ptr_from_fragment(fragment);
            uint64_t fragment_size = fragment_hdr->meta.size;
            computed_chksum = crc32(0, fragment_data, fragment_size);
            if (stored_chksum != computed_chksum) {
                fragment_metadata->chksum_mismatch = 1;
            } else {
                fragment_metadata->chksum_mismatch = 0;
            }
            break;
        }
        case CHKSUM_MD5:
            break;
        case CHKSUM_NONE:
        default:
            break;
    }

out:
    return ret;
}

int is_invalid_fragment_header(fragment_header_t *header)
{
    uint32_t *stored_csum = NULL, csum = 0;
    assert (NULL != header);
    if (header->libec_version < _VERSION(1,2,0))
        /* no metadata checksum support */
        return 0;
    stored_csum = get_metadata_chksum((char *) header);
    if (NULL == stored_csum)
        return 1; /* can't verify, possibly crc32 call error */
    csum = crc32(0, &header->meta, sizeof(fragment_metadata_t));
    return (*stored_csum != csum);
}

int liberasurecode_verify_fragment_metadata(ec_backend_t be,
                                            fragment_metadata_t *md)
{
    int k = be->args.uargs.k;
    int m = be->args.uargs.m;
    if (md->idx < 0 || (md->idx > (k + m))) {
        return 1;
    }
    if (md->backend_id != be->common.id) {
        return 1;
    }
    if (!be->common.ops->is_compatible_with(md->backend_version))  {
        return 1;
    }
    return 0;
}

int is_invalid_fragment_metadata(int desc, fragment_metadata_t *fragment_metadata)
{
    ec_backend_t be = liberasurecode_backend_instance_get_by_desc(desc);
    if (!be) {
        log_error("Unable to verify fragment metadata: invalid backend id %d.",
                desc);
        return -EINVALIDPARAMS;
    }
    if (liberasurecode_verify_fragment_metadata(be,
            fragment_metadata) != 0) {
        return -EBADHEADER;
    }
    if (!be->common.ops->is_compatible_with(fragment_metadata->backend_version))  {
        return -EBADHEADER;
    }
    if (fragment_metadata->chksum_mismatch == 1) {
        return -EBADCHKSUM;
    }
    return 0;
}

int is_invalid_fragment(int desc, char *fragment)
{
    uint32_t ver = 0;
    fragment_metadata_t fragment_metadata;
    ec_backend_t be = liberasurecode_backend_instance_get_by_desc(desc);
    if (!be) {
        log_error("Unable to verify fragment metadata: invalid backend id %d.",
                desc);
        return 1;
    }
    if (!fragment) {
        log_error("Unable to verify fragment validity: fragments missing.");
        return 1;
    }
    if (get_libec_version(fragment, &ver) != 0 ||
            ver > LIBERASURECODE_VERSION) {
        return 1;
    }
    if (liberasurecode_get_fragment_metadata(fragment, &fragment_metadata) != 0) {
        return 1;
    }
    if (is_invalid_fragment_metadata(desc, &fragment_metadata) != 0) {
        return 1;
    }
    return 0;
}

int liberasurecode_verify_stripe_metadata(int desc,
        char **fragments, int num_fragments)
{
    int i = 0;
    if (!fragments) {
        log_error("Unable to verify stripe metadata: fragments missing.");
        return -EINVALIDPARAMS;
    }
    if (num_fragments <= 0) {
        log_error("Unable to verify stripe metadata: "
                "number of fragments must be greater than 0.");
        return -EINVALIDPARAMS;
    }

    for (i = 0; i < num_fragments; i++) {
        fragment_metadata_t *fragment_metadata = (fragment_metadata_t*)fragments[i];
        int ret = is_invalid_fragment_metadata(desc, fragment_metadata);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

/* =~=*=~==~=*=~==~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~= */

/**
 * This computes the aligned size of a buffer passed into
 * the encode function.  The encode function must pad fragments
 * to be algined with the word size (w) and the last fragment also
 * needs to be aligned.  This computes the sum of the algined fragment
 * sizes for a given buffer to encode.
 */
int liberasurecode_get_aligned_data_size(int desc, uint64_t data_len)
{
    int k;
    int ret = 0;
    int word_size;
    int alignment_multiple;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out;
    }

    k = instance->args.uargs.k;

    word_size = instance->common.ops->element_size(
            instance->desc.backend_desc) / 8;

    alignment_multiple = k * word_size;

    ret = (int) ceill( (double)
            data_len / alignment_multiple) * alignment_multiple;

out:
    return ret;
}

/**
 * This will return the minumum encode size, which is the minimum
 * buffer size that can be encoded.
 */
int liberasurecode_get_minimum_encode_size(int desc)
{
    return liberasurecode_get_aligned_data_size(desc, 1);
}

int liberasurecode_get_fragment_size(int desc, int data_len)
{
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    // TODO: Create a common function to calculate fragment size also for preprocessing
    int aligned_data_len = get_aligned_data_size(instance, data_len);
    int size = (aligned_data_len / instance->args.uargs.k) + instance->common.backend_metadata_size;

    return size;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=* misc *=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#if 0
/* Validate backend before calling init */
int liberasurecode_backend_validate(ec_backend_t backend)
{
    /* Verify that the backend implements all required methods */
}

/* FIXME - do we need to use reference counts if we are creating
* a new instance per user */

/* Get a reference to an EC backend */
ec_backend_t liberasurecode_backend_get(const char *name)
{
    ec_backend_t b = liberasurecode_backend_lookup_by_name(name);
    if (NULL != b)
        ++b->users;
    return b;
}

/* Drop an EC backend reference held */
void liberasurecode_backend_put(ec_backend_t backend)
{
    if (backend->users > 0)
        --backend->users;
}

/* Query interface for active instances */
ec_backend_t liberasurecode_backend_instance_active(ec_backend_t instance)
{
    ec_backend_t b;

    SLIST_FOREACH(b, &active_instances, link) {
        if (strcmp(b->name, name) == 0)
            return b;
    }

    return NULL;
}

ec_backend_t liberasurecode_backend_lookup_by_soname(const char *soname)
{
    ec_backend_t b;

    SLIST_FOREACH(b, &active_instances, link) {
        if (strcmp(b->soname, soname) == 0)
            return b;
    }

    return NULL;
}
#endif

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */
