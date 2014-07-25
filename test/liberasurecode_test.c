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

typedef int (*TEST_FUNC)(const char *, struct ec_args *);

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

static int create_frags_array(char ***array, 
                              char **data,
                              int data_size,
                              char **parity,
                              int parity_size)
{
    int num_frags = 0;
    *array = malloc((data_size + parity_size) * sizeof(char *));
    if (array == NULL) {
        num_frags = -1;
        goto out;
    }
    //add data frags
    int i = 0;
    char **ptr = *array;
    for (i = 0; i < data_size; i++)
    {
        *ptr++ = data[i]; 
        num_frags++;
    }
    //add parity frags
    for (i = 0; i < parity_size; i++)
    {
        *ptr++ = parity[i]; 
        num_frags++;
    }
out:
    return num_frags;
}

static int test_liberasurecode_supported_backends(
        const char *backend,
        struct ec_args *args)
{
    //EDL skipping for now since this function is not implemented.
}

static int test_create_and_destroy_backend(
        const char *backend,
        struct ec_args *args)
{
    int desc = liberasurecode_instance_create(backend, args);
    assert(desc > 0 || -EBACKENDNOTAVAIL == desc);
    if (desc)
        liberasurecode_instance_destroy(desc);
    return 0;
}

static int test_simple_encode_decode(
        const char *backend,
        struct ec_args *args)
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
        
    desc = liberasurecode_instance_create(backend, args);
    assert(desc > 0 || -EBACKENDNOTAVAIL == desc);

    if (-EBACKENDNOTAVAIL == desc) {
        fprintf (stderr, "Backend library not available! ");
        return 0;
    }

    orig_data = create_buffer(orig_data_size, 'x');
    if (NULL == orig_data) {
        rc = -ENOMEM;
        goto out;
    }

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);

    char **avail_frags = NULL;
    int num_avail_frags = create_frags_array(&avail_frags,
                          encoded_data, args->k, encoded_parity, args->m);
    if (num_avail_frags == -1) {
        rc = -ENOMEM;
        goto out;
    }

    rc = liberasurecode_decode(desc, avail_frags,
            num_avail_frags, encoded_fragment_len, &decoded_data, &decoded_data_len);
    assert(0 == rc);
    assert(decoded_data_len == orig_data_size);
    assert(memcmp(decoded_data, orig_data, orig_data_size) == 0);

    if (desc)
        liberasurecode_instance_destroy(desc);

    free(orig_data);

out:
    if (avail_frags != NULL)
    {
        int idx = 0;
        for (idx = 0; idx < num_fragments; idx++) 
        {
            free(avail_frags[idx]);
        }
        free(avail_frags);
    }
    return rc;
}

struct ec_args null_args = {
    .k = 8,
    .m = 4,
    .priv_args1.null_args.arg1 = 11,
};

struct ec_args flat_xor_hd_args = {
    .k = 10,
    .m = 6,
    .priv_args1.flat_xor_hd_args.hd = 3,
};

struct ec_args jerasure_rs_vand_args = {
    .k = 10,
    .m = 4,
    .w = 16,
};

struct ec_args jerasure_rs_cauchy_args = {
    .k = 10,
    .m = 4,
    .w = 4,
};

struct testcase testcases[] = {
    {"liberasurecode_supported_backends",
        test_liberasurecode_supported_backends,
        NULL, NULL,
        .skip = true},
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
    {"simple_encode_flat_xor_hd",
        test_simple_encode_decode,
        "null", &null_args,
        .skip = true},
    {"simple_encode_flat_xor_hd",
        test_simple_encode_decode,
        "flat_xor_hd", &flat_xor_hd_args,
        .skip = false},
    {"simple_encode_jerasure_rs_vand",
        test_simple_encode_decode,
        "jerasure_rs_vand", &jerasure_rs_vand_args,
        .skip = false},
    {"simple_encode_jerasure_rs_cauchy",
        test_simple_encode_decode,
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
