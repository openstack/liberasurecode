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
extern struct ec_backend_common backend_flat_xor_3;

ec_backend_t ec_backends_supported[EC_BACKENDS_MAX] = {
    /* backend_null */ NULL,
    /* backend_rs_vand */ NULL,
    /* backend_rs_cauchy_orig */ NULL,
    (ec_backend_t) &backend_flat_xor_3,
    /* backend_flat_xor_4 */ NULL,
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
        if (b->instance_desc == desc)
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
 * Returns new backend descriptor
 */
int liberasurecode_backend_instance_register(ec_backend_t instance)
{
    int desc = -1;

    rwlock_wrlock(&active_instances_rwlock);
    SLIST_INSERT_HEAD(&active_instances, instance, link);
    desc = liberasurecode_backend_alloc_desc();
    if (desc <= 0)
        goto register_out;
    instance->instance_desc = desc;

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
    if (instance && NULL != instance->backend_sohandle)
        return 0;

    /* Use RTLD_LOCAL to avoid symbol collisions */
    instance->backend_sohandle = dlopen(instance->common.soname, RTLD_LAZY | RTLD_LOCAL);
    if (NULL == instance->backend_sohandle) {
        print_dlerror(__func__);
        return -EBACKENDNOTAVAIL;
    }

    dlerror();    /* Clear any existing errors */
    return 0;
}

int liberasurecode_backend_close(ec_backend_t instance)
{
    if (NULL == instance || NULL == instance->backend_sohandle)
        return 0;

    dlclose(instance->backend_sohandle);
    dlerror();    /* Clear any existing errors */

    instance->backend_sohandle = NULL;
    return 0;
}

/* =~=*=~==~=*=~= liberasurecode frontend API implementation =~=*=~==~=*=~== */

/**
 * Initialize and register a liberasurecode backend
 *
 * Returns instance descriptor (int > 0)
 */
int liberasurecode_instance_create(const char *backend_name,
        int k, int m, int w, void *args)
{
    int err = 0;
    ec_backend_t instance = NULL;
    struct ec_backend_args bargs = {
        .k = k, .m = m, .w = w,
        .args = args,
    };

    ec_backend_id_t id = liberasurecode_backend_lookup_id(backend_name);
    if (-1 == id)
        return -EBACKENDNOTSUPP;

    /* Allocate memory for ec_backend instance */
    instance = calloc(1, sizeof(*instance));
    if (NULL == instance)
        return -ENOMEM;

    /* Copy common backend, args struct */
    instance->common = ec_backends_supported[id]->common;
    instance->args = bargs;

    /* Open backend .so if not already open */
    /* .so handle is returned in instance->backend_sohandle */
    err = liberasurecode_backend_open(instance);
    if (err < 0) {
        /* ignore during init, return the same handle */
        free(instance);
        return err;
    }

    /* Call private init() for the backend */
    instance->backend_desc = instance->common.ops->init(bargs);
    printf ("instance->backend_desc = %p\n", instance->backend_desc);

    /* Register instance and return a descriptor/instance id */
    instance->instance_desc = liberasurecode_backend_instance_register(instance);

    return instance->instance_desc;
}

/* Deinitialize and close a backend */
int liberasurecode_instance_destroy(int desc)
{
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);

    if (instance == NULL)
        return 0;

    /* Call private exit() for the backend */
    instance->common.ops->exit(instance->backend_desc);

    /* dlclose() backend library */
    liberasurecode_backend_close(instance);

    /* Remove instace from registry */
    liberasurecode_backend_instance_unregister(instance);

    /* Cleanup */
    free(instance);
}

/**
 * Lookup backend library function name given stub name
 *
 * @map - function stub name to library function name map
 * @stub - backend stub name
 *
 * @returns pointer to a string representing backend library function name
 */
const char *lookup_fn_name(struct ec_backend_fnmap *map, const char *stub)
{
    int i = 0;
    const char* fn_name = NULL;

    for (i = 0; i < MAX_FNS; i++) {
        if (!strcmp(map[i].stub_name, stub)) {
            fn_name = strdup(map[i].fn_name);
            break;
        }
    }

    return fn_name;
}

/**
 * Erasure encode a data buffer
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @data: original data split into an array of k equal-sized data buffers
 * @parity: array of m parity buffers
 * @blocksize: length of each of the k data elements
 *
 * @returns: a list of buffers (first k entries are data and
 *           the last m are parity)
 */
int liberasurecode_encode(int desc, char **data, char **parity, int blocksize)
{
    int (*fptr)() = NULL;
    const char *fn_name = NULL;
    const char *stub = FN_NAME(ENCODE);
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL)
        return -ENOENT;

    fn_name = lookup_fn_name(instance->common.fnmap, stub);

    /* find the address of the ENCODE function */
    *(void **)(&fptr) = dlsym(instance->backend_sohandle, fn_name);

    /* call the backend encode function passing it fptr */
    instance->common.ops->encode(instance->backend_desc, fptr,
                                 data, parity, blocksize);

    return 0;
}

/**
 * Reconstruct all of the missing fragments from a set of fragments
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @data: array of k encoded data buffers
 * @parity: array of m parity buffers
 * @missing_idx_list: list of indexes of missing elements
 * @blocksize: length of each of the k data elements
 * @fragment_size: size in bytes of the fragments
 *
 * @return list of fragments
 */
int liberasurecode_decode(int desc, char **data, char **parity,
                          int *missing_idxs, int blocksize)
{
    int (*fptr)() = NULL;
    const char *fn_name = NULL;
    const char *stub = FN_NAME(DECODE);
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL)
        return -ENOENT;

    fn_name = lookup_fn_name(instance->common.fnmap, stub);

    /* find the address of the DECODE function */
    *(void **)(&fptr) = dlsym(instance->backend_sohandle, fn_name);

    /* call the backend encode function passing it fptr */
    instance->common.ops->decode(instance->backend_desc, fptr,
                                 data, parity, missing_idxs, blocksize);

    return 0;
}

/**
 * Reconstruct a missing fragment from the the remaining fragments.
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @data: array of k encoded data buffers
 * @parity: array of m parity buffers
 * @missing_idx_list: list of indexes of missing elements
 * @destination_idx: index of fragment to reconstruct
 * @blocksize: length of each of the k data elements
 * @fragment_size: size in bytes of the fragments
 *
 * @return reconstructed destination fragment or NULL on error
 */
int liberasurecode_reconstruct(int desc, char **data, char **parity,
                               int *missing_idxs, int destination_idx,
                               int blocksize)
{
    int (*fptr)() = NULL;
    const char *fn_name = NULL;
    const char *stub = FN_NAME(RECONSTRUCT);
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL)
        return -ENOENT;

    fn_name = lookup_fn_name(instance->common.fnmap, stub);

    /* find the address of the RECONSTRUCT function */
    *(void **)(&fptr) = dlsym(instance->backend_sohandle, fn_name);

    /* call the backend encode function passing it fptr */
    instance->common.ops->reconstruct(instance->backend_desc, fptr,
                                      data, parity, missing_idxs,
                                      destination_idx, blocksize);

    return 0;
}

/**
 * Return a list of lists with valid rebuild indexes given an EC algorithm
 * and a list of missing indexes.
 *
 * @desc: liberasurecode instance descriptor (obtained with
 *        liberasurecode_instance_create)
 * @missing_idx_list: list of indexes of missing elements
 *
 * @return a list of lists of indexes to rebuild data from
 *         (in 'fragments_needed')
 */
int liberasurecode_fragments_needed(int desc, int *missing_idxs,
                                    int *fragments_needed)
{
    int (*fptr)() = NULL;
    const char *fn_name = NULL;
    const char *stub = FN_NAME(FRAGSNEEDED);
    ec_backend_t instance = liberasurecode_backend_instance_get_by_desc(desc);
    if (instance == NULL)
        return -ENOENT;

    fn_name = lookup_fn_name(instance->common.fnmap, stub);

    /* find the address of the FRAGSNEEDED function */
    *(void **)(&fptr) = dlsym(instance->backend_sohandle, fn_name);

    /* call the backend encode function passing it fptr */
    instance->common.ops->fragments_needed(instance->backend_desc, fptr,
                                           missing_idxs, fragments_needed);

    return 0;
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
