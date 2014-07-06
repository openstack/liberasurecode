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

#include <emmintrin.h>  //SSE2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xor_code.h>

const int g_bit_lookup[] = {0x1, 0x2, 0x4, 0x8,
                                 0x10, 0x20, 0x40, 0x80,
                                 0x100, 0x200, 0x400, 0x800,
                                 0x1000, 0x2000, 0x4000, 0x8000,
                                 0x10000, 0x20000, 0x40000, 0x80000,
                                 0x100000, 0x200000, 0x400000, 0x800000,
                                 0x1000000, 0x2000000, 0x4000000, 0x8000000,
                                 0x10000000, 0x20000000, 0x40000000, 0x80000000};

int is_data_in_parity(int data_idx, unsigned int parity_bm)
{
  return ((g_bit_lookup[data_idx] & parity_bm) == g_bit_lookup[data_idx]);
}

int does_parity_have_data(int parity_idx, unsigned int data_bm)
{
  return ((g_bit_lookup[parity_idx] & data_bm) == g_bit_lookup[parity_idx]);
}

int parity_bit_lookup(xor_code_t *code_desc, int index)
{
  return g_bit_lookup[code_desc->k - index];
}

int data_bit_lookup(xor_code_t *code_desc, int index)
{
  return g_bit_lookup[index];
}

int missing_elements_bm(xor_code_t *code_desc, int *missing_elements, int (*bit_lookup_func)(xor_code_t *code_desc, int index))
{
  int i = 0;
  int bm = 0;

  while (missing_elements[i] > -1) {
    bm |= bit_lookup_func(code_desc, missing_elements[i]);
    i++;
  } 

  return bm;
}

failure_pattern_t get_failure_pattern(xor_code_t *code_desc, int *missing_idxs)
{
  int i = 0;
  int num_failures = 0;
  failure_pattern_t pattern = FAIL_PATTERN_0D_0P;

  while (missing_idxs[i] > -1) {
    if (num_failures >= code_desc->hd) {
      pattern = FAIL_PATTERN_GE_HD;
    }
    switch(pattern) {
      case FAIL_PATTERN_0D_0P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_1D_0P : FAIL_PATTERN_0D_1P;
        break;
      case FAIL_PATTERN_1D_0P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_2D_0P : FAIL_PATTERN_1D_1P;
        break;
      case FAIL_PATTERN_2D_0P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_3D_0P : FAIL_PATTERN_2D_1P;
        break;
      case FAIL_PATTERN_3D_0P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_1D_1P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_2D_1P : FAIL_PATTERN_1D_2P;
        break;
      case FAIL_PATTERN_1D_2P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_2D_1P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_0D_1P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_1D_1P : FAIL_PATTERN_0D_2P;
        break;
      case FAIL_PATTERN_0D_2P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_1D_2P : FAIL_PATTERN_0D_3P;
        break;
      case FAIL_PATTERN_0D_3P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_GE_HD:
      default:
        break;
    } 
    if (pattern == FAIL_PATTERN_GE_HD) {
      break;
    }
    i++;
  }

  return pattern; 
}

void fast_memcpy(char *dst, char *src, int size)
{
    // Use _mm_stream_si128((__m128i*) _buf2, sum);
    memcpy(dst, src, size);
}

/*
 * Buffers must be aligned to 16-byte boundaries
 *
 * Store in buf2 (opposite of memcpy convention...  Maybe change?)
 */
void xor_bufs_and_store(char *buf1, char *buf2, int blocksize)
{
#ifdef INTEL_SSE2
  int residual_bytes = num_unaligned_end(blocksize);
  int fast_blocksize = blocksize > residual_bytes ? (blocksize - residual_bytes) : 0;
  int fast_int_blocksize = fast_blocksize / sizeof(__m128i);
  int i;
  __m128i *_buf1 = (__m128i*)buf1; 
  __m128i *_buf2 = (__m128i*)buf2; 

  /*
   * XOR aligned region using 128-bit XOR
   */
  for (i=0; i < fast_int_blocksize; i++) {
    _buf2[i] = _mm_xor_si128(_buf1[i], _buf2[i]);
  }
#else
  int residual_bytes = num_unaligned_end(blocksize);
  int fast_blocksize = blocksize > residual_bytes ? (blocksize - residual_bytes) : 0;
  int fast_int_blocksize = fast_blocksize / sizeof(unsigned long);
  int i;

  unsigned long*_buf1 = (unsigned long*)buf1; 
  unsigned long*_buf2 = (unsigned long*)buf2; 
  
  for (i=0; i < fast_int_blocksize; i++) {
    _buf2[i] = _buf1[i] ^ _buf2[i];
  }
#endif

  /*
   * XOR unaligned end of region
   */
  for (i=fast_blocksize; i < blocksize; i++)
  {
    buf2[i] ^= buf1[i];
  }
}

void xor_code_encode(xor_code_t *code_desc, char **data, char **parity, int blocksize)
{
  int i, j;
  
  for (i=0; i < code_desc->k; i++) {
    for (j=0; j < code_desc->m; j++) {
      if (is_data_in_parity(i, code_desc->parity_bms[j])) {
        xor_bufs_and_store(data[i], parity[j], blocksize);
      }
    }
  }
}

void selective_encode(xor_code_t *code_desc, char **data, char **parity, int *missing_parity, int blocksize)
{
  int i;
  for (i=0; i < code_desc->k; i++) {
    int j=0;
    while (missing_parity[j] > -1) {
      int parity_index = missing_parity[j] - code_desc->k;
      if (is_data_in_parity(i, code_desc->parity_bms[parity_index])) {
        xor_bufs_and_store(data[i], parity[parity_index], blocksize);
      }
      j++;
    }
  }
}

int * get_missing_parity(xor_code_t *code_desc, int *missing_idxs)
{
  int *missing_parity = (int*)malloc(sizeof(int)*MAX_PARITY);
  int i = 0, j = 0;

  while (missing_idxs[i] > -1) {
    if (missing_idxs[i] >= code_desc->k) {
      missing_parity[j] = missing_idxs[i]; 
      j++;
    }
    i++;
  }
  
  missing_parity[j] = -1;
  return missing_parity;
}

int * get_missing_data(xor_code_t *code_desc, int *missing_idxs)
{
  int *missing_data = (int*)malloc(sizeof(int)*MAX_DATA);
  int i = 0, j = 0;

  while (missing_idxs[i] > -1) {
    if (missing_idxs[i] < code_desc->k) {
      missing_data[j] = missing_idxs[i]; 
      j++;
    }
    i++;
  }
  
  missing_data[j] = -1;
  return missing_data;
}

/*
 * Reconstruct a single missing symbol, given other symbols may be missing
 */
void xor_reconstruct_one(xor_code_t *code_desc, char **data, char **parity, int *missing_idxs, int index_to_reconstruct, int blocksize)
{
  int *missing_data = get_missing_data(code_desc, missing_idxs);
  int *missing_parity = get_missing_parity(code_desc, missing_idxs);
  int i;

  // If it is a data symbol, we need to figure out
  // what data+parity symbols are needed to reconstruct
  // If there is not at least one parity equation with
  // one missing data element (the index to resonstruct),
  // just call the underlying decode function
  if (index_to_reconstruct < code_desc->k) {
    int connected_parity_idx = index_of_connected_parity(code_desc, index_to_reconstruct, missing_parity, missing_data);

    if (connected_parity_idx >= 0) {
      // Can do a cheap reoncstruction!
      int relative_parity_idx = connected_parity_idx - code_desc->k;
      int parity_bm = code_desc->parity_bms[relative_parity_idx];

      fast_memcpy(data[index_to_reconstruct], parity[relative_parity_idx], blocksize);

      for (i=0; i < code_desc->k; i++) {
        if (parity_bm & (1 << i)) {
          if (i != index_to_reconstruct) {
            xor_bufs_and_store(data[i], data[index_to_reconstruct], blocksize);
          }
        }
      }

    } else {
      // Just call decode
      code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 1);
    }

  } else {

    // If it is a parity symbol, we need to figure out
    // what data symbols are needed to reconstruct the
    // parity.  If *any* data symbols in the parity 
    // equation are missing, we are better off calling
    // the underlying decode function.
    int num_data_missing = num_missing_data_in_parity(code_desc, index_to_reconstruct, missing_data);

    if (num_data_missing == 0) {
      int relative_parity_idx = index_to_reconstruct - code_desc->k;
      int parity_bm = code_desc->parity_bms[relative_parity_idx];   

      memset(parity[relative_parity_idx], 0, blocksize);
      
      for (i=0; i < code_desc->k; i++) {
        if (parity_bm & (1 << i)) {
          xor_bufs_and_store(data[i], parity[relative_parity_idx], blocksize);
        }
      }

    } else {
      // Just call decode
      code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 1);
    }
  }
  free(missing_data);
  free(missing_parity);
}

int num_missing_data_in_parity(xor_code_t *code_desc, int parity_idx, int *missing_data)
{
  int i = 0;
  int num_missing_data = 0;
  int relative_parity_index = parity_idx - code_desc->k;
  if (missing_data == NULL) {
    return 0;
  }

  while (missing_data[i] > -1) {
    if (does_parity_have_data(relative_parity_index, code_desc->data_bms[missing_data[i]]) > 0) {
      num_missing_data++;
    }
    i++;
  }
  
  return num_missing_data;
}

int index_of_connected_parity(xor_code_t *code_desc, int data_index, int *missing_parity, int *missing_data)
{
  int parity_index = -1;
  int i;
  
  for (i=0; i < code_desc->m; i++) {
    if (num_missing_data_in_parity(code_desc, i + code_desc->k, missing_data) > 1) {
      continue;
    }
    if (is_data_in_parity(data_index, code_desc->parity_bms[i])) {
      int j=0;
      int is_missing = 0;
      if (missing_parity == NULL) {
        parity_index = i;
        break;
      }
      while (missing_parity[j] > -1) {
        if ((code_desc->k + i) == missing_parity[j]) {
          is_missing = 1; 
          break; 
        }
        j++;
      }
      if (!is_missing) {
        parity_index = i;
        break;
      }
    }
  }
  
  // Must add k to get the absolute
  // index of the parity in the stripe
  return parity_index > -1 ? parity_index + code_desc->k : parity_index;
}

void remove_from_missing_list(int element, int *missing_list)
{
  int i = 0;
  int elem_idx = -1;
  int num_elems = 0;
  
  while (missing_list[i] > -1) {
    if (missing_list[i] == element) {
      elem_idx = i;
      missing_list[i] = -1;
    }
    i++;
  }

  num_elems = i;

  for (i=elem_idx;i < num_elems-1;i++) {
    int tmp = missing_list[i+1]; 
    missing_list[i+1] = missing_list[i];
    missing_list[i] = tmp;
  }
}

