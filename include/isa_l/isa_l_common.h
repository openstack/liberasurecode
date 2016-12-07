/*
 * Copyright 2014 Kevin M Greenan
 * Copyright 2014 Tushar Gohad
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
 * isa_l_rs_vand backend implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#define ISA_L_W 8

/* Forward declarations */
typedef void (*ec_encode_data_func)(int, int, int, unsigned char*, unsigned char **, unsigned char **);
typedef void (*ec_init_tables_func)(int, int, unsigned char*, unsigned char *);
typedef void (*gf_gen_encoding_matrix_func)(unsigned char*, int, int);
typedef int (*gf_invert_matrix_func)(unsigned char*, unsigned char*, const int);
typedef unsigned char (*gf_mul_func)(unsigned char, unsigned char);

typedef struct {
    /* calls required for init */
    ec_init_tables_func ec_init_tables;
    gf_gen_encoding_matrix_func gf_gen_encoding_matrix;

    /* calls required for encode */
    ec_encode_data_func ec_encode_data;

    /* calls required for decode and reconstruct */
    gf_invert_matrix_func gf_invert_matrix;

    /* multiplication function used by ISA-L */
    gf_mul_func gf_mul;

    /* fields needed to hold state */
    unsigned char *matrix;
    int k;
    int m;
    int w;
} isa_l_descriptor;

int isa_l_encode(void *desc, char **data, char **parity, int blocksize);
int isa_l_decode(void *desc, char **data, char **parity, int *missing_idxs,
        int blocksize);
int isa_l_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize);
int isa_l_min_fragments(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed);
int isa_l_element_size(void* desc);
int isa_l_exit(void *desc);
void * isa_l_common_init(struct ec_backend_args *args, void *backend_sohandle,
        const char* gen_matrix_func_name);
