/*
 * Copyright 2014 Eric Lambert
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

/*
 * EDL 8/26/2014: This test is based on the test/test_xor_hd_code. It runs
 * through a similar set of conditions but instead uses the liberasurecode
 * API as opposed to directly talking to xor implementation. In the original
 * test_xor_hd_code, we measured the performance of a series of encode/decode
 * ops. For the time being, I have "disabled" the performance measurement in
 * this test ... the main reason for doing so was that we need to some more
 * memory management when using the API and I did not want those management
 * ops polluting the resutls. When I have some time I will renable address
 * this (figure out how to make sure memory management does not affect perf 
 * numbers).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "erasurecode.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"
#include "builtin/xor_codes/test_xor_hd_code.h"

struct frag_array_set {
    unsigned int num_fragments;
    char **array;
};

void print_mask(unsigned long mask)
{
    unsigned int i = 0;
    long pos = 1;

    if (mask == 0) {
        fprintf(stderr,"  No Missing fragments");
        return;
    }
    fprintf(stderr,"  Missing fragments = ");
    for (i = 0; i < (sizeof(size_t) * 8) - 1; i++) {
        if ((mask & (pos << i)) != 0) {
            fprintf(stderr,"%d ",i);
        }
    }
}

void missing_mask_to_array(long mask, int *missing)
{
    unsigned int i = 0;
    long pos = 1;

    for (i = 0; i < (sizeof(size_t) * 8) - 1; i++) {
        if ((mask & (pos << i)) != 0) {
            *missing = i;
        }
    }
}

size_t add_item_to_missing_mask(unsigned long mask, long pos) 
{
    if (pos < 0) {
        return mask;
    }
    unsigned long f = 1L << pos;
    mask |= f;
    return mask;
}

static int create_frags_array_set(struct frag_array_set *set,
        char **data,
        unsigned int num_data_frags,
        char **parity,
        unsigned int num_parity_frags,
        unsigned long missing_mask)
{
    int rc =0;
    unsigned int num_frags = 0;
    unsigned long i = 0;
    fragment_header_t *header = NULL;
    size_t size = (num_data_frags + num_parity_frags) * sizeof(char *);
    char **array = malloc(size);

    if (array == NULL) {
        rc = -1;
        goto out;
    }

    //add data frags
    memset(array, 0, size);
    for (i = 0; i < num_data_frags; i++) {
        if ( (missing_mask | 1L << i) == 1) {
            continue;
        }
        header = (fragment_header_t*)data[i];
        if (header == NULL ||
                header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
            continue;
        }
        array[num_frags++] = data[i];
    }

    //add parity frags
    for (i = 0; i < num_parity_frags; i++) {
        if ( (missing_mask | 1L << (i + num_data_frags)) == 1) {
            continue;
        }
        header = (fragment_header_t*)parity[i];
        if (header == NULL ||
                header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
            continue;
        }
        array[num_frags++] = parity[i];
    }

    set->num_fragments = num_frags;
    set->array = array;
out:
    return rc;
}

static void fill_buffer(char *buf, size_t size, int seed)
{
    size_t i;

    for (i=0; i < size; i++) {
        buf[i] = (seed += i);
    }
}

static int test_hd_code(struct ec_args *args,
    int num_failure_combs,
    int failure_combs[][4])
{
    int i, j, err;
    unsigned int num_iter = 1000;
    size_t blocksize = 32768;
    int missing_idxs[4] = { -1, -1, -1, -1 };
    int excluded_idxs[4] = { -1, -1, -1, -1 };
    int ret = 0;
    char *data, **parity;
    int *fragments_needed;
    char **encoded_data = NULL; 
    char **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int rc = 0;
    char *out_data = NULL;
    uint64_t out_data_len = 0;
    unsigned long mask = 0;
    int desc = -1;
    struct frag_array_set frags; //MOVE ME

    srand(time(NULL));

    /*
     * Set up data and parity fragments. 
     */

    fragments_needed = (int*)malloc(args->k*args->m*sizeof(int));
    if (!fragments_needed) {
        fprintf(stderr, "Could not allocate memory for fragments\n");
        exit(2);
    }
    memset(fragments_needed, 0,  args->k*args->m*sizeof(int));

    err = posix_memalign((void **) &data, 16, blocksize * args->k);
    if (err != 0 || !data) {
        fprintf(stderr, "Could not allocate memory for data\n");
        exit(1);
    }
    fill_buffer(data, blocksize * args->k, 0);

    parity = (char**)malloc(args->m * sizeof(char*));
    for (i=0; i < args->m; i++) {
        err = posix_memalign((void **) &parity[i], 16, blocksize);
        if (err != 0 || !parity[i]) {
            fprintf(stderr, "Could not allocate memory for parity %d\n", i);
            exit(1);
        }
        memset(parity[i], 0, blocksize);
    }

    /*
     * Get handle
     */
    desc = liberasurecode_instance_create(EC_BACKEND_FLAT_XOR_HD, args);
    if (desc <= 0) {
        fprintf(stderr, "Could not create libec descriptor\n");
        exit(1);
    }

    /*
     * Run Encode test
     */
    for (i=0; i < num_iter-1; i++) {
        rc = liberasurecode_encode(desc, data, blocksize * args->k,
                                   &encoded_data, &encoded_parity,
                                   &encoded_fragment_len);
        //FIXME: this and the following free's taint the perf test
        assert(0 == rc);
        for (j = 0; j < args->k; j++) {
            free(encoded_data[j]);    
        }
        free(encoded_data);
        for (j = 0; j < args->m; j++) {
            free(encoded_parity[j]);    
        }
        free(encoded_parity);
    }
    fprintf(stderr, " Encode: OK\n");

    for (i=0; i < args->m; i++) {
        memset(parity[i], 0, blocksize);
    }
    rc = liberasurecode_encode(desc, data, blocksize * args->k, &encoded_data,
                               &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);

    /*
     * Run Decode Test
     */
    for (i=0; i < num_failure_combs; i++) {
        mask = 0;
        for (j = 0; j < 3; j++) {
            int idx = failure_combs[i][j];
            if (idx == -1) {
                continue;
            }
            mask = add_item_to_missing_mask(mask, idx);
        }

        /*
         * Spot check to ensure missing elements are not included in
         * list of fragments needed and that decode is 'doable'
         */
        missing_mask_to_array(mask, missing_idxs);
        ret = liberasurecode_fragments_needed(desc, missing_idxs, excluded_idxs,
                                              fragments_needed); //known leak
        if (ret < 0) {
            fprintf(stderr,"xor_hd_fragments_needed thinks reconstruction not possible, when it is!\n");
            exit(2);
        }

        /*
         * Make sure that none of the missig fragments are in the set of
         * fragments needed to reconstruct the object.
         */
        j = 0;
        while (fragments_needed[j] > -1) {
            if (fragments_needed[j] == missing_idxs[0] ||
                fragments_needed[j] == missing_idxs[1] ||
                fragments_needed[j] == missing_idxs[2]) {
                fprintf(stderr, 
                        "fragments_needed[%d]=%d in missing index list: (%d %d %d)!\n",
                        j, fragments_needed[j], missing_idxs[0],
                        missing_idxs[1], missing_idxs[2]);
                exit(2);
            }
            j++;
        }
        create_frags_array_set(&frags,encoded_data, args->k, encoded_parity,
                               args->m, mask);
        rc = liberasurecode_decode(desc, frags.array, frags.num_fragments,
                                   encoded_fragment_len, 1,
                                   &out_data, &out_data_len);
        assert(rc == 0);
        assert(out_data_len == blocksize * args->k);
        if (memcmp(data, out_data, out_data_len) != 0) {
            fprintf(stderr, "Decode did not work: (%d %d %d)!\n",
                    missing_idxs[0], missing_idxs[1], missing_idxs[2]);
            exit(2);
        }
        free(frags.array);
        free(out_data);
    }

    for (i=0; i < num_iter; i++) {
        mask = 0;
        int mi = rand() % (args->k + args->m);

        mask = add_item_to_missing_mask(mask, mi);
        for (j=1; j < args->hd-1;j++) {
            mi = mi + 1 % (args->k + args->m);
            mask = add_item_to_missing_mask(mask, mi);
        }
        create_frags_array_set(&frags,encoded_data, args->k, encoded_parity,
                               args->m, mask);
        rc = liberasurecode_decode(desc, frags.array, frags.num_fragments,
                                   encoded_fragment_len, 1,
                                   &out_data, &out_data_len);
        free(frags.array);
        free(out_data);

        assert(rc == 0);
        //print_mask(mask);
    }
    for (j = 0; j < args->k; j++) {
        free(encoded_data[j]);    
    }
    free(encoded_data);
    for (j = 0; j < args->m; j++) {
        free(encoded_parity[j]);    
    }
    free(encoded_parity);
    free(fragments_needed);
    free(data);
    for (i = 0; i < args->m; i++) {
        free(parity[i]);
    }
    free(parity);

    liberasurecode_instance_destroy(desc);
    return 0;
}

static int run_test(int k, int m, int hd)
{
    int ret = -1;
    struct ec_args args = {
        .k = k,
        .m = m,
        .hd = hd,
    };

    fprintf(stderr, "Running (%d, %d, %d):\n", k, m, hd);

    switch(k+m)
    {
        case 10:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_10_3_COMBS, failure_combs_10_3);
            } else {
                ret = test_hd_code(&args, NUM_10_4_COMBS, failure_combs_10_4);
            }
            break;
        case 11:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_11_3_COMBS, failure_combs_11_3);
            } else {
                ret = test_hd_code(&args, NUM_11_4_COMBS, failure_combs_11_4);
            }
            break;
        case 12:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_12_3_COMBS, failure_combs_12_3);
            } else {
                ret = test_hd_code(&args, NUM_12_4_COMBS, failure_combs_12_4);
            }
            break;
        case 13:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_13_3_COMBS, failure_combs_13_3);
            } else {
                ret = test_hd_code(&args, NUM_13_4_COMBS, failure_combs_13_4);
            }
            break;
        case 14:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_14_3_COMBS, failure_combs_14_3);
            } else {
                ret = test_hd_code(&args, NUM_14_4_COMBS, failure_combs_14_4);
            }
            break;
        case 15:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_15_3_COMBS, failure_combs_15_3);
            } else {
                ret = test_hd_code(&args, NUM_15_4_COMBS, failure_combs_15_4);
            }
            break;
        case 16:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_16_3_COMBS, failure_combs_16_3);
            } else {
                ret = test_hd_code(&args, NUM_16_4_COMBS, failure_combs_16_4);
            }
            break;
        case 17:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_17_3_COMBS, failure_combs_17_3);
            } else {
                ret = test_hd_code(&args, NUM_17_4_COMBS, failure_combs_17_4);
            }
            break;
        case 18:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_18_3_COMBS, failure_combs_18_3);
            } else {
                ret = test_hd_code(&args, NUM_18_4_COMBS, failure_combs_18_4);
            }
            break;
        case 19:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_19_3_COMBS, failure_combs_19_3);
            } else {
                ret = test_hd_code(&args, NUM_19_4_COMBS, failure_combs_19_4);
            }
            break;
        case 20:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_20_3_COMBS, failure_combs_20_3);
            } else {
                ret = test_hd_code(&args, NUM_20_4_COMBS, failure_combs_20_4);
            }
            break;
        case 21:
            if (hd == 3) {
                ret = test_hd_code(&args, NUM_21_3_COMBS, failure_combs_21_3);
            } else {
                ret = test_hd_code(&args, NUM_21_4_COMBS, failure_combs_21_4);
            }
            break;
        case 22:
            ret = test_hd_code(&args, NUM_22_4_COMBS, failure_combs_22_4);
            break;
        case 23:
            ret = test_hd_code(&args, NUM_23_4_COMBS, failure_combs_23_4);
            break;
        case 24:
            ret = test_hd_code(&args, NUM_24_4_COMBS, failure_combs_24_4);
            break;
        case 25:
            ret = test_hd_code(&args, NUM_25_4_COMBS, failure_combs_25_4);
            break;
        case 26:
            ret = test_hd_code(&args, NUM_26_4_COMBS, failure_combs_26_4);
            break;
        default:
            ret = -1; 
    }
    return ret; 
}

int main()
{
    int ret = 0;
    int i;
    for (i=6; i < 16; i++) {
        ret = run_test(i, 6, 3);
        if (ret != 0) {
            return ret;
        }
    }

    for (i=5; i < 11; i++) {
        ret = run_test(i, 5, 3);
        if (ret != 0) {
            return ret;
        }
    }

    for (i=6; i < 21; i++) {
        ret = run_test(i, 6, 4);
        if (ret != 0) {
            return ret;
        }
    }
    for (i=5; i < 11; i++) {
        ret = run_test(i, 5, 4);
        if (ret != 0) {
            return ret;
        }
    }
    exit(ret);
}

