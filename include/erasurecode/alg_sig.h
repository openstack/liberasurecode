/* * Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
 * All rights reserved.
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

#ifndef _ALG_SIG_H
#define _ALG_SIG_H

typedef int (*galois_single_multiply_func)(int, int, int);

struct jerasure_mult_routines {
  galois_single_multiply_func galois_single_multiply;
};

#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define JERASURE_SONAME "libJerasure.dylib"
#else
#define JERASURE_SONAME "libJerasure.so"
#endif

typedef struct alg_sig_s
{
  int gf_w;
  int sig_len;
  struct jerasure_mult_routines mult_routines;
  void *jerasure_sohandle;
  int *tbl1_l;
  int *tbl1_r;
  int *tbl2_l;
  int *tbl2_r;
  int *tbl3_l;
  int *tbl3_r;
} alg_sig_t;

alg_sig_t *init_alg_sig(int sig_len, int gf_w);
void destroy_alg_sig(alg_sig_t* alg_sig_handle);

int compute_alg_sig(alg_sig_t* alg_sig_handle, char *buf, int len, char *sig);
int crc32_build_fast_table();
int crc32(int crc, const void *buf, int size);

#endif

