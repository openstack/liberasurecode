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
#include "erasurecode_internal.h"
#include "list.h"

#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

/* EC backend references */
extern struct ec_backend_common backend_flat_xor_3;

ec_backend_t ec_backends_supported[EC_BACKENDS_MAX] = {
    /* backend_null */ NULL,
    /* backend_rs_vand */ NULL,
    /* backend_rs_cauchy_orig */ NULL,
    (ec_backend_t) &backend_flat_xor_3,
    /* backend_flat_xor_4 */ NULL,
};

/* Registered erasure code backend instances */
SLIST_HEAD(backend_list, ec_backend) active_instances =
    SLIST_HEAD_INITIALIZER(active_instances);
pthread_mutex_t active_instances_mutex = PTHREAD_MUTEX_INITIALIZER;

int liberasurecode_backend_instance_register(ec_backend_t instance)
{
    pthread_mutex_lock(&active_instances_mutex);
    SLIST_INSERT_HEAD(&active_instances, instance, link);
    pthread_mutex_unlock(&active_instances_mutex);

    return 0;
}

int liberasurecode_backend_instance_unregister(ec_backend_t instance)
{
    pthread_mutex_lock(&active_instances_mutex);
    SLIST_REMOVE(&active_instances, instance, ec_backend, link);
    pthread_mutex_unlock(&active_instances_mutex);

    return 0;
}

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

/* Check if a backend name is in the supported list */
int liberasurecode_backend_supported(const char *name)
{
    return (liberasurecode_backend_lookup_by_name(name) != NULL);
}

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
    if (instance && NULL != instance->handle)
        return 0;

    /* Use RTLD_LOCAL to avoid symbol collisions */
    instance->handle = dlopen(instance->common.soname, RTLD_LAZY | RTLD_LOCAL);
    if (NULL == instance->handle) {
        print_dlerror(__func__);
        return -EBACKENDNOTAVAIL;
    }

    dlerror();    /* Clear any existing errors */
    return 0;
}

int liberasurecode_backend_close(ec_backend_t instance)
{
    if (NULL == instance || NULL == instance->handle)
        return 0;

    dlclose(instance->handle);
    dlerror();    /* Clear any existing errors */

    instance->handle = NULL;
    return 0;
}

/* Initialize and open a backend - pointer to backend instance
 * returned in 'pinstance.' */
int liberasurecode_backend_create_instance(
        ec_backend_t *pinstance,
        const char *name, int k, int m, int w,
        int arg1, int arg2, int arg3)
{
    int err = 0;
    ec_backend_t instance = NULL;
    struct ec_backend_args args = {
        .k = k, .m = m, .w = w,
        .arg1 = arg1, .arg2 = arg2, .arg3 = arg3,
    };

    ec_backend_id_t id = liberasurecode_backend_lookup_id(name);
    if (-1 == id)
        return -EBACKENDNOTSUPP;

    /* Allocate memory for ec_backend instance */
    instance = calloc(1, sizeof(*instance));
    if (NULL == instance)
        return -ENOMEM;

    /* Copy common backend, args struct */
    instance->common = ec_backends_supported[id]->common;
    instance->args = args;

    /* Open backend .so if not already open */
    /* backend handle is returned in backend->handle */
    err = liberasurecode_backend_open(instance);
    if (err < 0) {
        /* ignore during init, return the same handle */
        free(instance);
        *pinstance = NULL;
        return err;
    }

    /* Call private init() for the backend */
    instance->common.ops->init();

    /* Register instance */
    liberasurecode_backend_instance_register(instance);

    *pinstance = instance;

    return 0;
}

/* deinitialize and close a backend */
int liberasurecode_backend_destroy_instance(ec_backend_t instance)
{
    liberasurecode_backend_close(instance);
    liberasurecode_backend_instance_unregister(instance);
    free(instance);
}

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


