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

#ifndef _XOR_CODE_H
#define _XOR_CODE_H

#define MAX_DATA 32
#define MAX_PARITY MAX_DATA

#define MEM_ALIGN_SIZE 16

#define DECODED_MISSING_IDX MAX_DATA

typedef enum { FAIL_PATTERN_GE_HD, // Num failures greater than or equal to HD
               FAIL_PATTERN_0D_0P, 
               FAIL_PATTERN_1D_0P, 
               FAIL_PATTERN_2D_0P, 
               FAIL_PATTERN_3D_0P, 
               FAIL_PATTERN_1D_1P, 
               FAIL_PATTERN_1D_2P, 
               FAIL_PATTERN_2D_1P, 
               FAIL_PATTERN_0D_1P, 
               FAIL_PATTERN_0D_2P, 
               FAIL_PATTERN_0D_3P } failure_pattern_t;

#define is_aligned(x) (((unsigned long)x & (MEM_ALIGN_SIZE-1)) == 0)
#define num_unaligned_end(size) (size % MEM_ALIGN_SIZE)

struct xor_code_s;

typedef struct xor_code_s
{
  int k;
  int m;
  int hd;
  int *parity_bms;
  int *data_bms;
  void (*decode)(struct xor_code_s *code_desc, char **data, char **parity, int *missing_idxs, int blocksize, int decode_parity);
  void (*encode)(struct xor_code_s *code_desc, char **data, char **parity, int blocksize);
  int (*fragments_needed)(struct xor_code_s *code_desc, int *missing_idxs, int *fragments_needed);
} xor_code_t;

int is_data_in_parity(int data_idx, unsigned int parity_bm);

int does_parity_have_data(int parity_idx, unsigned int data_bm);

int parity_bit_lookup(xor_code_t *code_desc, int index);

int data_bit_lookup(xor_code_t *code_desc, int index);

int missing_elements_bm(xor_code_t *code_desc, int *missing_elements, int (*bit_lookup_func)(xor_code_t *code_desc, int index));

failure_pattern_t get_failure_pattern(xor_code_t *code_desc, int *missing_idxs);

void fast_memcpy(char *dst, char *src, int size);

void xor_bufs_and_store(char *buf1, char *buf2, int blocksize);

void xor_code_encode(xor_code_t *code_desc, char **data, char **parity, int blocksize);

void selective_encode(xor_code_t *code_desc, char **data, char **parity, int *missing_parity, int blocksize);

int * get_missing_parity(xor_code_t *code_desc, int *missing_idxs);

int * get_missing_data(xor_code_t *code_desc, int *missing_idxs);

int num_missing_data_in_parity(xor_code_t *code_desc, int parity_idx, int *missing_data);

int index_of_connected_parity(xor_code_t *code_desc, int data_index, int *missing_parity, int *missing_data);

void remove_from_missing_list(int element, int *missing_list);

int* get_symbols_needed(xor_code_t *code_desc, int *missing_list);

void xor_reconstruct_one(xor_code_t *code_desc, char **data, char **parity, int *missing_idxs, int index_to_reconstruct, int blocksize);

xor_code_t* init_xor_hd_code(int k, int m, int hd);

#endif
