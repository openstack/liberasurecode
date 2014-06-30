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
