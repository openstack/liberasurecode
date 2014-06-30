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

#include <pthread.h>

/* Registered erasure code backends */
SLIST_HEAD(, ec_backend_t) registered_backends =
    SLIST_HEAD_INITIALIZER(registered_backends);
pthread_mutex_t registered_backends_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Add erasurecode backend to the supported backends list */
int liberasurecode_backend_register(ec_backend_t *backend)
{
    pthread_mutex_lock(&registered_backends_mutex);
    SLIST_INSERT_HEAD(&registered_backends, backend, link);
    pthread_mutex_unlock(&registered_backends_mutex);
}

/* Remove erasurecode backend from the supported backends list */
int liberasurecode_backend_unregister(ec_backend_t *backend)
{
    pthread_mutex_lock(&registered_backends_mutex);
    SLIST_REMOVE(&registered_backends, backend, ec_backend, link);
    pthread_mutex_unlock(&registered_backends_mutex);
}

/* Backend query interfaces */
ec_backend_t* liberasurecode_backend_get_by_name(const char *name)
{
    ec_backend_t *b;

    SLIST_FOREACH(b, &registered_backends, link) {
        if (strcmp(b->name, name) == 0) {
            ++b->users;
            return b;
        }
    }

    return NULL;
}

ec_backend_t* liberasurecode_backend_get_by_soname(const char *soname)
{
    ec_backend_t *b;

    SLIST_FOREACH(b, &registered_backends, link) {
        if (strcmp(b->soname, soname) == 0) {
            ++b->users;
            return b;
        }
    }

    return NULL;
}

void liberasurecode_backend_put(ec_backend_t *backend)
{
    if (b->users > 0)
        --b->users;
}

int liberasurecode_backend_supported(const char *name)
{
    return (liberasurecode_backend_get_by_name(name) != NULL);
}

/* Validate backend before calling init */
int validate_backend(ec_backend_t backend)
{
    /* Verify that the backend implements all required methods */
}

/* Try to register all supported backends */
int liberasurecode_backend_init_all(ec_backend_t backend)
{
    // FIXME - implement init table for other backend init methods
    // for now, init just the xor backend
}

