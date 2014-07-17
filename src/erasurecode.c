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
#include "erasurecode_stdinc.h"
#include "erasurecode_backend.h"

/* =~=*=~==~=*=~==~=*=~= Supported EC backends =~=*=~==~=*=~==~=*=~==~=*=~== */

/* EC backend references */
extern struct ec_backend_common backend_flat_xor_hd;

ec_backend_t ec_backends_supported[EC_BACKENDS_MAX] = {
    /* backend_null */ NULL,
    /* backend_rs_vand */ NULL,
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

/* =~=*=~==~=*=~== liberasurecode backend API implementation =~=*=~==~=*=~== */

static void print_dlerror(const char *caller)
{
    char *msg = dlerror();
    if (NULL == msg)
        fprintf (stderr, "%s: unknown dynamic linking error\n", caller);
    else
        fprintf (stderr, "%s: dynamic linking error %s\n", caller, msg);
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

    if (instance == NULL)
        return 0;

    /* Call private exit() for the backend */
    instance->common.ops->exit(instance->desc.backend_desc);

    /* dlclose() backend library */
    liberasurecode_backend_close(instance);

    /* Remove instace from registry */
    liberasurecode_backend_instance_unregister(instance);

    /* Cleanup */
    free(instance);
}

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
        char **encoded_data, char **encoded_parity)
{
    int ret = 0;
    int blocksize = 0;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL) {
        ret = -EBACKENDNOTAVAIL;
        goto out_error;
    }

    /* FIXME preprocess orig_data, get blocksize */

    /* call the backend encode function passing it desc instance */
    ret = instance->common.ops->encode(instance->desc.backend_desc,
                                       encoded_data, encoded_parity, blocksize);

out_error:
    return ret;
}

/**
 * Reconstruct original data from a set of k encoded fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragment_size - size in bytes of the fragments
 * @param fragments - erasure encoded fragments (> = k)
 * @param out_data - output of decode
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc,
        uint64_t fragment_size, char **available_fragments,
        char *out_data)
{
    int ret = 0;
    int blocksize = 0;
    char **data = NULL;
    char **parity = NULL;
    int *missing_idxs;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL) {
        ret = -EBACKENDNOTAVAIL;
        goto out_error;
    }

    /* FIXME preprocess available_fragments, split into data and parity,
     * determine missing_idxs and calculate blocksize */

    /* call the backend decode function passing it desc instance */
    ret = instance->common.ops->decode(instance->desc.backend_desc,
                                       data, parity, missing_idxs, blocksize);

out_error:
    return ret;
}

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
        int destination_idx, char* out_fragment)
{
    int ret = 0;
    int blocksize = 0;
    char **data = NULL;
    char **parity = NULL;
    int *missing_idxs;

    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL) {
        ret = -EBACKENDNOTAVAIL;
        goto out_error;
    }

    /* FIXME preprocess available_fragments, split into data and parity,
     * determine missing_idxs and calculate blocksize */

    /* call the backend reconstruct function passing it desc instance */
    ret = instance->common.ops->reconstruct(instance->desc.backend_desc,
                                            data, parity, missing_idxs,
                                            destination_idx, blocksize);

out_error:
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
    if (instance == NULL) {
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
