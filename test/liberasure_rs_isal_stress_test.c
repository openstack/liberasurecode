/*
 * Copyright 2025 aitassou
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
 * liberasurecode frontend API header
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include "erasurecode.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"
#include "erasurecode_preprocessing.h"


char *create_buffer(int size, int fill)
{
    char *buf = malloc(size);
    if (buf == NULL) {
        return NULL;
    }
    int urand = open("/dev/urandom", O_RDONLY);
    if (urand < 0 || read(urand, buf, size) != size) {
        for(size_t i = 0; i < size; i++)
            buf[i] = rand() % 256;
    }
    if (urand >= 0) {
        close(urand);
    }
    return buf;
}

int *create_skips_array(struct ec_args *args, int skip)
{
    int num = args->k + args->m;
    size_t array_size = sizeof(int) * num;
    int *buf = malloc(array_size);
    if (buf == NULL) {
        return NULL;
    }
    memset(buf, 0, array_size);
    if (skip >= 0 && skip < num) {
        buf[skip] = 1;
    }
    return buf;
}

static int create_frags_array(char ***array,
                              char **data,
                              char **parity,
                              struct ec_args *args,
                              int *skips)
{
    // N.B. this function sets pointer reference to the ***array
    // from **data and **parity so DO NOT free each value of
    // the array independently because the data and parity will
    // be expected to be cleanup via liberasurecode_encode_cleanup
    int num_frags = 0;
    int i = 0;
    char **ptr = NULL;
    *array = malloc((args->k + args->m) * sizeof(void *));
    if (array == NULL) {
        num_frags = -1;
        goto out;
    }
    //add data frags
    ptr = *array;
    for (i = 0; i < args->k; i++) {
        if (data[i] == NULL || skips[i] == 1)
        {
            continue;
        }
        *ptr++ = data[i];
        num_frags++;
    }
    //add parity frags
    for (i = 0; i < args->m; i++) {
        if (parity[i] == NULL || skips[i + args->k] == 1) {
            continue;
        }
        *ptr++ = parity[i];
        num_frags++;
    }
out:
    return num_frags;
}

static void encode_decode_test_impl(const ec_backend_id_t be_id,
                                   struct ec_args *args,
                                   int *skip)
{
    int i = 0;
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024;
    char *orig_data = NULL;
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    uint64_t decoded_data_len = 0;
    char *decoded_data = NULL;
    size_t frag_header_size =  sizeof(fragment_header_t);
    char **avail_frags = NULL;
    int num_avail_frags = 0;
    char *orig_data_ptr = NULL;
    int remaining = 0;

    desc = liberasurecode_instance_create(be_id, args);

    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    } else if ((args->k + args->m) > EC_MAX_FRAGMENTS) {
        fprintf(stderr, "data + parity is greater than %d!\n", EC_MAX_FRAGMENTS);
        assert(-EINVALIDPARAMS == desc);
        return;
    } else
        assert(desc > 0);

    orig_data = create_buffer(orig_data_size, 'x');
    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);
    orig_data_ptr = orig_data;
    remaining = orig_data_size;
    for (i = 0; i < args->k + args->m; i++)
    {
        int cmp_size = -1;
        char *data_ptr = NULL;
        char *frag = NULL;

        frag = (i < args->k) ? encoded_data[i] : encoded_parity[i - args->k];
        assert(frag != NULL);
        fragment_header_t *header = (fragment_header_t*)frag;
        assert(header != NULL);

        fragment_metadata_t metadata = header->meta;
        assert(metadata.idx == i);
        assert(metadata.size == encoded_fragment_len - frag_header_size - metadata.frag_backend_metadata_size);
        assert(metadata.orig_data_size == orig_data_size);
        assert(metadata.backend_id == be_id);
        assert(metadata.chksum_mismatch == 0);
        data_ptr = frag + frag_header_size;
        cmp_size = remaining >= metadata.size ? metadata.size : remaining;
        // shss & libphazr doesn't keep original data on data fragments
        if (be_id != EC_BACKEND_SHSS && be_id != EC_BACKEND_LIBPHAZR) {
            assert(memcmp(data_ptr, orig_data_ptr, cmp_size) == 0);
        }
        remaining -= cmp_size;
        orig_data_ptr += metadata.size;
    }

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, args, skip);
    assert(num_avail_frags > 0);
    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, &decoded_data_len);
    assert(0 == rc);
    assert(decoded_data_len == orig_data_size);
    assert(memcmp(decoded_data, orig_data, orig_data_size) == 0);

    rc = liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    assert(rc == 0);

    rc = liberasurecode_decode_cleanup(desc, decoded_data);
    assert(rc == 0);

    assert(0 == liberasurecode_instance_destroy(desc));
    free(orig_data);
    free(avail_frags);
}


static void test_decode_with_missing_multi_data_parity(
    const ec_backend_id_t be_id, struct ec_args *args, uint64_t nb_iter)
{
    uint64_t i;
    for (i = 0; i < nb_iter; i++) {
        int *skip = create_skips_array(args,-1);
        assert(skip != NULL);
        int count = 0;
        while(true) {
                int value = rand() % (args->k + args->m) ;

                if (skip[value] != 1){
                    skip[value] = 1;
                    count++;
                }

                if(count == args->m) {
                    break;
                }
          }
        encode_decode_test_impl(be_id, args, skip);
        free(skip);
    }
}


int main(int argc, char **argv)
{
    if (argc != 4) {
        printf("Stress Test with m missing positions for RS(k,m).\n");
        printf("Number of iterations {nb_iter} should be at least ~ (k+m)!/(k!)(m!).\n");
        printf("Usage: %s <message_length> <parity_length> <nb_iter>\n", argv[0]);
        return 0;
    }
    int k = atoi(argv[1]);
    int m = atoi(argv[2]);
    u_int64_t nb_iter =  atoll(argv[3]);
    struct ec_args isa_l_km_args = {
        .k = k,
        .m = m,
        .w = 8,
        .hd = m+1,
    };

    // Test is ok if no Assertion `0 == rc' occurs
    // We can make backend configurable  and test EC_BACKEND_ISA_L_RS_VAND
    test_decode_with_missing_multi_data_parity(EC_BACKEND_ISA_L_RS_VAND_INV, &isa_l_km_args, nb_iter);

    return 0;
}
