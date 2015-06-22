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

int* create_non_systematic_vand_matrix(int k, int m);
void free_systematic_matrix(int *matrix);
int* make_systematic_matrix(int k, int m);
int is_missing(int *missing_idxs, int index_to_check);
int gaussj_inversion(int *matrix, int *inverse, int n);
int get_non_zero_diagonal(int *matrix, int row, int num_rows, int num_cols);
int rs_galois_div(int x, int y);
int rs_galois_inverse(int x);
int rs_galois_mult(int x, int y);
void init_liberasurecode_rs_vand(int k, int m);
void deinit_liberasurecode_rs_vand();
void print_matrix(int *matrix, int rows, int cols);
void square_matrix_multiply(int *m1, int *m2, int *prod, int n);
int create_decoding_matrix(int *gen_matrix, int *dec_matrix, int *missing_idxs, int k, int m);
int is_identity_matrix(int *matrix, int n);
int liberasurecode_rs_vand_encode(int *generator_matrix, char **data, char **parity, int k, int m, int blocksize);
int liberasurecode_rs_vand_decode(int *generator_matrix, char **data, char **parity, int k, int m, int *missing, int blocksize, int rebuild_parity);
int liberasurecode_rs_vand_reconstruct(int *generator_matrix, char **data, char **parity, int k, int m, int *missing, int destination_idx, int blocksize);
