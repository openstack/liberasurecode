/* 
 * Copyright 2015 Kevin M Greenan
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
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <liberasurecode_rs_vand.h>

int test_make_systematic_matrix(int k, int m)
{
  int *matrix = make_systematic_matrix(k, m);
  int is_identity = is_identity_matrix(matrix, k);
  
  if (!is_identity) {
    printf("Generating systematic matrix did not work!\n");
    printf("Generator matrix: \n\n");
    print_matrix(matrix, m+k, k);
  }

  free_systematic_matrix(matrix);

  return is_identity;
}

void dump_buffer(char *buf, int size, const char* filename)
{
  int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ssize_t nbytes = write(fd, buf, size);
  if (nbytes < 0) {
    printf("dump_buffer: write error!\n");
  }
  close(fd);
}

int test_invert_systematic_matrix(int k, int m, int num_missing)
{
  int *inverse = (int*)malloc(sizeof(int)*k*k);
  int *prod = (int*)malloc(sizeof(int)*k*k);
  int *missing = (int*)malloc(sizeof(int)*(num_missing+1));
  int *matrix = make_systematic_matrix(k, m);
  int *decoding_matrix = (int*)malloc(sizeof(int)*k*k);
  int *decoding_matrix_cpy = (int*)malloc(sizeof(int)*k*k);
  int n = k+m;
  int i, res;

  srand((unsigned int)time(0));

  for (i = 0;i < num_missing+1; i++) {
    missing[i] = -1;
  }

  for (i = 0;i < num_missing; i++) {
    int idx = rand() % n;
    while (is_missing(missing, idx)) {
      idx = rand() % n;
    }
    missing[i] = idx;
  }

  create_decoding_matrix(matrix, decoding_matrix, missing, k, m); 
  create_decoding_matrix(matrix, decoding_matrix_cpy, missing, k, m); 
  
  gaussj_inversion(decoding_matrix, inverse, k);  
  
  square_matrix_multiply(decoding_matrix_cpy, inverse, prod, k);
  
  res = is_identity_matrix(prod, k);

  if (!res) {
    printf("Inverting decoding matrix did not work!\n");
    printf("Generator matrix: \n\n");
    print_matrix(matrix, n, k);
    printf("Decoding matrix: \n\n");
    print_matrix(decoding_matrix_cpy, k, k);
    printf("Inverse Decoding matrix: \n\n");
    print_matrix(inverse, k, k);
    printf("Missing: \n\n");
    print_matrix(missing, 1, num_missing);
  }

  free(inverse);
  free(prod);
  free(missing);
  free(decoding_matrix);
  free(decoding_matrix_cpy);
  free_systematic_matrix(matrix);

  return res;
}

char* gen_random_buffer(int blocksize)
{
  int i;
  char *buf = (char*)malloc(blocksize);

  for (i = 0; i < blocksize; i++) {
    buf[i] = (char)(rand() % 255); 
  }

  return buf;
}

int test_encode_decode(int k, int m, int num_missing, int blocksize)
{
  char **data = (char**)malloc(sizeof(char*)*k);
  char **parity = (char**)malloc(sizeof(char*)*m);
  char **missing_bufs = (char**)malloc(sizeof(char*)*num_missing);
  int *missing = (int*)malloc(sizeof(int)*(num_missing+1));
  int *matrix = make_systematic_matrix(k, m);
  int n = k + m;
  int i;
  int ret = 1;
  
  srand((unsigned int)time(0));
    
  for (i = 0; i < k; i++) {
    data[i] = gen_random_buffer(blocksize);
  }

  for (i = 0; i < m; i++) {
    parity[i] = (char*)malloc(blocksize);
  }

  for (i = 0;i < num_missing+1; i++) {
    missing[i] = -1;
  }
  
  // Encode
  liberasurecode_rs_vand_encode(matrix, data, parity, k, m, blocksize); 

  // Copy data and parity
  for (i = 0;i < num_missing; i++) {
    int idx = rand() % n;
    while (is_missing(missing, idx)) {
      idx = rand() % n;
    }
    missing_bufs[i] = (char*)malloc(blocksize);
    memcpy(missing_bufs[i], idx < k ? data[idx] : parity[idx - k], blocksize);
    missing[i] = idx;
  }
  
  // Zero missing bufs
  for (i = 0;i < num_missing; i++) {
    if (missing[i] < k) {
      memset(data[missing[i]], 0, blocksize);
    } else {
      memset(parity[missing[i] - k], 0, blocksize);
    }
  }
  
  // Decode and check
  liberasurecode_rs_vand_decode(matrix, data, parity, k, m, missing, blocksize, 1);

  for (i = 0; i < num_missing; i++) {
    int idx = missing[i];
    if (idx < k) { 
      if (memcmp(data[idx], missing_bufs[i], blocksize)) {
        dump_buffer(data[idx], blocksize, "decoded_buffer");
        dump_buffer(missing_bufs[i], blocksize, "orig_buffer");
        ret = 0;
      }
    } else if (memcmp(parity[idx - k], missing_bufs[i], blocksize)) {
      ret = 0;
    }
  }

  for (i = 0; i < k; i++) {
    free(data[i]);
  }
  free(data);
  for (i = 0; i < m; i++) {
    free(parity[i]);
  }
  free(parity);
  for (i = 0; i < num_missing; i++) {
    free(missing_bufs[i]);
  }
  free(missing_bufs);
  free(missing);
  free(matrix);

  return ret;
}

int test_reconstruct(int k, int m, int num_missing, int blocksize)
{
  char **data = (char**)malloc(sizeof(char*)*k);
  char **parity = (char**)malloc(sizeof(char*)*m);
  char **missing_bufs = (char**)malloc(sizeof(char*)*num_missing);
  int *missing = (int*)malloc(sizeof(int)*(num_missing+1));
  int *matrix = make_systematic_matrix(k, m);
  int destination_idx = 0;
  int n = k + m;
  int i;
  int ret = 1;
  
  srand((unsigned int)time(0));
    
  for (i = 0; i < k; i++) {
    data[i] = gen_random_buffer(blocksize);
  }

  for (i = 0; i < m; i++) {
    parity[i] = (char*)malloc(blocksize);
  }

  for (i = 0;i < num_missing+1; i++) {
    missing[i] = -1;
  }
  
  // Encode
  liberasurecode_rs_vand_encode(matrix, data, parity, k, m, blocksize); 

  // Copy data and parity
  for (i = 0; i < num_missing; i++) {
    int idx = rand() % n;
    while (is_missing(missing, idx)) {
      idx = rand() % n;
    }
    missing_bufs[i] = (char*)malloc(blocksize);
    memcpy(missing_bufs[i], idx < k ? data[idx] : parity[idx - k], blocksize);
    missing[i] = idx;
    if (i == 0) {
      destination_idx = missing[i];
    }
  }
  
  // Zero missing bufs
  for (i = 0;i < num_missing; i++) {
    if (missing[i] < k) {
      memset(data[missing[i]], 0, blocksize);
    } else {
      memset(parity[missing[i] - k], 0, blocksize);
    }
  }
  
  // Reconstruct and check destination buffer
  liberasurecode_rs_vand_reconstruct(matrix, data, parity, k, m, missing, destination_idx, blocksize);

  // The original copy of the destination buffer is in the 0th buffer (see above)
  if (destination_idx < k) { 
    if (memcmp(data[destination_idx], missing_bufs[0], blocksize)) {
      dump_buffer(data[destination_idx], blocksize, "decoded_buffer");
      dump_buffer(missing_bufs[0], blocksize, "orig_buffer");
      ret = 0;
    }
  } else if (memcmp(parity[destination_idx - k], missing_bufs[0], blocksize)) {
    ret = 0;
  }

  for (i = 0; i < k; i++) {
    free(data[i]);
  }
  free(data);
  for (i = 0; i < m; i++) {
    free(parity[i]);
  }
  free(parity);
  for (i = 0; i < num_missing; i++) {
    free(missing_bufs[i]);
  }
  free(missing_bufs);
  free(missing);
  free(matrix);

  return ret;
}

int matrix_dimensions[][2] = { {12, 6}, {12, 3}, {12, 2}, {12, 1}, {5, 3}, {5, 2}, {5, 1}, {1, 1}, {-1, -1} };

int main()
{
  int i = 0;
  int blocksize = 4096;

  while (matrix_dimensions[i][0] >= 0) {
    int k = matrix_dimensions[i][0], m = matrix_dimensions[i][1];

    init_liberasurecode_rs_vand(k, m);

    int make_systematic_res = test_make_systematic_matrix(k, m);
    if (!make_systematic_res) {
      fprintf(stderr, "Error running make systematic matrix for k=%d, m=%d\n", k, m);
      return 1;
    }

    int invert_res = test_invert_systematic_matrix(k, m, m);
    if (!invert_res) {
      fprintf(stderr, "Error running inversion test for k=%d, m=%d\n", k, m);
      return 1;
    }

    int enc_dec_res = test_encode_decode(k, m, m, blocksize);
    if (!enc_dec_res) {
      fprintf(stderr, "Error running encode/decode test for k=%d, m=%d, bs=%d\n", k, m, blocksize);
      return 1;
    }

    int reconstr_res = test_reconstruct(k, m, m, blocksize);
    if (!reconstr_res) {
      fprintf(stderr, "Error running reconstruction test for k=%d, m=%d, bs=%d\n", k, m, blocksize);
      return 1;
    }
    
    
    deinit_liberasurecode_rs_vand(k, m);
    i++;
  }

  return 0;
}
