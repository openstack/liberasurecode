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

/* liberasurecode API header */

/* Supported EC backends */
typedef enum {
    EC_BACKEND_NULL             = 0,
    EC_BACKEND_RS_VAND          = 1,
    EC_BACKEND_RS_CAUCHY_ORIG   = 2,
    EC_BACKEND_FLAT_XOR_3       = 3,
    EC_BACKEND_FLAT_XOR_4       = 4,
    EC_BACKENDS_MAX,
} ec_backend_id_t;

struct ec_backend_ops {
    /* Backend register/init, unregister/cleanup routines */
    int (*init)();
    int (*exit)();

    /* Generic function pointers to be overridden with dlsym() */
    /* Do not define these as int (*f)(void) */
    int (*encode)();
    int (*decode)();
    int (*reconstruct)();
    int (*get_fragments_needed)();
    int (*get_fragment_metadata)();
    int (*verify_fragment_metadata)();
    int (*verify_stripe_metadata)();
};

#define MAX_BASENAMELEN     64
#define MAX_LIBNAMELEN      64
#define MAX_LIBVERLEN       64
typedef void * ec_backend_handle_t;

/* EC backend private data */
struct ec_backend_args {
    uint32_t k;                                         /* Number of data fragments */
    uint32_t m;                                         /* Number of parity fragments */
    uint32_t w;                                         /* Word-size in bits */
    uint32_t arg1;                                      /* Reserved1 */
    uint32_t arg2;                                      /* Reserved2 */
    uint32_t arg3;                                      /* Reserved3 */
};

/* EC backend common attributes */
struct ec_backend_common {
    ec_backend_id_t         id;                         /* EC backend id */
    char                    name[MAX_BASENAMELEN];      /* EC backend common name */
    char                    soname[MAX_LIBNAMELEN];     /* EC backend shared library path */
    char                    soversion[MAX_LIBVERLEN];   /* EC backend shared library version */
    uint8_t                 users;                      /* EC backend number of active references */

    struct ec_backend_ops * ops;                        /* EC backend ops table */
};

/* EC backend definition */
typedef struct ec_backend {
    struct ec_backend_common    common;                 /* EC backend common attributes */
    struct ec_backend_args      args;                   /* EC backend instance data (private) */
    void *                      handle;                 /* EC backend shared library handle */

    SLIST_ENTRY(ec_backend)     link;
} *ec_backend_t;

/* API functions */
int liberasurecode_backend_create_instance(ec_backend_t *pinstance,
        const char *name, int k, int m, int w, int arg1, int arg2, int arg3);
int liberasurecode_backend_destroy_instance(ec_backend_t instance);


/* Error codes */
typedef enum {
    EBACKENDNOTSUPP  = 200,
    EECMETHODNOTIMPL = 201,
    EBACKENDINUSE    = 203,
    EBACKENDNOTAVAIL = 204,
} LIBERASURECODE_ERROR_CODES;

#ifdef __cplusplus
}
#endif

#endif  // _ERASURECODE_H_
