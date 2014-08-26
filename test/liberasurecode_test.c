/* 
 * <Copyright>
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
#include <stdbool.h>
#include "erasurecode.h"
#include "erasurecode_helpers.h"
#include "erasurecode_backend.h"

typedef void (*TEST_FUNC)();

struct testcase {
    const char *description;
    TEST_FUNC function;
    void *arg1;
    void *arg2;
    bool skip;
};

//TODO Make this a but more useful
char *create_buffer(int size, int fill)
{
    char *buf = malloc(size);
    memset(buf, fill, size);
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
    int num_frags = 0;
    int i = 0;
    char **ptr = NULL;
    *array = malloc((args->k + args->m) * sizeof(char *));
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

static void test_liberasurecode_supported_backends()
{
    int i, num_backends;
    const char **supported_ec_backends =
        liberasurecode_supported_backends(&num_backends);

    for (i = 0; i < num_backends; i++) {
        printf("%s\n", supported_ec_backends[i]);
    }
}

static void test_liberasurecode_supported_checksum_types()
{
    int i;
    int num_checksum_types;
    const char **supported_checksum_types =
        liberasurecode_supported_checksum_types(&num_checksum_types);

    assert(num_checksum_types == CHKSUM_TYPES_MAX);
    for (i = 0; i < CHKSUM_TYPES_MAX; i++) {
        printf("%s\n", supported_checksum_types[i]);
    }
}

static void test_create_and_destroy_backend(
        const char *backend,
        struct ec_args *args)
{
    int desc = liberasurecode_instance_create(backend, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf (stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);
    assert(0 == liberasurecode_instance_destroy(desc));
}

static void encode_decode_test_impl(const char *backend,
                                   struct ec_args *args,
                                   int *skip)
{
    int i = 0;
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = NULL;
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int num_fragments = args-> k + args->m;
    uint64_t decoded_data_len = 0;
    char *decoded_data = NULL;
    size_t frag_header_size =  sizeof(fragment_header_t);
    char **avail_frags = NULL;
    int num_avail_frags = 0;
    char *orig_data_ptr = NULL;
    int remaining = 0;

    desc = liberasurecode_instance_create(backend, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf (stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    orig_data = create_buffer(orig_data_size, 'x');
    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);
    orig_data_ptr = orig_data;
    remaining = orig_data_size;
    for (i = 0; i < args->k; i++)
    {
        char *frag = encoded_data[i];
        fragment_header_t *header = (fragment_header_t*)frag;
        assert(header != NULL);
        fragment_metadata_t metadata = header->meta;
        assert(metadata.idx == i);
        assert(metadata.size == encoded_fragment_len - frag_header_size);
        assert(metadata.orig_data_size == orig_data_size);
        unsigned char *data_ptr = frag + frag_header_size;
        int cmp_size = remaining >= metadata.size ? metadata.size : remaining;
        assert(memcmp(data_ptr, orig_data_ptr, cmp_size) == 0); 
        remaining -= cmp_size;
        orig_data_ptr += metadata.size;
    }

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, args, skip);
    assert(num_avail_frags != -1);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, &decoded_data,
                               &decoded_data_len);
    assert(0 == rc);
    assert(decoded_data_len == orig_data_size);
    assert(memcmp(decoded_data, orig_data, orig_data_size) == 0);

    if (desc) {
        assert(0 == liberasurecode_instance_destroy(desc));
    }

    free(orig_data);
    if (avail_frags != NULL) {
        int idx = 0;
        for (idx = 0; idx < num_avail_frags; idx++) {
            free(avail_frags[idx]);
        }
        free(avail_frags);
    }
}

static void reconstruct_test_impl(const char *backend,
                                 struct ec_args *args,
                                 int *skip)
{
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = NULL;
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int num_fragments = args-> k + args->m;
    uint64_t decoded_data_len = 0;
    char *decoded_data = NULL;
    char **avail_frags = NULL;
    int num_avail_frags = 0;
    int i = 0;
    char *out = NULL;

    desc = liberasurecode_instance_create(backend, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf (stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    orig_data = create_buffer(orig_data_size, 'x');
    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);
    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                             encoded_parity, args, skip);
    out = malloc(encoded_fragment_len);
    assert(out != NULL);
    for (i = 0; i < num_fragments; i++) {
        if (skip[i] == 0) {
            continue;
        }
        char *cmp = NULL;
        if (i < args->k) {
            cmp = encoded_data[i];
        }
        else {
            cmp = encoded_parity[i - args->k];
        }
        memset(out, 0, encoded_fragment_len);
        rc = liberasurecode_reconstruct_fragment(desc, avail_frags, num_avail_frags, encoded_fragment_len, i, out);
        assert(rc == 0);
        assert(memcmp(out, cmp, encoded_fragment_len) == 0);
    }
}

static void test_fragments_needed_impl(const char *backend,
                                      struct ec_args *args)
{
    int *fragments_to_reconstruct = NULL;
    int *fragments_to_exclude = NULL;
    int *fragments_needed = NULL;
    int *new_fragments_needed = NULL;
    int desc = liberasurecode_instance_create(backend, args);
    int ret = -1;
    int i = 0, j = 0;
    int n = args->k + args->m;

    if (-EBACKENDNOTAVAIL == desc) {
        fprintf (stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    /*
     * ToDo (KMG): In an effort to make this test "general"
     * it makes assumptions about how reconstruction equations
     * are derived.  It assumes the lowest-numbered parity
     * will be used when reconstructing a single failure.
     * This is typically true for many RS implementations,
     * since the first parity is the XOR of all data elements.
     * This is also the case for our internal flat XOR implementation.
     *
     * We will have to do something else for more complicated cases...
     *
     * Here is the gist:
     *
     * First get all of the data elements connected to the
     * first parity element.  Select one of the data elements
     * as the item to reconstruct and select one not in that
     * set as the missing element.  Elements needed should
     * be equal to the parity element, plus all other data 
     * elements connected to it.
     *
     * Simple example with XOR code (k=10, m=5):
     *
     * p_0 = d_0 + d_1 + d_2
     * p_1 = d_0 + d_3 + d_5
     * ...
     *
     * Call to fragments_needed(desc, [0, -1], [3, -1], [])
     * should return: [10, 1, 2]
     */
    fragments_to_reconstruct = (int*)malloc(sizeof(int) * args->k);
    assert(fragments_to_reconstruct != NULL);
    fragments_to_exclude = (int*)malloc(sizeof(int) * args->k);
    assert(fragments_to_exclude != NULL);
    fragments_needed = (int*)malloc(sizeof(int) * args->k);
    assert(fragments_needed != NULL);
    new_fragments_needed = (int*)malloc(sizeof(int) * args->k);
    assert(fragments_needed != NULL);

    // This is the first parity element
    fragments_to_reconstruct[0] = args->k;
    fragments_to_reconstruct[1] = -1;
    fragments_to_exclude[0] = -1;

    ret = liberasurecode_fragments_needed(desc, 
                                          fragments_to_reconstruct, 
                                          fragments_to_exclude, 
                                          fragments_needed);
    assert(ret > -1);
    
    // "Reconstruct" the first data in the parity equation
    fragments_to_reconstruct[0] = fragments_needed[0];
    fragments_to_reconstruct[1] = -1;

    fragments_to_exclude[0] = -1;
    // Find a proper fragment to exlcude
    for (i = 0; i < n; i++) {
        j = 1;
        while (fragments_needed[j] > -1) {
            if (fragments_needed[j] == i) {
                break;
            }
            j++;
        }
        // Found one!
        if (fragments_needed[j] == -1) {
            fragments_to_exclude[0] = i;
            fragments_to_exclude[1] = -1;
            break;
        }
    }

    assert(fragments_to_exclude[0] > -1);

    ret = liberasurecode_fragments_needed(desc, 
                                          fragments_to_reconstruct, 
                                          fragments_to_exclude, 
                                          new_fragments_needed);
    assert(ret > -1);
   
    // Verify that new_fragments_needed contains the
    // first parity element and all data elements connected
    // to that parity element sans the data to reconstruct.
    i = 0;
    while (new_fragments_needed[i] > -1) {
        int is_valid_fragment = 0;

        // This is the first parity
        if (new_fragments_needed[i] == args->k) {
            is_valid_fragment = 1;
        } else {
            // This checks for all of the other data elements
            j = 1;
            while (fragments_needed[j] > -1) {
                if (fragments_needed[j] == new_fragments_needed[i]) {
                    is_valid_fragment = 1;
                    break;
                }
                j++;
            }
        }
        assert(is_valid_fragment == 1);
        i++;
    }

    free(fragments_to_reconstruct);
    free(fragments_to_exclude);
    free(fragments_needed);
    free(new_fragments_needed);
}

static void test_decode_with_missing_data(const char *backend,
                                          struct ec_args *args)
{
    int i;
    int *skip = create_skips_array(args, -1);
    assert(skip != NULL);
    for (i = 0; i < args->k; i++)
    {
        skip[i] = 1;
        encode_decode_test_impl(backend, args, skip);
        skip[i] = 0;
    }
    free(skip);
}

static void test_decode_with_missing_parity(const char *backend,
                                           struct ec_args *args)
{
    int i;
    int *skip = create_skips_array(args,args->k);
    assert(skip != NULL);
    for (i = args->k; i < args->m; i++)
    {
        skip[i] = 1;
        encode_decode_test_impl(backend, args, skip);
        skip[i] = 0;
    }
    free(skip);
}

static void test_decode_with_missing_multi_data(const char *backend,
                                               struct ec_args *args)
{
    int max_num_missing = args->hd - 1;
    int i,j;
    for (i = 0; i < args->k - max_num_missing + 1; i++) {
        int *skip = create_skips_array(args,-1);
        assert(skip != NULL);
        for (j = i; j < i + max_num_missing; j++) {
            skip[j]=1;
        }
        encode_decode_test_impl(backend, args, skip);
        free(skip);
    }
}

static void test_decode_with_missing_multi_parity(const char *backend,
                                                 struct ec_args *args)
{
    int i,j;
    int max_num_missing = args->hd - 1;
    for (i = args->k; i < args->k + args->m - max_num_missing + 1; i++) {
        int *skip = create_skips_array(args,-1);
        assert(skip != NULL);
        for (j = i; j < i + max_num_missing; j++) {
            skip[j]=1;
        }
        encode_decode_test_impl(backend, args, skip);
        free(skip);
    }
}

static void test_decode_with_missing_multi_data_parity(const char *backend,
                                                       struct ec_args *args)
{
    int i,j;
    int max_num_missing = args->hd - 1;
    int start = args->k - max_num_missing + 1;
    for (i = start; i < start + max_num_missing -1; i++) {
        int *skip = create_skips_array(args,-1);
        assert(skip != NULL);
        for (j = i; j < i + max_num_missing; j++) {
            skip[j]=1;
        }
        encode_decode_test_impl(backend, args, skip);
        free(skip);
    }
}

static void test_simple_encode_decode(const char *backend,
                                     struct ec_args *args)
{
    int *skip = create_skips_array(args,-1);
    assert(skip != NULL);
    encode_decode_test_impl(backend, args, skip);
    free(skip);
}

static void test_simple_reconstruct(const char *backend,
                                     struct ec_args *args)
{
    int i = 0;
    for (i = 0; i < args->k + args->m; i++) {
        int *skip = create_skips_array(args,i);
        assert(skip != NULL);
        reconstruct_test_impl(backend, args, skip);
        free(skip);
    }
}

static void test_fragments_needed(const char *backend,
                                  struct ec_args *args)
{
    test_fragments_needed_impl(backend, args);
}

struct ec_args null_args = {
    .k = 8,
    .m = 4,
    .priv_args1.null_args.arg1 = 11,
};

struct ec_args flat_xor_hd_args = {
    .k = 10,
    .m = 6,
    .hd = 4,
};

struct ec_args jerasure_rs_vand_args = {
    .k = 10,
    .m = 4,
    .w = 16,
    .hd = 5,
};

struct ec_args jerasure_rs_cauchy_args = {
    .k = 10,
    .m = 4,
    .w = 4,
    .hd = 5,
};

struct testcase testcases[] = {
    {"liberasurecode_supported_backends",
        test_liberasurecode_supported_backends,
        NULL, NULL,
        .skip = false},
    {"test_liberasurecode_supported_checksum_types",
        test_liberasurecode_supported_checksum_types,
        NULL, NULL,
        .skip = false},
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        "null", &null_args,
        .skip = false},
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    // NULL backend tests
    {"simple_encode_null",
        test_simple_encode_decode,
        "null", &null_args,
        .skip = false},
    // Flat XOR backend tests
    {"simple_encode_flat_xor_hd",
        test_simple_encode_decode,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"decode_with_missing_data_flat_xor_hd",
        test_decode_with_missing_data,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"decode_with_missing_parity_flat_xor_hd",
        test_decode_with_missing_parity,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"decode_with_missing_multi_data_flat_xor_hd",
        test_decode_with_missing_multi_data,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"decode_with_missing_multi_parity_flat_xor_hd",
        test_decode_with_missing_multi_parity,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"test_decode_with_missing_multi_data_parity_flat_xor_hd",
        test_decode_with_missing_multi_data_parity,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"simple_reconstruct_flat_xor_hd",
        test_simple_reconstruct,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"test_fragments_needed_flat_xor_hd",
        test_fragments_needed, 
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    // Jerasure RS Vand backend tests
    {"simple_encode_jerasure_rs_vand",
        test_simple_encode_decode,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"decode_with_missing_data_jerasure_rs_vand",
        test_decode_with_missing_data,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"decode_with_missing_multi_data_jerasure_rs_vand",
        test_decode_with_missing_multi_data,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"decode_with_missing_multi_parity_jerasure_rs_vand",
        test_decode_with_missing_multi_parity,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"test_decode_with_missing_multi_data_parity_jerasure_rs_vand",
        test_decode_with_missing_multi_data_parity,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"simple_reconstruct_jerasure_rs_vand",
        test_simple_reconstruct,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"test_fragments_needed_jerasure_rs_vand",
        test_fragments_needed, 
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    // Jerasure RS Cauchy backend tests
    {"simple_encode_jerasure_rs_cauchy",
        test_simple_encode_decode,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    {"decode_with_missing_data_jerasure_rs_cauchy",
        test_decode_with_missing_data,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    {"decode_with_missing_multi_data_jerasure_rs_cauchy",
        test_decode_with_missing_multi_data,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    {"decode_with_missing_multi_parity_jerasure_rs_cauchy",
        test_decode_with_missing_multi_parity,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    {"decode_with_missing_multi_data_parity_jerasure_rs_cauchy",
        test_decode_with_missing_multi_data_parity,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    {"simple_reconstruct_jerasure_rs_cauchy",
        test_simple_reconstruct,
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    {"test_fragments_needed_jerasure_rs_cauchy",
        test_fragments_needed, 
        "jerasure_rs_cauchy", &jerasure_rs_cauchy_args,
        .skip = false},
    { NULL, NULL, NULL, NULL, false },
};

int main(int argc, char **argv)
{
    int ii = 0, num_cases = 0;

    for (num_cases = 0; testcases[num_cases].description; num_cases++) {
        /* Just counting */
    }

    printf("1..%d\n", num_cases);

    for (ii = 0; testcases[ii].description != NULL; ++ii) {
        const char *testname = (const char *) testcases[ii].arg1;
        fflush(stdout);
        if (testcases[ii].skip) {
            fprintf(stdout, "ok # SKIP %d - %s: %s\n", ii + 1,
                    testcases[ii].description,
                    (testname) ? testname : "");
            continue;
        }
        testcases[ii].function(testcases[ii].arg1, testcases[ii].arg2);
        fprintf(stdout, "ok %d - %s: %s\n", ii + 1,
                testcases[ii].description,
                (testname) ? testname : "");
        fflush(stdout);
    }
    return 0;
}
