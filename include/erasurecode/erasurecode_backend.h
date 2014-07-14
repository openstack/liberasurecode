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

#ifndef _ERASURECODE_INTERNAL_H_
#define _ERASURECODE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "erasurecode.h"

/* EC backend private data */
struct ec_backend_args {
    uint32_t    k;                                  /* Number of data fragments */
    uint32_t    m;                                  /* Number of parity fragments */
    uint32_t    w;                                  /* Word-size in bits */
    void*       args;
};

#define INIT            init
#define EXIT            exit
#define ENCODE          encode
#define DECODE          decode
#define FRAGSNEEDED     fragments_needed
#define RECONSTRUCT     reconstruct
#define FN_NAME(s)      str(s)
#define str(s)          #s

/* EC backend stubs - implemented per backend */
struct ec_backend_op_stubs {
    /** Backend register/init, unregister/cleanup routines **/

    /* Private backend init routine */
    void * (*INIT)(struct ec_backend_args args);

    /* Private backend cleanup routine */
    int (*EXIT)(void *);

    /* Do not define these as int (*f)(void) */
    int (*ENCODE)(void *desc);
    int (*DECODE)(void *desc);
    int (*FRAGSNEEDED)(void *desc);
    int (*RECONSTRUCT)(void *desc);
};

/**
 * EC backend method names - actual function names from the library
 * 1:1 mapping from op_stubs above to function names in the .so
 * */
struct ec_backend_fnmap {
    const char *stub_name;  /* stub name in ec_backend_op_stubs */
    const char *fn_name;    /* corresponding library function name */
};

#define MAX_LEN     64
/* EC backend common attributes */
struct ec_backend_common {
    ec_backend_id_t             id;                 /* EC backend type */
    char                        name[MAX_LEN];      /* EC backend common name */
    char                        soname[PATH_MAX];   /* EC backend shared library path */
    char                        soversion[MAX_LEN]; /* EC backend shared library version */

    struct ec_backend_op_stubs  *ops;               /* EC backend stubs */
    struct ec_backend_fnmap     *fnmap;             /* EC backend ops/stubs to library fn name map */

    uint8_t                     users;              /* EC backend number of active references */
};

/* EC backend definition */
typedef struct ec_backend {
    struct ec_backend_common    common;             /* EC backend common attributes */
    struct ec_backend_args      args;               /* EC backend instance data (private) */
    void *                      backend_desc;       /* EC backend instance handle */
    void *                      backend_sohandle;   /* EC backend shared library handle */

    int                         instance_desc;      /* liberasurecode instance handle */

    SLIST_ENTRY(ec_backend)     link;
} *ec_backend_t;

/* Init/exit routines */
int liberasurecode_backend_init(ec_backend_t backend);
int liberasurecode_backend_exit(ec_backend_t backend);

/* Backend registration interface */
int liberasurecode_backend_register(ec_backend_t backend);
int liberasurecode_backend_unregister(ec_backend_t backend);

/* Backend query interface */
ec_backend_t liberasurecode_backend_get_by_name(const char *name);
ec_backend_t liberasurecode_backend_get_by_soname(const char *soname);
void liberasurecode_backend_put(ec_backend_t backend);

/* Validate backend before calling init */
int validate_backend(ec_backend_t backend);

#endif  // _ERASURECODE_INTERNAL_H_

