/* 
 * Copyright 2014 Eric Lambert, Tushar Gohad, Kevin Greenan
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
#include "alg_sig.h"
#define NULL_BACKEND "null"
#define FLAT_XOR_HD_BACKEND "flat_xor_hd"
#define JERASURE_RS_VAND_BACKEND "jerasure_rs_vand"
#define JERASURE_RS_CAUCHY_BACKEND "jerasure_rs_cauchy"

typedef void (*TEST_FUNC)();

struct testcase {
    const char *description;
    TEST_FUNC function;
    ec_backend_id_t be_id;
    ec_checksum_type_t ct;
    bool skip;
};

struct ec_args null_args = {
    .k = 8,
    .m = 4,
    .priv_args1.null_args.arg1 = 11,
    .ct = CHKSUM_NONE,
};

struct ec_args flat_xor_hd_args = {
    .k = 10,
    .m = 6,
    .hd = 4,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_vand_args = {
    .k = 10,
    .m = 4,
    .w = 16,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_cauchy_args = {
    .k = 10,
    .m = 4,
    .w = 4,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

char * get_name_from_backend_id(ec_backend_id_t be) {
    switch(be) {
        case EC_BACKEND_NULL:
            return NULL_BACKEND;
        case EC_BACKEND_JERASURE_RS_VAND:
            return JERASURE_RS_VAND_BACKEND;
        case EC_BACKEND_JERASURE_RS_CAUCHY:
            return JERASURE_RS_CAUCHY_BACKEND;
        case EC_BACKEND_FLAT_XOR_HD:
            return FLAT_XOR_HD_BACKEND;
        default:
            return "UNKNOWN";
    }
}

struct ec_args *create_ec_args(ec_backend_id_t be, ec_checksum_type_t ct)
{
    size_t ec_args_size = sizeof(struct ec_args);
    struct ec_args *template = NULL;
    switch(be) {
        case EC_BACKEND_NULL:
            template = &null_args;
            break;
        case EC_BACKEND_JERASURE_RS_VAND:
            template = &jerasure_rs_vand_args;
            break;
        case EC_BACKEND_JERASURE_RS_CAUCHY:
            template = &jerasure_rs_cauchy_args;
            break;
        case EC_BACKEND_FLAT_XOR_HD:
            template = &flat_xor_hd_args;
            break;
        default:
            return NULL;
    }
    struct ec_args *args = malloc(ec_args_size);
    assert(args);
    memcpy(args, template, ec_args_size);
    args->ct = ct;
    return args;
}

//TODO Make this a bit more useful
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

static void validate_fragment_checksum(struct ec_args *args,
    fragment_metadata_t *metadata, char *fragment_data)
{
    uint32_t chksum = metadata->chksum[0];
    uint32_t computed = 0;
    uint32_t size = metadata->size;
    switch (args->ct) {
        case CHKSUM_MD5:
            assert(false); //currently only have support crc32
            break;
        case CHKSUM_CRC32:
            computed = crc32(0, fragment_data, size);
            break;
        case CHKSUM_NONE:
            assert(metadata->chksum_mismatch == 0);
            break;
        default:
            assert(false);
            break;
    }
    if (metadata->chksum_mismatch) {
        assert(chksum != computed);
    } else {
        assert(chksum == computed);
    }
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

static void test_create_backend_invalid_args()
{
    assert(liberasurecode_instance_create(NULL, &null_args) < 0);
    assert(liberasurecode_instance_create("DOESNOTEXIST", &null_args) == -EBACKENDNOTSUPP);
    assert(liberasurecode_instance_create("", &null_args)  == -EBACKENDNOTSUPP);
    assert(liberasurecode_instance_create(" ", &null_args)  == -EBACKENDNOTSUPP);
    assert(liberasurecode_instance_create("null", NULL) < 0);
}

static void test_destroy_backend_invalid_args()
{
    int desc = -1;
    assert(liberasurecode_instance_destroy(desc) < 0);
    desc = 1;
    assert(liberasurecode_instance_destroy(desc) < 0);
    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);
    assert(0 == liberasurecode_instance_destroy(desc));
    assert(liberasurecode_instance_destroy(desc) < 0);
}

static void test_encode_invalid_args()
{
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;

    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc < 0);

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);

    rc = liberasurecode_encode(desc, NULL, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc < 0);

    rc = liberasurecode_encode(desc, orig_data, 0,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc < 0);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            NULL, &encoded_parity, &encoded_fragment_len);
    assert(rc < 0);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, NULL, &encoded_fragment_len);
    assert(rc < 0);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, NULL);
    assert(rc < 0);
}

static void test_encode_cleanup_invalid_args()
{
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    rc = liberasurecode_encode_cleanup(-1, encoded_data, encoded_parity);
    assert(rc < 0);

    rc = liberasurecode_encode_cleanup(desc, NULL, NULL);
    assert(rc == 0);
}

static void test_decode_invalid_args()
{
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int num_avail_frags = -1;
    char **avail_frags = NULL;
    int *skips = create_skips_array(&null_args, -1);
    char *decoded_data = NULL;
    uint64_t decoded_data_len = 0;

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &null_args, skips);

    rc = liberasurecode_decode(-1, avail_frags, num_avail_frags,
                               encoded_fragment_len, &decoded_data,
                               &decoded_data_len);
    assert(rc < 0);

    rc = liberasurecode_decode(desc, NULL, num_avail_frags,
                               encoded_fragment_len, &decoded_data,
                               &decoded_data_len);
    assert(rc < 0);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, NULL,
                               &decoded_data_len);
    assert(rc < 0);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, &decoded_data,
                               NULL);
    assert(rc < 0);
}

static void test_decode_cleanup_invalid_args()
{
    int rc = 0;
    int desc = 1;
    char *orig_data = create_buffer(1024, 'x');

    rc = liberasurecode_decode_cleanup(desc, orig_data);
    assert(rc < 0);

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);

    rc = liberasurecode_decode_cleanup(desc, NULL);
    assert(rc == 0);
}

static void test_reconstruct_fragment_invalid_args()
{
    int rc = -1;
    int desc = 1;
    int frag_len = 10;
    char **avail_frags = malloc(sizeof(char *) * 2);
    char *out_frag = malloc(frag_len);

    assert(avail_frags);
    assert(out_frag);

    rc = liberasurecode_reconstruct_fragment(desc, avail_frags, 1, frag_len, 1, out_frag);
    assert(rc < 0);

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);

    rc = liberasurecode_reconstruct_fragment(desc, NULL, 1, frag_len, 1, out_frag);
    assert(rc < 0);

    rc = liberasurecode_reconstruct_fragment(desc, avail_frags, 1, frag_len, 1, NULL);
    assert(rc < 0);
}

static void test_fragments_needed_invalid_args()
{
    int rc = -1;
    int desc = 1;
    int frags_to_recon = -1;
    int frags_to_exclude = -1;
    int *frags_needed= NULL;

    rc = liberasurecode_fragments_needed(desc, &frags_to_recon, &frags_to_exclude, frags_needed);
    assert(rc < 0);

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);

    rc = liberasurecode_fragments_needed(desc, NULL, &frags_to_exclude, frags_needed);
    assert(rc < 0);

    rc = liberasurecode_fragments_needed(desc, &frags_to_recon, NULL, frags_needed);
    assert(rc < 0);

    rc = liberasurecode_fragments_needed(desc, &frags_to_recon, &frags_to_exclude, NULL);
    assert(rc < 0);
}

static void test_get_fragment_metadata_invalid_args() {
    int rc = -1;
    char *frag = malloc(1024);
    fragment_metadata_t metadata;

    assert(frag);
    memset(frag, 0, 1024);
    fragment_header_t *fragment_hdr = (fragment_header_t *)frag;
    fragment_hdr->magic = LIBERASURECODE_FRAG_HEADER_MAGIC;


    rc = liberasurecode_get_fragment_metadata(NULL, &metadata);
    assert(rc < 0);

    rc = liberasurecode_get_fragment_metadata(frag, NULL);
    assert(rc < 0);

    memset(frag, 0, 1024); //clears magic
    rc = liberasurecode_get_fragment_metadata(frag, &metadata);
    assert(rc < 0);
}

static void test_verify_stripe_metadata_invalid_args() {
    int rc = -1;
    int num_frags = 6;
    int desc = -1;
    char **frags = malloc(sizeof(char *) * num_frags);

    rc = liberasurecode_verify_stripe_metadata(desc, frags, num_frags);
    assert(rc < 0);

    desc = liberasurecode_instance_create("null", &null_args);
    assert(desc > 0);

    rc = liberasurecode_verify_stripe_metadata(desc, NULL, num_frags);
    assert(rc < 0);

    rc = liberasurecode_verify_stripe_metadata(desc, frags, -1);
    assert(rc < 0);
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
        char *data_ptr = frag + frag_header_size;
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

    rc = liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    assert(rc == 0);

    rc = liberasurecode_decode_cleanup(desc, decoded_data);
    assert(rc == 0);

    if (desc) {
        assert(0 == liberasurecode_instance_destroy(desc));
    }

    free(orig_data);
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
    fragments_to_reconstruct = (int*)malloc(sizeof(int) * n);
    assert(fragments_to_reconstruct != NULL);
    fragments_to_exclude = (int*)malloc(sizeof(int) * n);
    assert(fragments_to_exclude != NULL);
    fragments_needed = (int*)malloc(sizeof(int) * n);
    assert(fragments_needed != NULL);
    new_fragments_needed = (int*)malloc(sizeof(int) * n);
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

static void test_get_fragment_metadata(char *backend, struct ec_args *args)
{
    int i = 0;
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = NULL;
    char **encoded_data = NULL, **encoded_parity = NULL;
    int num_fragments = args-> k + args->m;
    uint64_t encoded_fragment_len = 0;
    fragment_metadata_t cur_frag;
    fragment_metadata_t cmp_frag;

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

    memset(&cmp_frag, -1, sizeof(fragment_metadata_t));

    for (i = 0; i < num_fragments; i++) {
        char * data = NULL;
        memset(&cur_frag, -1, sizeof(fragment_metadata_t));
        if (i < args->k) {
            rc = liberasurecode_get_fragment_metadata(encoded_data[i], &cur_frag);
            data = get_data_ptr_from_fragment(encoded_data[i]);
        } else {
            rc = liberasurecode_get_fragment_metadata(encoded_parity[i - args->k], &cur_frag);
            data = get_data_ptr_from_fragment(encoded_parity[i - args->k]);
        }
        assert(rc == 0);
        assert(cur_frag.orig_data_size == orig_data_size);
        assert(cur_frag.size != 0);
        assert(cur_frag.chksum_type == args->ct);
        validate_fragment_checksum(args, &cur_frag, data);
        rc = memcmp(&cur_frag, &cmp_frag, sizeof(fragment_metadata_t));
        assert(rc != 0);
    }
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

static void test_backend_lookup_by_name()
{
    const char *null_name = NULL;
    const char *empty_name = "";
    const char *white_space_name = " ";
    const char *missing_name = "1DONT3XIST!";
    int i =0, num_backends;
    const char **supported_ec_backends =
        liberasurecode_supported_backends(&num_backends);

    assert(liberasurecode_backend_lookup_by_name(null_name) == NULL);
    assert(liberasurecode_backend_lookup_by_name(empty_name) == NULL);
    assert(liberasurecode_backend_lookup_by_name(white_space_name) == NULL);
    assert(liberasurecode_backend_lookup_by_name(missing_name) == NULL);

    for (i = 0; i < num_backends; i++) {
        assert(liberasurecode_backend_lookup_by_name(supported_ec_backends[i]) != NULL);
    }
}

static void test_backend_lookup_by_id()
{
    const char *null_name = NULL;
    const char *empty_name = "";
    const char *white_space_name = " ";
    const char *missing_name = "1DONT3XIST!";
    int i =0, num_backends;
    const char **supported_ec_backends =
        liberasurecode_supported_backends(&num_backends);
    assert(liberasurecode_backend_lookup_id(null_name) == -1);
    assert(liberasurecode_backend_lookup_id(empty_name) == -1);
    assert(liberasurecode_backend_lookup_id(white_space_name) == -1);
    assert(liberasurecode_backend_lookup_id(missing_name) == -1);
    for (i = 0; i < num_backends; i++) {
        assert(liberasurecode_backend_lookup_id(supported_ec_backends[i]) == i);
    }
}

struct testcase testcases[] = {
    {"liberasurecode_supported_backends",
        test_liberasurecode_supported_backends,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_liberasurecode_supported_checksum_types",
        test_liberasurecode_supported_checksum_types,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"look_up_by_name",
        test_backend_lookup_by_name,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"look_up_by_id",
        test_backend_lookup_by_id,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_create_backend_invalid_args",
        test_create_backend_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_create_backend_invalid_args",
        test_destroy_backend_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_encode_invalid_args",
        test_encode_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_encode_cleanup_invalid_args",
        test_encode_cleanup_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_decode_invalid_args",
        test_decode_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_decode_cleanup_invalid_args",
        test_decode_cleanup_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_reconstruct_fragment_invalid_args",
        test_reconstruct_fragment_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_get_fragment_metadata_invalid_args",
        test_get_fragment_metadata_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    {"test_verify_stripe_metadata_invalid_args",
        test_verify_stripe_metadata_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = true}, //EDL, liberasurecode_verify_stripe_metadata is not implemented at the moment
    {"test_fragments_needed_invalid_args",
        test_fragments_needed_invalid_args,
        EC_BACKENDS_MAX, CHKSUM_TYPES_MAX,
        .skip = false},
    // NULL backend test
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        EC_BACKEND_NULL, CHKSUM_NONE,
        .skip = false},
    {"simple_encode_null",
        test_simple_encode_decode,
        EC_BACKEND_NULL, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata",
        test_get_fragment_metadata,
        EC_BACKEND_NULL, CHKSUM_NONE,
        .skip = false},
    // Flat XOR backend tests
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"simple_encode_flat_xor_hd",
        test_simple_encode_decode,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_data_flat_xor_hd",
        test_decode_with_missing_data,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_parity_flat_xor_hd",
        test_decode_with_missing_parity,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_data_flat_xor_hd",
        test_decode_with_missing_multi_data,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_parity_flat_xor_hd",
        test_decode_with_missing_multi_parity,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"test_decode_with_missing_multi_data_parity_flat_xor_hd",
        test_decode_with_missing_multi_data_parity,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"simple_reconstruct_flat_xor_hd",
        test_simple_reconstruct,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"test_fragments_needed_flat_xor_hd",
        test_fragments_needed, 
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata_flat_xor_hd",
        test_get_fragment_metadata,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata_flat_xor_hd_crc32",
        test_get_fragment_metadata,
        EC_BACKEND_FLAT_XOR_HD, CHKSUM_CRC32,
        .skip = true},
    // Jerasure RS Vand backend tests
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"simple_encode_jerasure_rs_vand",
        test_simple_encode_decode,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_data_jerasure_rs_vand",
        test_decode_with_missing_data,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_data_jerasure_rs_vand",
        test_decode_with_missing_multi_data,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_parity_jerasure_rs_vand",
        test_decode_with_missing_multi_parity,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"test_decode_with_missing_multi_data_parity_jerasure_rs_vand",
        test_decode_with_missing_multi_data_parity,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"simple_reconstruct_jerasure_rs_vand",
        test_simple_reconstruct,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"test_fragments_needed_jerasure_rs_vand",
        test_fragments_needed, 
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata_jerasure_rs_vand",
        test_get_fragment_metadata,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata_jerasure_rs_vand_crc32",
        test_get_fragment_metadata,
        EC_BACKEND_JERASURE_RS_VAND, CHKSUM_CRC32,
        .skip = true},
    // Jerasure RS Cauchy backend tests
    {"create_and_destroy_backend",
        test_create_and_destroy_backend,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"simple_encode_jerasure_rs_cauchy",
        test_simple_encode_decode,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_data_jerasure_rs_cauchy",
        test_decode_with_missing_data,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_data_jerasure_rs_cauchy",
        test_decode_with_missing_multi_data,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_parity_jerasure_rs_cauchy",
        test_decode_with_missing_multi_parity,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"decode_with_missing_multi_data_parity_jerasure_rs_cauchy",
        test_decode_with_missing_multi_data_parity,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"simple_reconstruct_jerasure_rs_cauchy",
        test_simple_reconstruct,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"test_fragments_needed_jerasure_rs_cauchy",
        test_fragments_needed, 
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata_jerasure_rs_cauchy",
        test_get_fragment_metadata,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_NONE,
        .skip = false},
    {"test_get_fragment_metadata_jerasure_rs_cauchy",
        test_get_fragment_metadata,
        EC_BACKEND_JERASURE_RS_CAUCHY, CHKSUM_CRC32,
        .skip = true},
    { NULL, NULL, 0, 0, false },
};

int main(int argc, char **argv)
{
    int ii = 0, num_cases = 0;

    for (num_cases = 0; testcases[num_cases].description; num_cases++) {
        /* Just counting */
    }

    printf("1..%d\n", num_cases);

    for (ii = 0; testcases[ii].description != NULL; ++ii) {
        const char *testname = get_name_from_backend_id(testcases[ii].be_id);
        fflush(stdout);
        if (testcases[ii].skip) {
            fprintf(stdout, "ok # SKIP %d - %s: %s\n", ii + 1,
                    testcases[ii].description,
                    (testname) ? testname : "");
            continue;
        }
        struct ec_args *args = create_ec_args(testcases[ii].be_id, testcases[ii].ct);
        testcases[ii].function(testname, args);
        fprintf(stdout, "ok %d - %s: %s\n", ii + 1,
                testcases[ii].description,
                (testname) ? testname : "");
        fflush(stdout);
        free(args);
    }
    return 0;
}
