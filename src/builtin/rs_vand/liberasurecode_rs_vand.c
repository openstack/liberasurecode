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

// DISCLAIMER: This is a totally basic implementation of RS used if a user does not
// want to install one of the supported backends, such as Jerasure and ISA-L.
// This is not expected to perform as well as the other supported backends,
// but does not make any assumptions about the host system.  Using a library
// like Jerasure with GF-Complete will give users the ability to tune to their
// architecture (Intel or ARM), CPU and memory (lots of options).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <rs_galois.h>
#include <liberasurecode_rs_vand.h>

#include <unistd.h>
#include <fcntl.h>

void print_matrix(int *matrix, int rows, int cols)
{
  int i, j;

  printf("\n");
  for (i = 0; i < rows; i++) {
    for (j = 0; j < cols; j++) {
      printf("%d ", matrix[(i * cols) + j]);
    }
    printf("\n");
  }
  printf("\n");
}

void square_matrix_multiply(int *m1, int *m2, int *prod, int n)
{
  int i, j, k;
  
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      int p = 0;
      for (k = 0; k < n; k++) {
        p ^= rs_galois_mult(m1[(j*n)+k], m2[(k*n)+i]);
      }
      prod[(j*n)+i] = p;
    }
  }
}

int is_identity_matrix(int *matrix, int n)
{
  int i, j;

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      int val = matrix[(i*n) + j];
      if (i != j) {
        if (val != 0) {
          return 0;
        }
      } else {
        if (val != 1) {
          return 0;
        }
      }
    }
  }
  return 1;
}

int* get_matrix_row(int *matrix, int row_idx, int num_cols)
{
  return &matrix[row_idx * num_cols];
}

void copy_row(int *from_matrix, int *to_matrix, int from_row_idx, int to_row_idx, int num_cols)
{
  int *from_row = get_matrix_row(from_matrix, from_row_idx, num_cols);
  int *to_row = get_matrix_row(to_matrix, to_row_idx, num_cols);

  memcpy(to_row, from_row, sizeof(int)*num_cols);
}

int is_missing(int *missing_idxs, int index_to_check)
{
  int i = 0;
  while (missing_idxs[i] > -1) {
    if (missing_idxs[i] == index_to_check) {
      return 1;
    }
    i++;
  }
  return 0;
}

int create_decoding_matrix(int *gen_matrix, int *dec_matrix, int *missing_idxs, int k, int m)
{
  int i, j;
  int n = k+m;

  for (i = 0, j = 0; i < n && j < k; i++) {
    if (!is_missing(missing_idxs, i)) {
      copy_row(gen_matrix, dec_matrix, i, j, k);
      j++; 
    } 
  }

  return j == k;
}


void init_liberasurecode_rs_vand(int k, int m)
{
  rs_galois_init_tables();
}

void deinit_liberasurecode_rs_vand(int k, int m)
{
  rs_galois_deinit_tables();
}

int * create_non_systematic_vand_matrix(int k, int m)
{
  int rows = k + m;
  int cols = k;
  int i, j, acc;
  int *matrix = (int*)malloc(sizeof(int)*rows*cols);

  if (NULL == matrix) return NULL; 

  // First row is 1, 0, 0, ..., 0
  matrix[0] = 1;
  for (i = 1; i < cols; i++) matrix[i] = 0;

  // Other rows are:
  // i^0 (=1), i^1, i^2, ..., i^(cols-1)
  for (i = 1; i < rows; i++) {
    acc = 1;
    for (j = 0; j < cols; j++) {
      matrix[i * cols + j] = acc;
      acc = rs_galois_mult(acc, i);
    }
  }

  return matrix;
}

// Swap the entries of two rows in a matrix
void swap_matrix_rows(int *r1, int *r2, int num_cols)
{
  int i;
  int tmp;

  for (i = 0; i < num_cols; i++) {
    tmp = r1[i];
    r1[i] = r2[i];
    r2[i] = tmp; 
  }
}

void col_mult(int *matrix, int elem, int col_idx, int num_rows, int num_cols)
{
  int i;
   
  for (i = 0; i < num_rows; i++) {
    matrix[col_idx] = rs_galois_mult(matrix[col_idx], elem);
    col_idx += num_cols;
  }
}

void row_mult(int *matrix, int elem, int row_idx, int num_rows, int num_cols)
{
  int i, to_row = row_idx * num_cols;

  for (i = 0; i < num_cols; i++) {
    matrix[to_row] = rs_galois_mult(matrix[to_row], elem);
    to_row++;
  }
}

void col_mult_and_add(int *matrix, int elem, int from_col, int to_col, int num_rows, int num_cols)
{
  int i;
   
  for (i = 0; i < num_rows; i++) {
    matrix[to_col] = matrix[to_col] ^ rs_galois_mult(matrix[from_col], elem);
    from_col += num_cols;
    to_col += num_cols;
  }
}

void row_mult_and_add(int *matrix, int elem, int from_row, int to_row, int num_rows, int num_cols)
{
  int i;
  from_row = from_row * num_cols;
  to_row = to_row * num_cols;
  for (i = 0; i < num_cols; i++) {
    matrix[to_row] = matrix[to_row] ^ rs_galois_mult(matrix[from_row], elem); 
    to_row++;
    from_row++;
  }
}

int get_non_zero_diagonal(int *matrix, int row, int num_rows, int num_cols)
{
  int i, row_idx;
  
  row_idx = (num_cols * row) + row;
  for (i = row; i < num_rows; i++) {
    if (matrix[row_idx] != 0) {
      return i;
    } 
    row_idx += num_cols;
  }

  return -1;
}

int * make_systematic_matrix(int k, int m)
{
  int rows = k + m;
  int cols = k;
  int i, j; 
  int *matrix = create_non_systematic_vand_matrix(k, m);

  if (NULL == matrix) return NULL; 

  // The first row is already 1, 0, 0, ..., 0
  for (i = 1; i < cols; i++) {
    int diag_idx = ((cols*i) + i);
    // Get next row candidate, whose diagonal entry @ i,i != 0
    int next_row = get_non_zero_diagonal(matrix, i, rows, cols);
  
    // Swap candidate row with row i, if needed
    if (next_row != i) {
      swap_matrix_rows(&matrix[next_row*cols], &matrix[i*cols], cols);
    }

    // Ensure the leading entry of row i is 1 by multiplying the 
    // column by the inverse of matrix[diag_idx]
    if (matrix[diag_idx] != 1) {
      col_mult(matrix, rs_galois_inverse(matrix[diag_idx]), i, rows, cols);
    }

    // Zero-out all non-zero, non-diagonal entries in row i
    // by multiplying the corresponding columns by col-i*<row_value>
    for (j = 0; j < cols; j++) {
      int row_val = matrix[(i * cols) + j];
      if (i != j && row_val != 0) {
        col_mult_and_add(matrix, row_val, i, j, rows, cols);
      }
    }
  }

  // Create all-XOR parity as first row of parity submatrix
  for (i = 0; i < cols; i++) {
    int row_val = matrix[(cols * cols) + i];
    if (row_val != 1) {
      // Multiply the parity sub-column by the inverse of row_val
      // We then implicitly multuply row i by the inverse of row_val 
      // (not explicitly necessary, since all other entries are 0)
      col_mult(&matrix[cols*cols], rs_galois_inverse(row_val), i, rows - cols, cols);
    }
  }

  return matrix;
}

void free_systematic_matrix(int *matrix)
{
  free(matrix);
}

int gaussj_inversion(int *matrix, int *inverse, int n)
{
  int i, j;

  // Zero out the inverse matrix
  memset(inverse, 0, sizeof(int)*n*n);

  // Make the inverse matrix an identity matrix
  for (i = 0; i < n; i++) {
    int diag_idx = ((n*i) + i);
    inverse[diag_idx] = 1;
  }

  for (i = 0; i < n; i++) {
    int diag_idx = ((n*i) + i);
    // Get next row candidate, whose diagonal entry @ i,i != 0
    int next_row = get_non_zero_diagonal(matrix, i, n, n);
    
    // Swap candidate row with row i, if needed
    if (next_row != i) {
      swap_matrix_rows(&matrix[next_row*n], &matrix[i*n], n);
      swap_matrix_rows(&inverse[next_row*n], &inverse[i*n], n);
    }

    // Make the leading entry a '1'
    if (matrix[diag_idx] != 1) {
      int leading_val_inv = rs_galois_inverse(matrix[diag_idx]);
      row_mult(matrix, leading_val_inv, i, n, n);
      row_mult(inverse, leading_val_inv, i, n, n);
    }
    
    // Zero-out all other entries in column i
    for (j = 0; j < n; j++) {
      if (i != j) {
        int val = matrix[(j * n) + i];
        row_mult_and_add(matrix, val, i, j, n, n);
        row_mult_and_add(inverse, val, i, j, n, n);
      }
    }
  }
  return 0;
}

void region_xor(char *from_buf, char *to_buf, int blocksize)
{
  int i;
  
  uint32_t *_from_buf = (uint32_t*)from_buf;
  uint32_t *_to_buf = (uint32_t*)to_buf;
  int adj_blocksize = blocksize / 4;
  int trailing_bytes = blocksize % 4;

  for (i = 0; i < adj_blocksize; i++) {
    _to_buf[i] = _to_buf[i] ^ _from_buf[i];
  }
  
  for (i = blocksize-trailing_bytes; i < blocksize; i++) {
    to_buf[i] = to_buf[i] ^ from_buf[i];
  }
}

void region_multiply(char *from_buf, char *to_buf, int mult, int xor, int blocksize)
{
  int i;
  uint16_t *_from_buf = (uint16_t*)from_buf;
  uint16_t *_to_buf = (uint16_t*)to_buf;
  int adj_blocksize = blocksize / 2;
  int trailing_bytes = blocksize % 2;

  if (xor) {
    for (i = 0; i < adj_blocksize; i++) {
      _to_buf[i] = _to_buf[i] ^ (uint16_t)rs_galois_mult(_from_buf[i], mult);
    }
  
    if (trailing_bytes == 1) {
      i = blocksize - 1;
      to_buf[i] = to_buf[i] ^ (char)rs_galois_mult(from_buf[i], mult);
    }
  } else {
    for (i = 0; i < adj_blocksize; i++) {
      _to_buf[i] = (uint16_t)rs_galois_mult(_from_buf[i], mult);
    }
  
    if (trailing_bytes == 1) {
      i = blocksize - 1;
      to_buf[i] = (char)rs_galois_mult(from_buf[i], mult);
    }
  }
}

void region_dot_product(char **from_bufs, char *to_buf, int *matrix_row, int num_entries, int blocksize)
{
  int i;
  
  for (i = 0; i < num_entries; i++) {
    int mult = matrix_row[i];
    if (mult == 1) {
      region_xor(from_bufs[i], to_buf, blocksize);
    } else {
      region_multiply(from_bufs[i], to_buf, mult, 1, blocksize);
    }
  }
}

int liberasurecode_rs_vand_encode(int *generator_matrix, char **data, char **parity, int k, int m, int blocksize)
{
  int i;
  int n = k + m;
  
  for (i = k; i < n; i++) {
    memset(parity[i - k], 0, blocksize);
    region_dot_product(data, parity[i - k], &generator_matrix[(i * k)], k, blocksize);
  } 

  return 0;
}

char **get_first_k_available(char **data, char **parity, int *missing, int k)
{
  int i, j;
  char **first_k_available = (char**)malloc(sizeof(char*)*k);
  
  for (i = 0, j = 0; j < k; i++) {
    if (!missing[i]) {
      first_k_available[j] = i < k ? data[i] : parity[i - k];
      j++;
    }
  }
  return first_k_available;
}

int liberasurecode_rs_vand_decode(int *generator_matrix, char **data, char **parity, int k, int m, int *missing, int blocksize, int rebuild_parity)
{
  int *decoding_matrix = NULL;
  int *inverse_decoding_matrix = NULL;
  char **first_k_available = NULL;
  int n = m + k;
  int *_missing = (int*)malloc(sizeof(int)*n);
  int i = 0;
  int num_missing = 0;

  memset(_missing, 0, sizeof(int)*n);

  while (missing[num_missing] > -1) {
    _missing[missing[num_missing]] = 1;
    num_missing++;
  }

  if (num_missing > m) {
    free(_missing);
    return -1;
  }

  decoding_matrix = (int*)malloc(sizeof(int)*k*k);
  inverse_decoding_matrix = (int*)malloc(sizeof(int)*k*k);
  first_k_available = get_first_k_available(data, parity, _missing, k);
  
  create_decoding_matrix(generator_matrix, decoding_matrix, missing, k, m);
  gaussj_inversion(decoding_matrix, inverse_decoding_matrix, k);

  // Rebuild data fragments
  for (i = 0; i < k; i++) {
    // Data fragment i is missing, recover it
    if (_missing[i]) {
      region_dot_product(first_k_available, data[i], &inverse_decoding_matrix[(i * k)], k, blocksize);
    }
  }
  
  // Rebuild parity fragments
  if (rebuild_parity) {
    for (i = k; i < n; i++) {
      // Parity fragment i is missing, recover it
      if (_missing[i]) {
        region_dot_product(data, parity[i - k], &generator_matrix[(i * k)], k, blocksize);
      }
    }
  }
  
  free(decoding_matrix);
  free(inverse_decoding_matrix);
  free(first_k_available);
  free(_missing);

  return 0;
}

int liberasurecode_rs_vand_reconstruct(int *generator_matrix, char **data, char **parity, int k, int m, int *missing, int destination_idx, int blocksize)
{
  int *decoding_matrix = NULL;
  int *inverse_decoding_matrix = NULL;
  char **first_k_available = NULL;
  int *parity_row = NULL;
  int n = k + m;
  int *_missing = (int*)malloc(sizeof(int)*n);
  int i, j;
  int num_missing = 0;

  memset(_missing, 0, sizeof(int)*n);

  while (missing[num_missing] > -1) {
    _missing[missing[num_missing]] = 1;
    num_missing++;
  }

  if (num_missing > m) {
    free(_missing);
    return -1;
  }

  decoding_matrix = (int*)malloc(sizeof(int)*k*k);
  inverse_decoding_matrix = (int*)malloc(sizeof(int)*k*k);
  first_k_available = get_first_k_available(data, parity, _missing, k);
  
  create_decoding_matrix(generator_matrix, decoding_matrix, missing, k, m);
  gaussj_inversion(decoding_matrix, inverse_decoding_matrix, k);

  // Rebuilding data is easy, just do a dot product using the inverted decoding
  // matrix
  if (destination_idx < k) {
    region_dot_product(first_k_available, data[destination_idx], &inverse_decoding_matrix[(destination_idx * k)], k, blocksize);
  } else {
    // Rebuilding parity is a little tricker, we first copy the corresp. parity row
    // and update it to reconstruct the parity with the first k available elements

    // Copy the parity entries for available data elements
    // from the original generator matrix
    parity_row = (int*)malloc(sizeof(int)*k);
    memset(parity_row, 0, sizeof(int)*k);
    j = 0;
    for (i = 0; i < k; i++) {
      if (!_missing[i]) {
        parity_row[j] = generator_matrix[(destination_idx * k) + i];
        j++;
      } 
    }

    i = 0;
    // For each missing data element, we substitute in the row (equation) for the data element into the
    // the parity row.
    while (missing[i] > -1) {
      if (missing[i] < k) {
        for (j = 0; j < k; j++) {
          parity_row[j] ^= rs_galois_mult(generator_matrix[(destination_idx * k) + missing[i]], inverse_decoding_matrix[(missing[i] * k) + j]);
        }
      }
      i++;
    }
    region_dot_product(first_k_available, parity[destination_idx - k], parity_row, k, blocksize);
  }
  
  free(decoding_matrix);
  free(inverse_decoding_matrix);
  free(first_k_available);
  free(_missing);

  return 0;
}
