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
 * liberasurecode API implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include "list.h"
#include "erasurecode.h"
#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_preprocessing.h"
#include "erasurecode_stdinc.h"

/* =*=~==~=*=~==~=*=~= liberasurecode init/exit routines =~=*=~==~=*=~==~=*= */

void __attribute__ ((constructor))
liberasurecode_init(void) {
    openlog("liberasurecode", LOG_PID | LOG_CONS, LOG_USER);
}

void __attribute__ ((destructor))
liberasurecode_exit(void) {
    closelog();
}

/* =~=*=~==~=*=~==~=*=~= Supported EC backends =~=*=~==~=*=~==~=*=~==~=*=~== */

/* EC backend references */
extern struct ec_backend_common backend_null;
extern struct ec_backend_common backend_flat_xor_hd;
extern struct ec_backend_common backend_jerasure_rs_vand;

ec_backend_t ec_backends_supported[EC_BACKENDS_MAX] = {
    (ec_backend_t) &backend_null,
    (ec_backend_t) &backend_jerasure_rs_vand,
    /* backend_rs_cauchy_orig */ NULL,
    (ec_backend_t) &backend_flat_xor_hd,
};

/* Get EC backend by name */
ec_backend_t liberasurecode_backend_lookup_by_name(const char *name)
{
    int b = 0;

    for (b = 0; b < EC_BACKENDS_MAX; ++b) {
        if (!strcmp(ec_backends_supported[b]->common.name, name))
            return ec_backends_supported[b];
    }

    return NULL;
}

/* Name to ID mapping for EC backend */
ec_backend_id_t liberasurecode_backend_lookup_id(const char *name)
{
    int b = 0;

    for (b = 0; b < EC_BACKENDS_MAX; ++b) {
        ec_backend_t backend = ec_backends_supported[b];
        if (backend && !strcmp(backend->common.name, name))
            return backend->common.id;
    }

    return -1;
}

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
 * Returns pointer to a registered liberasurecode instance
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
    int desc = -1;

    rwlock_wrlock(&active_instances_rwlock);
    SLIST_INSERT_HEAD(&active_instances, instance, link);
    desc = liberasurecode_backend_alloc_desc();
    if (desc <= 0)
        goto register_out;
    instance->idesc = desc;

register_out:
    rwlock_unlock(&active_instances_rwlock);
    return desc;
}

/**
 * Unregister a backend instance
 */
int liberasurecode_backend_instance_unregister(ec_backend_t instance)
{
    rwlock_wrlock(&active_instances_rwlock);
    SLIST_REMOVE(&active_instances, instance, ec_backend, link);
    rwlock_unlock(&active_instances_rwlock);

    return 0;
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
int liberasurecode_backend_open(ec_backend_t instance)
{
	void *handle = NULL;
    if (instance && NULL != instance->desc.backend_sohandle)
        return 0;

    /* Use RTLD_LOCAL to avoid symbol collisions */
    instance->desc.backend_sohandle = dlopen(instance->common.soname, RTLD_LAZY | RTLD_LOCAL);
    if (NULL == instance->desc.backend_sohandle) {
        print_dlerror(__func__);
        return -EBACKENDNOTAVAIL;
    }

    dlerror();    /* Clear any existing errors */
    return 0;
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

/* =~=*=~==~=*=~= liberasurecode frontend API implementation =~=*=~==~=*=~== */

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
 *          null_args - arguments for the null backend
 *          flat_xor_hd_args - arguments for the xor_hd backend
 *          jerasure_args - arguments for the Jerasure backend
 *      
 * @returns liberasurecode instance descriptor (int > 0)
 */
int liberasurecode_instance_create(const char *backend_name,
                                   struct ec_args *args)
{
    int err = 0;
    ec_backend_t instance = NULL;
    struct ec_backend_args bargs;

    ec_backend_id_t id = liberasurecode_backend_lookup_id(backend_name);
    if (-1 == id)
        return -EBACKENDNOTSUPP;

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
    err = liberasurecode_backend_open(instance);
    if (err < 0) {
        /* ignore during init, return the same handle */
        free(instance);
        return err;
    }

    /* Call private init() for the backend */
    instance->desc.backend_desc = instance->common.ops->init(&instance->args,
            instance->desc.backend_sohandle);
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
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);

    if (NULL == instance)
        return 0;

    /* Call private exit() for the backend */
    instance->common.ops->exit(instance->desc.backend_desc);

    /* dlclose() backend library */
    liberasurecode_backend_close(instance);

    /* Remove instace from registry */
    liberasurecode_backend_instance_unregister(instance);

    /* Cleanup */
    free(instance);

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
    int i;
    int k, m;
    int ret = 0;            /* return code */

    int blocksize = 0;      /* length of each of k data elements */
    int data_len;           /* data len to write to fragment headers */
    int aligned_data_len;   /* EC algorithm compatible data length */

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
        goto out;
    }

    /* call the backend encode function passing it desc instance */
    ret = instance->common.ops->encode(instance->desc.backend_desc,
                                       *encoded_data, *encoded_parity, blocksize);
    if (ret < 0) {
        goto out;
    }

    ret = finalize_fragments_after_encode(instance, k, m, blocksize,
                                          *encoded_data, *encoded_parity);

    *fragment_len = get_fragment_size((*encoded_data)[0]);
out:
    /* FIXME add cleanup API to call when encode() has an error */
    if (ret)
        log_error("Error in liberasurecode_encode %d", ret);
    return ret;
}

/**
 * Reconstruct original data from a set of k encoded fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragments - erasure encoded fragments (> = k)
 * @param num_fragments - number of fragments being passed in
 * @param fragment_len - length of each fragment (assume they are the same)
 * @param out_data - _output_ pointer to decoded data
 * @param out_data_len - _output_ length of decoded output
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        char **out_data, uint64_t *out_data_len)         /* output */
{
    int i, j;
    int ret = 0;

    int k, m;
    int orig_data_size = 0;

    int blocksize = 0;
    char **data = NULL;
    char **parity = NULL;
    int *missing_idxs = NULL;

    uint64_t realloc_bm = 0;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out;
    }

    k = instance->args.uargs.k;
    m = instance->args.uargs.m;

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
    
    missing_idxs = alloc_zeroed_buffer(sizeof(char*) * k);
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
     * Preparing the fragments for decode.  This will alloc aligned buffers
     * when unaligned buffers were passed in available_fragments.  It passes
     * back a bitmap telling us which buffers need to be freed by us
     * (realloc_bm).
     *
     * This also returns data/parity as fragment payloads (the header is not
     * included).  The pointers need to be asjusted after decode to include
     * the headers.
     */
    ret = prepare_fragments_for_decode(k, m,
                                       data, parity, missing_idxs, 
                                       &orig_data_size, &blocksize,
                                       fragment_len, &realloc_bm);
    if (ret < 0) {
        log_error("Could not prepare fragments for decode!");
        goto out;
    }

    /* call the backend decode function passing it desc instance */
    ret = instance->common.ops->decode(instance->desc.backend_desc,
                                       data, parity, missing_idxs, blocksize);
    
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
        int missing_idx = missing_idxs[j]; 
        if (missing_idx < k) {
            /* Generate headers */
            char *fragment_ptr = 
                get_fragment_ptr_from_data_novalidate(data[missing_idx]);

            init_fragment_header(fragment_ptr);
            set_fragment_idx(fragment_ptr, missing_idx);
            set_orig_data_size(fragment_ptr, orig_data_size);
            set_fragment_payload_size(fragment_ptr, blocksize);

            /* Swap it! */
            data[missing_idx] = fragment_ptr;
        }
        j++;
    }
    
    /* Try to generate the original string */
    ret = fragments_to_string(k, m, data, k, out_data, out_data_len);

    if (ret < 0) {
        log_error("Could not prepare convert decoded fragments to a string!");
    }

out:
    /* Free the buffers allocated in prepare_fragments_for_decode */
    if (realloc_bm != 0) {
        for (i = 0; i < k; i++) {
            if (realloc_bm & (1 << i)) {
                free(get_fragment_ptr_from_data_novalidate(data[i]));
            }
        }

        for (i = 0; i < m; i++) {
            if (realloc_bm & (1 << (i + k))) {
                free(get_fragment_ptr_from_data_novalidate(parity[i]));
            }
        }
    }

    if (NULL != data) {
        free(data);
    }
    if (NULL != parity) {
        free(parity);
    }
    if (NULL != missing_idxs) {
        free(missing_idxs);
    }

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
    int k;
    int m;
    int i;
    int j;
    uint64_t realloc_bm = 0;

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
    
    missing_idxs = alloc_zeroed_buffer(sizeof(char*) * k);
    if (NULL == missing_idxs) {
        log_error("Could not allocate missing_idxs buffer!");
        goto out;
    }
    
    /*
     * Separate the fragments into data and parity.  Also determine which
     * pieces are missing.
     */
    ret = get_fragment_partition(k, m, available_fragments, num_fragments, data, parity, missing_idxs);

    if (ret < 0) {
        log_error("Could not properly partition the fragments!");
        goto out;
    }

    /*
     * Preparing the fragments for reconstruction.  This will alloc aligned buffers when unaligned buffers
     * were passed in available_fragments.  It passes back a bitmap telling us which buffers need to
     * be freed by us (realloc_bm).
     *
     * This also returns data/parity as fragment payloads (the header is not included).  The pointers
     * need to be asjusted after reconstruction to include the headers.
     */
    ret = prepare_fragments_for_decode(k, m, data, parity, missing_idxs, &orig_data_size, &blocksize, fragment_len, &realloc_bm);
    if (ret < 0) {
        log_error("Could not prepare fragments for reconstruction!");
        goto out;
    }

    /* call the backend reconstruct function passing it desc instance */
    ret = instance->common.ops->reconstruct(instance->desc.backend_desc,
                                            data, parity, missing_idxs,
                                            destination_idx, blocksize);

    /*
     * Update the header to reflect the newly constructed fragment
     */
    if (destination_idx < k) {
        fragment_ptr = get_fragment_ptr_from_data_novalidate(data[destination_idx]);
    } else {
        fragment_ptr = get_fragment_ptr_from_data_novalidate(parity[destination_idx - k]);
    }
    init_fragment_header(fragment_ptr);
    set_fragment_idx(fragment_ptr, destination_idx);
    set_orig_data_size(fragment_ptr, orig_data_size);
    set_fragment_payload_size(fragment_ptr, blocksize);

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
                free(get_fragment_ptr_from_data_novalidate(data[i]));
            }
        }

        for (i = 0; i < m; i++) {
            if (realloc_bm & (1 << (i + k))) {
                free(get_fragment_ptr_from_data_novalidate(parity[i]));
            }
        }
    }

    if (NULL != data) {
        free(data);
    }
    if (NULL != parity) {
        free(parity);
    }
    if (NULL != missing_idxs) {
        free(missing_idxs);
    }

    return ret;
}

/**
 * Return a list of lists with valid rebuild indexes given
 * a list of missing indexes.
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @missing_idx_list: list of indexes of missing elements
 *
 * @return a list of lists (bitmaps) of indexes to rebuild data
 *        from (in 'fragments_needed')
 */
int liberasurecode_fragments_needed(int desc, int *missing_idxs,
                                    int *fragments_needed)
{
    int ret = 0;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (NULL == instance) {
        ret = -EBACKENDNOTAVAIL;
        goto out_error;
    }

    /* FIXME preprocessing */

    /* call the backend fragments_needed function passing it desc instance */
    ret = instance->common.ops->fragments_needed(
            instance->desc.backend_desc,
            missing_idxs, fragments_needed);

out_error:
    return ret;
}

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
