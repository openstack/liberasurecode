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

#include "alg_sig.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Max is 8 bytes right now (64-bits)
#define MAX_SIG_LEN 8

void fill_random_buffer(char *buf, int size)
{
  int i;
  
  for (i=0; i < size; i++) {
    buf[i] = (char)(rand() % 256);
  }
}

void compute_parity(char **data, char* parity, int num_data, int size)
{
  int i, j;

  bzero(parity, size);

  for (i=0; i < num_data; i++) {
    for (j=0; j < size; j++) {
      parity[j] ^= data[i][j];
    }
  }
}

int check_parity_of_sigs(char **sigs, int num_data, int size)
{
  int i, j;
  int ret = 0;
  char *parity_sig = (char*)malloc(MAX_SIG_LEN);

  bzero(parity_sig, MAX_SIG_LEN);

  for (i=0; i < num_data; i++) {
    for (j=0; j < MAX_SIG_LEN; j++) {
      parity_sig[j] ^= sigs[i][j];
    }
  } 

  if (memcmp(parity_sig, sigs[num_data], MAX_SIG_LEN) != 0) {
    fprintf(stderr, "Signatures do not match:\n");
    for (i=0; i < MAX_SIG_LEN; i++) {
      fprintf(stderr, "parity_sig[%d] = 0x%x, sigs[%d][%d] = 0x%x\n", i, parity_sig[i], num_data, i, sigs[num_data][i]);
    }
    ret = 1;
  }

  free(parity_sig);

  return ret;
}

static int basic_xor_test_8_32()
{
  int blocksize = 65536;
  int num_data = 12;
  char **data;
  char *parity;
  char **sigs;
  int i;
  int ret = 0;

  alg_sig_t* sig_handle = init_alg_sig(32, 8);
  if (NULL == sig_handle) {
      goto out;
  }
  data = (char**)malloc(sizeof(char*) * num_data);
  sigs = (char**)malloc(sizeof(char*) * (num_data + 1));
  for (i=0; i < num_data; i++) {
    data[i] = (char*)malloc(sizeof(char)*blocksize);
    fill_random_buffer(data[i], blocksize);
    sigs[i] = (char*)malloc(MAX_SIG_LEN);

  }
  parity = (char*)malloc(sizeof(char)*blocksize);
  sigs[i] = (char*)malloc(MAX_SIG_LEN);

  compute_parity(data, parity, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    bzero(sigs[i], MAX_SIG_LEN);
    compute_alg_sig(sig_handle, data[i], blocksize, sigs[i]);
  }
  bzero(sigs[i], MAX_SIG_LEN);
  compute_alg_sig(sig_handle, parity, blocksize, sigs[i]);

  ret = check_parity_of_sigs(sigs, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    free(data[i]);
    free(sigs[i]);
  }

  free(parity);
  free(sigs[num_data]);
  free(sigs);
  free(data);
  destroy_alg_sig(sig_handle);

out:
  return ret;
}

static int basic_xor_test_16_64()
{
  int blocksize = 65536;
  int num_data = 12;
  char **data;
  char *parity;
  char **sigs;
  int i;
  int ret = 0;

  alg_sig_t* sig_handle = init_alg_sig(64, 16);
  if (NULL == sig_handle) {
      goto out;
  }

  data = (char**)malloc(sizeof(char*) * num_data);
  sigs = (char**)malloc(sizeof(char*) * (num_data + 1));
  for (i=0; i < num_data; i++) {
    data[i] = (char*)malloc(sizeof(char)*blocksize);
    fill_random_buffer(data[i], blocksize);
    sigs[i] = (char*)malloc(MAX_SIG_LEN);
  }
  parity = (char*)malloc(sizeof(char)*blocksize);
  sigs[i] = (char*)malloc(MAX_SIG_LEN);

  compute_parity(data, parity, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    bzero(sigs[i], MAX_SIG_LEN);
    compute_alg_sig(sig_handle, data[i], blocksize, sigs[i]);
  }
  bzero(sigs[i], MAX_SIG_LEN);
  compute_alg_sig(sig_handle, parity, blocksize, sigs[i]);
  
  ret = check_parity_of_sigs(sigs, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    free(data[i]);
    free(sigs[i]);
  }

  free(parity);
  free(sigs[num_data]);
  free(sigs);
  free(data);
  destroy_alg_sig(sig_handle);

out:
  return ret;
}

static int basic_xor_test_16_32()
{
  int blocksize = 65536;
  int num_data = 12;
  char **data;
  char *parity;
  char **sigs;
  int i;
  int ret = 0;

  alg_sig_t* sig_handle = init_alg_sig(32, 16);
  if (NULL == sig_handle) {
      goto out;
  }
  data = (char**)malloc(sizeof(char*) * num_data);
  sigs = (char**)malloc(sizeof(char*) * (num_data + 1));
  for (i=0; i < num_data; i++) {
    data[i] = (char*)malloc(sizeof(char)*blocksize);
    fill_random_buffer(data[i], blocksize);
    sigs[i] = (char*)malloc(MAX_SIG_LEN);
    
  }
  parity = (char*)malloc(sizeof(char)*blocksize);
  sigs[i] = (char*)malloc(MAX_SIG_LEN);

  compute_parity(data, parity, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    bzero(sigs[i], MAX_SIG_LEN);
    compute_alg_sig(sig_handle, data[i], blocksize, sigs[i]);
  }
  bzero(sigs[i], MAX_SIG_LEN);
  compute_alg_sig(sig_handle, parity, blocksize, sigs[i]);
  
  ret = check_parity_of_sigs(sigs, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    free(data[i]);
    free(sigs[i]);
  }

  free(parity);
  free(sigs[num_data]);
  free(sigs);
  free(data);
  destroy_alg_sig(sig_handle);

out:
  return ret;
}

int main(int argc, char**argv)
{
  int ret;
  int num_failed = 0;
  
  srand(time(NULL));

  ret = basic_xor_test_16_32();
  if (ret) {
    fprintf(stderr, "basic_xor_test_16_32 has failed!\n"); 
    num_failed++;
  }
  ret = basic_xor_test_16_64();
  if (ret) {
    fprintf(stderr, "basic_xor_test_16_64 has failed!\n"); 
    num_failed++;
  }
  ret = basic_xor_test_8_32();
  if (ret) {
    fprintf(stderr, "basic_xor_test_8_32 has failed!\n"); 
    num_failed++;
  }

  if (num_failed == 0) {
    fprintf(stderr, "Tests pass!!!\n");
  }
  return num_failed;
}

