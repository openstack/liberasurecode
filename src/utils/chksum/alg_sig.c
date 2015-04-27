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

#include <dlfcn.h>
#include <alg_sig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GALOIS_SINGLE_MULTIPLY "galois_single_multiply"
#define GALOIS_UNINIT "galois_uninit_field"

int valid_gf_w[] = { 8, 16, -1 };
int valid_pairs[][2] = { { 8, 32}, {16, 32}, {16, 64}, {-1, -1} };

galois_single_multiply_func get_galois_multi_func(void *handle) {
    /*
     * ISO C forbids casting a void* to a function pointer.
     * Since dlsym return returns a void*, we use this union to
     * "transform" the void* to a function pointer.
     */
    union {
        galois_single_multiply_func fptr;
        void *vptr;
    } func_handle = {.vptr = NULL};
    func_handle.vptr = dlsym(handle,  GALOIS_SINGLE_MULTIPLY);
    return func_handle.fptr;
}

galois_uninit_field_func get_galois_uninit_func(void *handle) {
    /*
     * ISO C forbids casting a void* to a function pointer.
     * Since dlsym return returns a void*, we use this union to
     * "transform" the void* to a function pointer.
     */
    union {
        galois_uninit_field_func fptr;
        void *vptr;
    } func_handle = {.vptr = NULL};
    func_handle.vptr = dlsym(handle,  GALOIS_UNINIT);
    return func_handle.fptr;
}


void *get_jerasure_sohandle()
{
    return dlopen(JERASURE_SONAME, RTLD_LAZY | RTLD_LOCAL);
}

int load_gf_functions(void *sohandle, struct jerasure_mult_routines *routines)
{
    routines->galois_single_multiply = get_galois_multi_func(sohandle);
    routines->galois_uninit_field = get_galois_uninit_func(sohandle);
    if (NULL == routines->galois_single_multiply) {
      return -1;
    }
    return 0;
}

static
alg_sig_t *init_alg_sig_w8(void *jerasure_sohandle, int sig_len)
{
    alg_sig_t *alg_sig_handle;
    int num_gf_lr_table_syms;
    int i;
    int w = 8;
    int alpha = 2, beta = 4, gamma = 8;
    int num_components = sig_len / w;

    alg_sig_handle = (alg_sig_t *)malloc(sizeof(alg_sig_t));
    if (NULL == alg_sig_handle) {
      return NULL;
    }

    alg_sig_handle->jerasure_sohandle = jerasure_sohandle;

    if (load_gf_functions(alg_sig_handle->jerasure_sohandle, &(alg_sig_handle->mult_routines)) < 0) {
      free(alg_sig_handle);
      return NULL;
    }

    alg_sig_handle->sig_len = sig_len;
    alg_sig_handle->gf_w = w;

    num_gf_lr_table_syms = 1 << (w >> 1);

    if (num_components >= 4) {
      alg_sig_handle->tbl1_l = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl1_r = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl2_l = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl2_r = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl3_l = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl3_r = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
    }
    
    /*
     * Note that \alpha = 2 
     * Note that \beta = 4 (\alpha ^ 2)
     * Note that \gamme = 8 (\alpha ^ 3)
     */
    for (i = 0; i < 16; i++) {
      if (num_components >= 4) {
        alg_sig_handle->tbl1_l[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned char)(i << 4) & 0xf0, alpha, w);
        alg_sig_handle->tbl1_r[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned char) i, alpha, w);

        alg_sig_handle->tbl2_l[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned char) (i << 4) & 0xf0, beta, w);
        alg_sig_handle->tbl2_r[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned char) i, beta, w);
      
        alg_sig_handle->tbl3_l[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned char) (i << 4) & 0xf0, gamma, w);
        alg_sig_handle->tbl3_r[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned char) i, gamma, w);
      }
    }

    return alg_sig_handle;
}

static
alg_sig_t *init_alg_sig_w16(void *jerasure_sohandle, int sig_len)
{
    alg_sig_t *alg_sig_handle;
    int num_gf_lr_table_syms;
    int i;
    int w = 16;
    int alpha = 2, beta = 4, gamma = 8;
    int num_components = sig_len / w;

    if (NULL == jerasure_sohandle) {
        return NULL;
    }
    
    alg_sig_handle = (alg_sig_t *)malloc(sizeof(alg_sig_t));
    if (NULL == alg_sig_handle) {
      return NULL;
    }

    alg_sig_handle->jerasure_sohandle = jerasure_sohandle;

    if (load_gf_functions(alg_sig_handle->jerasure_sohandle, &(alg_sig_handle->mult_routines)) < 0) {
      free(alg_sig_handle);
      return NULL;
    }

    alg_sig_handle->sig_len = sig_len;
    alg_sig_handle->gf_w = w;

    num_gf_lr_table_syms = 1 << (w >> 1);

    if (num_components >= 2) {
      alg_sig_handle->tbl1_l = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl1_r = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
    }

    if (num_components >= 4) {
      alg_sig_handle->tbl2_l = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl2_r = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl3_l = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
      alg_sig_handle->tbl3_r = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
    }
    
    /*
     * Note that \alpha = 2 
     * Note that \beta = 4 (\alpha ^ 2 MOD 2^16)
     * Note that \gamme = 8 (\alpha ^ 3 MOD 2^16)
     */
    for (i = 0; i < 256; i++) {
      alg_sig_handle->tbl1_l[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned short) (i << 8), alpha, w);
      alg_sig_handle->tbl1_r[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned short) i, alpha, w);

      if (num_components >= 4) {
        alg_sig_handle->tbl2_l[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned short) (i << 8), beta, w);
        alg_sig_handle->tbl2_r[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned short) i, beta, w);
      
        alg_sig_handle->tbl3_l[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned short) (i << 8), gamma, w);
        alg_sig_handle->tbl3_r[i] = alg_sig_handle->mult_routines.galois_single_multiply((unsigned short) i, gamma, w);
      }
    }

    return alg_sig_handle;
}

alg_sig_t *init_alg_sig(int sig_len, int gf_w)
{
  int i=0;
  void *jerasure_sohandle = get_jerasure_sohandle();

  if (NULL == jerasure_sohandle) {
    fprintf (stderr, "Could not open Jerasure backend.  Install Jerasure or fix LD_LIBRARY_PATH.  Passing.\n");
    return NULL;
  }

  while (valid_pairs[i][0] > -1) {
    if (gf_w == valid_pairs[i][0] && 
        sig_len == valid_pairs[i][1]) {
      break;
    }
    i++;
  }

  if (valid_pairs[i][0] == -1) {
    return NULL;
  }

  if (gf_w == 8) {
    return init_alg_sig_w8(jerasure_sohandle, sig_len);
  } else if (gf_w == 16) {
    return init_alg_sig_w16(jerasure_sohandle, sig_len);
  }
  return NULL;
}

void destroy_alg_sig(alg_sig_t* alg_sig_handle)
{
  if (alg_sig_handle == NULL) {
    return;
  }
  if (alg_sig_handle->gf_w == 0) {
    free(alg_sig_handle);
    return;
  }

  alg_sig_handle->mult_routines.galois_uninit_field(alg_sig_handle->gf_w);
  dlclose(alg_sig_handle->jerasure_sohandle);

  int num_components = alg_sig_handle->sig_len / alg_sig_handle->gf_w;

  free(alg_sig_handle->tbl1_l);
  free(alg_sig_handle->tbl1_r);
  if (num_components >= 4) {
    free(alg_sig_handle->tbl2_l);
    free(alg_sig_handle->tbl2_r);
    free(alg_sig_handle->tbl3_l);
    free(alg_sig_handle->tbl3_r);
  }

  free(alg_sig_handle);
}


static
int compute_w8_alg_sig_32(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  int i;

  if (len == 0) {
    bzero(sig, 4);
    return 0;
  }

  sig[0] = buf[len-1];
  sig[1] = buf[len-1];
  sig[2] = buf[len-1];
  sig[3] = buf[len-1];

  /**
   * This is the loop to optimize.  It is currently optimized enough : using Horner's alg.,
   * shortened mult. tables, and other tricks.
   */
  for (i = len - 2; i >= 0; i--) {
      sig[0] ^= buf[i];
      sig[1] = (buf[i] ^ (alg_sig_handle->tbl1_l[(sig[1] >> 4) & 0x0f] ^ alg_sig_handle->tbl1_r[sig[1] & 0x0f]));
      sig[2] = (buf[i] ^ (alg_sig_handle->tbl2_l[(sig[2] >> 4) & 0x0f] ^ alg_sig_handle->tbl2_r[sig[2] & 0x0f]));
      sig[3] = (buf[i] ^ (alg_sig_handle->tbl3_l[(sig[3] >> 4) & 0x0f] ^ alg_sig_handle->tbl3_r[sig[3] & 0x0f]));
  }

  return 0;
}

static
int compute_w16_alg_sig_64(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  int bit_mask;
  int adj_len = len / 2;
  int i;
  unsigned short *_buf = (unsigned short *)buf;
  unsigned short sig_buf[4];

  if (len == 0) {
    bzero(sig, 8);
    return 0;
  }

  switch (len % 2) {
      case 1:
          bit_mask = 0x00ff;
          break;
      default:
          bit_mask = 0xffff;
          break;
  }

  if (len % 2 > 0) {
      adj_len++;
  }

  // Account for buffer not being uint16_t aligned
  sig_buf[0] = (_buf[adj_len - 1] & bit_mask);
  sig_buf[1] = (_buf[adj_len - 1] & bit_mask);
  sig_buf[2] = (_buf[adj_len - 1] & bit_mask);
  sig_buf[3] = (_buf[adj_len - 1] & bit_mask);

  /**
   * This is the loop to optimize.  It is currently optimized enough : using Horner's alg.,
   * shortened mult. tables, and other tricks.
   */
  for (i = adj_len - 2; i >= 0; i--) {
      sig_buf[0] ^= _buf[i];
      sig_buf[1] = (_buf[i] ^ (alg_sig_handle->tbl1_l[(sig_buf[1] >> 8) & 0x00ff] ^ alg_sig_handle->tbl1_r[sig_buf[1] & 0x00ff]));
      sig_buf[2] = (_buf[i] ^ (alg_sig_handle->tbl2_l[(sig_buf[2] >> 8) & 0x00ff] ^ alg_sig_handle->tbl2_r[sig_buf[2] & 0x00ff]));
      sig_buf[3] = (_buf[i] ^ (alg_sig_handle->tbl3_l[(sig_buf[3] >> 8) & 0x00ff] ^ alg_sig_handle->tbl3_r[sig_buf[3] & 0x00ff]));
  }

  sig[0] = (char) (sig_buf[0] & 0x000ff);
  sig[1] = (char) ((sig_buf[0] >> 8) & 0x000ff);
  sig[2] = (char) (sig_buf[1] & 0x00ff);
  sig[3] = (char) ((sig_buf[1] >> 8) & 0x00ff);
  sig[4] = (char) (sig_buf[2] & 0x00ff);
  sig[5] = (char) ((sig_buf[2] >> 8) & 0x00ff);
  sig[6] = (char) (sig_buf[3] & 0x00ff);
  sig[7] = (char) ((sig_buf[3] >> 8) & 0x00ff);
  return 0;
}

static
int compute_w16_alg_sig_32(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  int bit_mask;
  int adj_len = len / 2;
  int i;
  unsigned short *_buf = (unsigned short *)buf;
  unsigned short sig_buf[2];

  if (len == 0) {
    bzero(sig, 8);
    return 0;
  }

  switch (len % 2) {
      case 1:
          bit_mask = 0x00ff;
          break;
      default:
          bit_mask = 0xffff;
          break;
  }

  if (len % 2 > 0) {
      adj_len++;
  }

  // Account for buffer not being uint16_t aligned
  sig_buf[0] = (_buf[adj_len - 1] & bit_mask);
  sig_buf[1] = (_buf[adj_len - 1] & bit_mask);

  /**
   * This is the loop to optimize.  It is currently optimized enough : using Horner's alg.,
   * shortened mult. tables, and other tricks.
   */
  for (i = adj_len - 2; i >= 0; i--) {
      sig_buf[0] ^= _buf[i];
      sig_buf[1] = (_buf[i] ^ (alg_sig_handle->tbl1_l[(sig_buf[1] >> 8) & 0x00ff] ^ alg_sig_handle->tbl1_r[sig_buf[1] & 0x00ff]));
  }

  sig[0] = (char) (sig_buf[0] & 0x000ff);
  sig[1] = (char) ((sig_buf[0] >> 8) & 0x000ff);
  sig[2] = (char) (sig_buf[1] & 0x00ff);
  sig[3] = (char) ((sig_buf[1] >> 8) & 0x00ff);
  return 0;
}

static
int compute_alg_sig_32(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  if (alg_sig_handle->gf_w == 8) {
    return compute_w8_alg_sig_32(alg_sig_handle, buf, len, sig);
  } else if (alg_sig_handle->gf_w == 16) {
    return compute_w16_alg_sig_32(alg_sig_handle, buf, len, sig);
  }
  return -1;
}

static
int compute_alg_sig_64(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  if (alg_sig_handle->gf_w == 16) {
    return compute_w16_alg_sig_64(alg_sig_handle, buf, len, sig);
  }
  return -1;
}

int compute_alg_sig(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  if (alg_sig_handle->sig_len == 32) {
    return compute_alg_sig_32(alg_sig_handle, buf, len, sig);
  } else if (alg_sig_handle->sig_len == 64) {
    return compute_alg_sig_64(alg_sig_handle, buf, len, sig);
  }
  return -1;
}
