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

#ifndef _ERASURECODE_STDINC_H_
#define _ERASURECODE_STDINC_H_

#ifndef _EXCLUDE_LIBERASURE_CODE_H_
#include "config_liberasurecode.h"
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#if defined(STDC_HEADERS)
# include <stdlib.h>
# include <stddef.h>
# include <stdarg.h>
# include <stdbool.h>
# include <unistd.h>
#else
# if defined(HAVE_STDLIB_H)
#  include <stdlib.h>
# elif defined(HAVE_MALLOC_H)
#  include <malloc.h>
# endif
# if defined(HAVE_STDDEF_H)
#  include <stddef.h>
# endif
# if defined(HAVE_STDARG_H)
#  include <stdarg.h>
# endif
# if defined(HAVE_UNISTD_H)
#  include <unistd.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#elif defined(HAVE_STDINT_H)
# include <stdint.h>
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#if defined(HAVE_ICONV) && defined(HAVE_ICONV_H)
# include <iconv.h>
#endif
#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif
#ifdef HAVE_DLFCN_H
# include <limits.h>
#endif
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#define RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#define rwlock_t pthread_rwlock_t
#define rwlock_rdlock pthread_rwlock_rdlock
#define rwlock_wrlock pthread_rwlock_wrlock
#define rwlock_tryrdlock pthread_rwlock_tryrdlock
#define rwlock_trywrlock pthread_rwlock_trywrlock
#define rwlock_unlock pthread_rwlock_unlock
#define rwlock_destroy pthread_rwlock_destroy
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_MATH_H
# include <math.h>
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
# define DECLSPEC	__attribute__ ((visibility("default")))
#else
# define DECLSPEC
#endif

// FIXME - need to move these to the main liberasurecode header
#ifdef HAVE_MALLOC
#define ERASURECODE_malloc	malloc
#else
extern DECLSPEC void *  ERASURECODE_malloc(size_t size);
#endif

#ifdef HAVE_CALLOC
#define ERASURECODE_calloc	calloc
#else
extern DECLSPEC void *  ERASURECODE_calloc(size_t nmemb, size_t size);
#endif

#ifdef HAVE_REALLOC
#define ERASURECODE_realloc	realloc
#else
extern DECLSPEC void *  ERASURECODE_realloc(void *mem, size_t size);
#endif

#ifdef HAVE_FREE
#define ERASURECODE_free	free
#else
extern DECLSPEC void    ERASURECODE_free(void *mem);
#endif

/*  Redefine main() on MacOS */

#if defined(__MACOS__) || defined(__MACOSX__)

#ifdef __cplusplus
#define C_LINKAGE	"C"
#else
#define C_LINKAGE
#endif /* __cplusplus */

/** The application's main() function must be called with C linkage,
 *  and should be declared like this:
 *      @code
 *      #ifdef __cplusplus
 *      extern "C"
 *      #endif
 *	int main(int argc, char *argv[])
 *	{
 *	}
 *      @endcode
 */
#define main	EC_main

/** The prototype for the application's main() function */
extern C_LINKAGE int EC_main(int argc, char *argv[]);

#endif  // MACOSX

#endif // _ERASURECODE_STDINC_H_
