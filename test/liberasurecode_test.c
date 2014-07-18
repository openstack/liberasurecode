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

typedef int (*TEST_FUNC)(void);
struct testcase {
    const char *description;
    TEST_FUNC function;
    bool skip;
};

//TODO Make this a but more useful
char *create_buffer(int size)
{
    char *buf = malloc(size);
    memset(buf, 'x', size);
    return buf;
}

static int test_liberasurecode_supported_backends(void)
{
    //EDL skipping for now since this function is not implemented.
}

static int test_create_and_destroy_backend(void)
{
    struct ec_args args = {
        .k = 10,
        .m = 4,
    };
    int desc = liberasurecode_instance_create("jerasure_rs_vand", &args);
    assert(desc >= 0);
    assert(liberasurecode_instance_destroy(desc) == 0);
}

static int test_simple_encode_flat_xor_hd(void)
{
    struct ec_args args = {
        .k = 10,
        .m = 4,
        .priv_args1.flat_xor_hd_args.hd = 3,
    };

    int rc = 0;
    int desc = -1;
    int size = 1024 * 1024;
    char **encoded_data = NULL, **parity = NULL;
    char *data = NULL;
        
    data = create_buffer(size);
    if (NULL == data) {
        rc = -ENOMEM;
        goto out;
    }

    desc = liberasurecode_instance_create("flat_xor_hd", &args);
    rc = liberasurecode_encode(desc, data, size, encoded_data, parity);
    assert(rc == 0);
    assert(liberasurecode_instance_destroy(desc) == 0);
    free(data);

out:
    return rc;
}

static int test_simple_encode_jerasure_rs_vand(void)
{
    struct ec_args args = {
        .k = 10,
        .m = 4,
    };

    int rc = 0;
    int desc = -1;
    int size = 1024 * 1024;
    char **encoded_data = NULL, **parity = NULL;
    char *data = NULL;
        
    data = create_buffer(size);
    if (NULL == data) {
        rc = -ENOMEM;
        goto out;
    }

    desc = liberasurecode_instance_create("jerasure_rs_vand", &args);
    rc = liberasurecode_encode(desc, data, size, encoded_data, parity);
    assert(rc == 0);
    assert(liberasurecode_instance_destroy(desc) == 0);
    free(data);

out:
    return rc;
}

struct testcase testcases[] = {
    {"liberasurecode_supported_backends",
        test_liberasurecode_supported_backends, true},
    {"create_and_destroy_backend",
        test_create_and_destroy_backend, false},
    {"simple_encode_flat_xor_hd",
        test_simple_encode_flat_xor_hd, true},
    {"simple_encode_jerasure_rs_vand",
        test_simple_encode_jerasure_rs_vand, false},
    { NULL, NULL, false }
};

int main(int argc, char **argv)
{
    int ii = 0, num_cases = 0;

    for (num_cases = 0; testcases[num_cases].description; num_cases++) {
        /* Just counting */
    }

    printf("1..%d\n", num_cases);

    for (ii = 0; testcases[ii].description != NULL; ++ii) {
        fflush(stdout);
        if (testcases[ii].skip) {
            fprintf(stdout, "ok # SKIP %d - %s\n", ii + 1,
                    testcases[ii].description);
            continue;
        }
        testcases[ii].function();
        fprintf(stdout, "ok %d - %s\n", ii + 1, testcases[ii].description);
        fflush(stdout);
    }

    return 0;
}

