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
#include <zlib.h>
#include "erasurecode.h"
#include "erasurecode_helpers.h"
#include "erasurecode_helpers_ext.h"
#include "erasurecode_preprocessing.h"
#include "erasurecode_backend.h"
#include "alg_sig.h"
#define NULL_BACKEND "null"
#define FLAT_XOR_HD_BACKEND "flat_xor_hd"
#define JERASURE_RS_VAND_BACKEND "jerasure_rs_vand"
#define JERASURE_RS_CAUCHY_BACKEND "jerasure_rs_cauchy"
#define ISA_L_RS_VAND_BACKEND "isa_l_rs_vand"
#define ISA_L_RS_VAND_INV_BACKEND "isa_l_rs_vand_inv"
#define ISA_L_RS_CAUCHY_BACKEND "isa_l_rs_cauchy"
#define SHSS_BACKEND "shss"
#define RS_VAND_BACKEND "liberasurecode_rs_vand"
#define LIBPHAZR_BACKEND "libphazr"

typedef void (*TEST_FUNC_NO_ARGS)(void);
typedef void (*TEST_FUNC_WITH_ARGS)(const ec_backend_id_t, struct ec_args *);

struct testcase {
    const char *description;
    union{
        TEST_FUNC_NO_ARGS no_args;
        TEST_FUNC_WITH_ARGS with_args;
    } function;
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

struct ec_args *null_test_args[] = { &null_args, NULL };

struct ec_args flat_xor_hd_args = {
    .k = 3,
    .m = 3,
    .hd = 3,
    .ct = CHKSUM_NONE,
};

struct ec_args *flat_xor_test_args[] = { &flat_xor_hd_args, NULL };

struct ec_args jerasure_rs_vand_args = {
    .k = 10,
    .m = 4,
    .w = 16,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_vand_44_args = {
    .k = 4,
    .m = 4,
    .w = 16,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_vand_48_args = {
    .k = 4,
    .m = 8,
    .w = 16,
    .hd = 9,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_vand_1010_args = {
    .k = 10,
    .m = 10,
    .w = 16,
    .hd = 11,
    .ct = CHKSUM_NONE,
};

struct ec_args *jerasure_rs_vand_test_args[] = { &jerasure_rs_vand_args,
                                                 &jerasure_rs_vand_44_args,
                                                 &jerasure_rs_vand_1010_args,
                                                 &jerasure_rs_vand_48_args,
                                                 NULL };
struct ec_args jerasure_rs_cauchy_args = {
    .k = 10,
    .m = 4,
    .w = 4,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_cauchy_44_args = {
    .k = 4,
    .m = 4,
    .w = 4,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args jerasure_rs_cauchy_48_args = {
    .k = 4,
    .m = 8,
    .w = 8,
    .hd = 9,
    .ct = CHKSUM_NONE,
};


struct ec_args jerasure_rs_cauchy_1010_args = {
    .k = 10,
    .m = 10,
    .w = 8,
    .hd = 11,
    .ct = CHKSUM_NONE,
};

struct ec_args *jerasure_rs_cauchy_test_args[] = { &jerasure_rs_cauchy_args,
                                                   &jerasure_rs_cauchy_44_args,
                                                   &jerasure_rs_cauchy_48_args,
                                                   &jerasure_rs_cauchy_1010_args,
                                                   NULL };

struct ec_args isa_l_args = {
    .k = 10,
    .m = 4,
    .w = 8,
    .hd = 5,
};

struct ec_args isa_l_44_args = {
    .k = 4,
    .m = 4,
    .w = 8,
    .hd = 5,
};

struct ec_args isa_l_1010_args = {
    .k = 10,
    .m = 10,
    .w = 8,
    .hd = 11,
};

struct ec_args isa_l_75_args = {
    .k = 7,
    .m = 5,
    .w = 8,
    .hd = 6,
};
struct ec_args isa_l_85_args = {
    .k = 8,
    .m = 5,
    .w = 8,
    .hd = 6,
};

struct ec_args *isa_l_test_args[] = { &isa_l_args,
                                      &isa_l_44_args,
                                      &isa_l_1010_args,
                                      &isa_l_75_args,
                                      &isa_l_85_args,
                                      NULL };

int priv = 128;
struct ec_args shss_args = {
    .k = 6,
    .m = 3,
    .hd = 3,
    .priv_args2 = &priv,
};

struct ec_args *shss_test_args[] = { &shss_args, NULL };

struct ec_args liberasurecode_rs_vand_args = {
    .k = 10,
    .m = 4,
    .w = 16,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args liberasurecode_rs_vand_44_args = {
    .k = 4,
    .m = 4,
    .w = 16,
    .hd = 5,
    .ct = CHKSUM_NONE,
};

struct ec_args liberasurecode_rs_vand_48_args = {
    .k = 4,
    .m = 8,
    .w = 16,
    .hd = 9,
    .ct = CHKSUM_NONE,
};

struct ec_args liberasurecode_rs_vand_1010_args = {
    .k = 10,
    .m = 10,
    .w = 16,
    .hd = 11,
    .ct = CHKSUM_NONE,
};

struct ec_args *liberasurecode_rs_vand_test_args[] = {
               &liberasurecode_rs_vand_args,
               &liberasurecode_rs_vand_44_args,
               &liberasurecode_rs_vand_1010_args,
               &liberasurecode_rs_vand_48_args,
               NULL };

struct ec_args libphazr_args = {
    .k = 4,
    .m = 4,
};

struct ec_args *libphazr_test_args[] = { &libphazr_args, NULL };

struct ec_args **all_backend_tests[] = {
               null_test_args,
               flat_xor_test_args,
               jerasure_rs_vand_test_args,
               jerasure_rs_cauchy_test_args,
               isa_l_test_args,
               shss_test_args,
               liberasurecode_rs_vand_test_args,
               libphazr_test_args,
               NULL};

int num_backends(void)
{
  int i = 0;

  while (NULL != all_backend_tests[i]) {
    i++;
  }

  return i;
}

int max_tests_for_backends(void)
{
  int n_backends = num_backends();
  int i = 0;
  int max = 0;

  for (i = 0; i < n_backends; i++) {
    int j = 0;
    while (NULL != all_backend_tests[i][j]) {
      j++;
    }
    if (j > max) {
      max = j;
    }
  }
  return max;
}

typedef enum {
    LIBEC_VERSION_MISMATCH,
    MAGIC_MISMATCH,
    BACKEND_ID_MISMATCH,
    BACKEND_VERSION_MISMATCH,
    FRAGIDX_INVALID,
    FRAGIDX_OUT_OF_RANGE,
} fragment_mismatch_scenario_t;

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
        case EC_BACKEND_ISA_L_RS_VAND:
            return ISA_L_RS_VAND_BACKEND;
        case EC_BACKEND_ISA_L_RS_VAND_INV:
            return ISA_L_RS_VAND_INV_BACKEND;
        case EC_BACKEND_ISA_L_RS_CAUCHY:
            return ISA_L_RS_CAUCHY_BACKEND;
        case EC_BACKEND_SHSS:
            return SHSS_BACKEND;
        case EC_BACKEND_LIBERASURECODE_RS_VAND:
            return RS_VAND_BACKEND;
        case EC_BACKEND_LIBPHAZR:
            return LIBPHAZR_BACKEND;
        default:
            return "UNKNOWN";
    }
}

struct ec_args *create_ec_args(ec_backend_id_t be, ec_checksum_type_t ct, int backend_test_idx)
{
    size_t ec_args_size = sizeof(struct ec_args);
    struct ec_args *template = NULL;
    struct ec_args** backend_args_array = NULL;
    int i = 0;

    switch(be) {
        case EC_BACKEND_NULL:
            backend_args_array = null_test_args;
            break;
        case EC_BACKEND_JERASURE_RS_VAND:
            backend_args_array = jerasure_rs_vand_test_args;
            break;
        case EC_BACKEND_JERASURE_RS_CAUCHY:
            backend_args_array = jerasure_rs_cauchy_test_args;
            break;
        case EC_BACKEND_LIBERASURECODE_RS_VAND:
            backend_args_array = liberasurecode_rs_vand_test_args;
            break;
        case EC_BACKEND_FLAT_XOR_HD:
            backend_args_array = flat_xor_test_args;
            break;
        case EC_BACKEND_ISA_L_RS_VAND:
            backend_args_array = isa_l_test_args;
            break;
        case EC_BACKEND_ISA_L_RS_VAND_INV:
            backend_args_array = isa_l_test_args;
            break;
        case EC_BACKEND_ISA_L_RS_CAUCHY:
            backend_args_array = isa_l_test_args;
            break;
        case EC_BACKEND_SHSS:
            backend_args_array = shss_test_args;
            break;
        case EC_BACKEND_LIBPHAZR:
            backend_args_array = libphazr_test_args;
            break;
        default:
            return NULL;
    }

    while (NULL != backend_args_array && NULL != backend_args_array[i]) {
        if (i == backend_test_idx) {
            template = backend_args_array[i];
            break;
        }
        i++;
    }

    if (NULL == template) {
      return NULL;
    }

    struct ec_args *args = malloc(ec_args_size);
    assert(args);
    memcpy(args, template, ec_args_size);
    args->ct = ct;
    return args;
}

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

inline static int create_fake_frags_no_meta(char ***array, int num_frags,
                                            const char data, int data_len)
{
    // N.B. The difference from creat_frags_arry is to creat new
    // memory allocation and set a copy of data/parity there. The
    // allocated memory should be maintained by the caller.
    int _num_frags = 0;
    int i = 0;
    char **ptr = NULL;

    *array = malloc(num_frags * sizeof(char *));
    if (array == NULL) {
        _num_frags = -1;
        goto out;
    }

    // add data and parity frags
    ptr = *array;
    for (i = 0; i < num_frags; i++) {
        *ptr++ = create_buffer(data_len, data);
        _num_frags++;
    }

out:
    return _num_frags;
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

static void cleanup_avail_frags(char **avail_frags,
                                int num_frags)
{
   while(num_frags > 0) free(avail_frags[--num_frags]);
   free(avail_frags);
}
static int encode_failure_stub(void *desc, char **data,
                               char **parity, int blocksize)
{
    return -1;
}

static void validate_fragment_checksum(struct ec_args *args,
    fragment_metadata_t *metadata, char *fragment_data)
{
    uint32_t chksum = metadata->chksum[0];
    uint32_t computed = 0;
    uint32_t size = metadata->size;
    char *flag;
    switch (args->ct) {
        case CHKSUM_MD5:
            assert(false); //currently only have support crc32
            break;
        case CHKSUM_CRC32:
            flag = getenv("LIBERASURECODE_WRITE_LEGACY_CRC");
            if (flag && !(flag[0] == '\0' || (flag[0] == '0' && flag[1] == '\0'))) {
                computed = liberasurecode_crc32_alt(0, fragment_data, size);
            } else {
                computed = crc32(0, (unsigned char *) fragment_data, size);
            }
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

static void test_create_and_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);
    assert(0 == liberasurecode_instance_destroy(desc));
}

static void test_create_and_destroy_multiple_backends(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    int desc1 = liberasurecode_instance_create(be_id, args);
    int desc2;
    if (-EBACKENDNOTAVAIL == desc1) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc1 > 0);
    desc2 = liberasurecode_instance_create(be_id, args);
    assert(desc2 > 0);
    assert(desc1 != desc2);
    assert(0 == liberasurecode_instance_destroy(desc1));
    /* TODO: check that we can still use desc2 */
    assert(0 == liberasurecode_instance_destroy(desc2));
}

static void test_backend_available(void) {
    assert(1 == liberasurecode_backend_available(EC_BACKEND_NULL));
}

static void test_backend_available_invalid_args(void)
{
    int ret = liberasurecode_backend_available(EC_BACKENDS_MAX);
    // returns 1 if a backend is available; 0 otherwise
    assert(0 == ret);
}

static void test_create_backend_invalid_args(void)
{
    int desc = liberasurecode_instance_create(-1, &null_args);
    assert(-EBACKENDNOTSUPP == desc);

    desc = liberasurecode_instance_create(EC_BACKENDS_MAX, &null_args);
    assert(-EBACKENDNOTSUPP == desc);

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, NULL);
    assert(-EINVALIDPARAMS == desc);

    struct ec_args invalid_args = {
        .k = 100,
        .m = 100,
    };
    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &invalid_args);
    assert(-EINVALIDPARAMS == desc);

    invalid_args.k = -1;
    invalid_args.m = 4;
    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &invalid_args);
    assert(-EINVALIDPARAMS == desc);

    invalid_args.k = 10;
    invalid_args.m = -1;
    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &invalid_args);
    assert(-EINVALIDPARAMS == desc);
}

static void test_destroy_backend_invalid_args(void)
{
    int desc = -1;
    assert(liberasurecode_instance_destroy(desc) < 0);
    desc = 1;
    assert(liberasurecode_instance_destroy(desc) < 0);
    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);
    assert(0 == liberasurecode_instance_destroy(desc));
    assert(liberasurecode_instance_destroy(desc) < 0);
}

static void test_encode_invalid_args(void)
{
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    ec_backend_t instance = NULL;
    int (*orig_encode_func)(void *, char **, char **, int);

    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc < 0);

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_encode(desc, NULL, orig_data_size,
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

    instance = liberasurecode_backend_instance_get_by_desc(desc);
    orig_encode_func = instance->common.ops->encode;
    instance->common.ops->encode = encode_failure_stub;
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc < 0);
    instance->common.ops->encode = orig_encode_func;

    liberasurecode_instance_destroy(desc);
    free(orig_data);
}

static void test_encode_cleanup_invalid_args(void)
{
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    rc = liberasurecode_encode_cleanup(-1, encoded_data, encoded_parity);
    assert(rc < 0);

    rc = liberasurecode_encode_cleanup(desc, NULL, NULL);
    assert(rc == 0);

    rc = liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    assert(rc == 0);
    liberasurecode_instance_destroy(desc);
    free(orig_data);
}

static void test_decode_invalid_args(void)
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
    // fake_data len should be bigger than fragment_header_t for
    // the verifications
    int fake_data_len = 1024;

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    // test with invalid fragments (no metadata headers)
    num_avail_frags = create_fake_frags_no_meta(&avail_frags, (null_args.k +
                                                null_args.m),
                                                'y', fake_data_len);
    assert(num_avail_frags > 0);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               fake_data_len, 1,
                               &decoded_data, &decoded_data_len);
    // no metadata headers w/ force_metadata_checks results in EBADHEADER
    assert(rc == -EBADHEADER);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               fake_data_len, 0,
                               &decoded_data, &decoded_data_len);
    // no metadata headers w/o force_metadata_checks also results in EBADHEADER
    assert(rc == -EBADHEADER);

    // encoded_fragment_len is too small
    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               1, 1, &decoded_data, &decoded_data_len);
    assert(rc == -EBADHEADER);

    cleanup_avail_frags(avail_frags, num_avail_frags);

    // test with num_fragments < (k)
    num_avail_frags = create_fake_frags_no_meta(&avail_frags, (null_args.k - 1),
                                                ' ', 1);
    assert(num_avail_frags > 0);
    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               fake_data_len, 1,
                               &decoded_data, &decoded_data_len);
    assert(rc == -EINSUFFFRAGS);

    cleanup_avail_frags(avail_frags, num_avail_frags);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &null_args, skips);
    assert(num_avail_frags > 0);

    rc = liberasurecode_decode(-1, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, &decoded_data_len);
    assert(rc < 0);

    rc = liberasurecode_decode(desc, NULL, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, &decoded_data_len);
    assert(rc < 0);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               NULL, &decoded_data_len);
    assert(rc < 0);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, NULL);
    assert(rc < 0);

    free(skips);
    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
    // N.B. create_frags_array sets pointer reference of either encoded_data
    // or encoded_parity and they are cleaned up via
    // liberasurecode_encode_cleanup
    free(avail_frags);
    free(orig_data);
}

static void test_decode_cleanup_invalid_args(void)
{
    int rc = 0;
    int desc = 1;
    char *orig_data = create_buffer(1024, 'x');

    rc = liberasurecode_decode_cleanup(desc, orig_data);
    assert(rc < 0);

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_decode_cleanup(desc, NULL);
    assert(rc == 0);
    liberasurecode_instance_destroy(desc);
    free(orig_data);
}

static void test_reconstruct_fragment_invalid_args(void)
{
    int rc = -1;
    int desc = 1;
    int frag_len = 10;
    char **avail_frags = malloc(sizeof(char *) * 2);
    char *out_frag = malloc(frag_len);
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;

    assert(avail_frags);
    assert(out_frag);

    rc = liberasurecode_reconstruct_fragment(desc, avail_frags, 1, frag_len, 1, out_frag);
    assert(rc < 0);

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_reconstruct_fragment(desc, NULL, 1, frag_len, 1, out_frag);
    assert(rc < 0);

    rc = liberasurecode_reconstruct_fragment(desc, avail_frags, 1, frag_len, 1, NULL);
    assert(rc < 0);

    free(out_frag);
    free(avail_frags);

    // Test for EINSUFFFRAGS
    // we have to call encode to get fragments which have valid header.
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    out_frag = malloc(encoded_fragment_len);

    assert(out_frag != NULL);
    rc = liberasurecode_reconstruct_fragment(desc, encoded_data, 1, encoded_fragment_len, 1, out_frag);

    assert(rc == -EINSUFFFRAGS);
    free(orig_data);
    free(out_frag);
    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
}

static void test_fragments_needed_invalid_args(void)
{
    int rc = -1;
    int desc = 1;
    int frags_to_recon = -1;
    int frags_to_exclude = -1;
    int *frags_needed= NULL;

    rc = liberasurecode_fragments_needed(desc, &frags_to_recon, &frags_to_exclude, frags_needed);
    assert(rc < 0);

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_fragments_needed(desc, NULL, &frags_to_exclude, frags_needed);
    assert(rc < 0);

    rc = liberasurecode_fragments_needed(desc, &frags_to_recon, NULL, frags_needed);
    assert(rc < 0);

    rc = liberasurecode_fragments_needed(desc, &frags_to_recon, &frags_to_exclude, NULL);
    assert(rc < 0);
    liberasurecode_instance_destroy(desc);
}

static void test_get_fragment_metadata_invalid_args(void) {
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
    assert(rc == -EBADHEADER);

    free(frag);
}

static void test_verify_stripe_metadata_invalid_args(void) {
    int rc = -1;
    int num_frags = 6;
    int desc = -1;
    char **frags = malloc(sizeof(char *) * num_frags);

    rc = liberasurecode_verify_stripe_metadata(desc, frags, num_frags);
    assert(rc == -EINVALIDPARAMS);

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_verify_stripe_metadata(desc, NULL, num_frags);
    assert(rc == -EINVALIDPARAMS);

    rc = liberasurecode_verify_stripe_metadata(desc, frags, -1);
    assert(rc == -EINVALIDPARAMS);

    rc = liberasurecode_verify_stripe_metadata(desc, frags, 0);
    assert(rc == -EINVALIDPARAMS);
    liberasurecode_instance_destroy(desc);
    free(frags);
}

static void test_get_fragment_partition(void)
{
    int i;
    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int num_avail_frags = -1;
    char **avail_frags = NULL;
    int *skips = create_skips_array(&null_args, -1);
    int *missing;

    desc = liberasurecode_instance_create(EC_BACKEND_NULL, &null_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        free(orig_data);
        free(skips);
        return;
    }
    assert(desc > 0);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);

    missing = alloc_and_set_buffer(sizeof(char*) * null_args.k, -1);

    for(i = 0; i < null_args.m; i++) {
        skips[i] = 1;
        /* get_fragment_partition is going to NULL all the entries in
         * encoded_data and encoded_parity before populating them again
         * from avail_frags. Since we're explicitly *not* including these
         * data frags in avail_frags, free them now.
         */
        if (i < null_args.k) {
            free(encoded_data[i]);
        } else {
            free(encoded_parity[i - null_args.k]);
        }
    }
    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &null_args, skips);

    rc = get_fragment_partition(null_args.k, null_args.m, avail_frags, num_avail_frags,
                                encoded_data, encoded_parity, missing);
    assert(0 == rc);

    for(i = 0; i < null_args.m; i++) assert(missing[i] == i);
    // Loop already pushed us one past
    assert(missing[i] == -1);

    free(avail_frags);
    free(missing);

    missing = alloc_and_set_buffer(sizeof(char*) * null_args.k, -1);
    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &null_args, skips);

    /* Passing k+m frags for a k+(m-1) policy, we should notice the
     * too-high frag index and call its header "bad".
     */
    rc = get_fragment_partition(null_args.k, null_args.m - 1, avail_frags, num_avail_frags,
                                encoded_data, encoded_parity, missing);
    assert(-EBADHEADER == rc);

    for(i = 0; i < null_args.m; i++) assert(missing[i] == -1);
    // Loop already pushed us one past
    assert(missing[i] == -1);

    free(avail_frags);
    free(missing);

    skips[i] = 1;
    if (i < null_args.k) {
        free(encoded_data[i]);
    } else {
        free(encoded_parity[i - null_args.k]);
    }

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &null_args, skips);

    missing = alloc_and_set_buffer(sizeof(char*) * null_args.k, -1);
    rc = get_fragment_partition(null_args.k, null_args.m, avail_frags, num_avail_frags,
                                encoded_data, encoded_parity, missing);

    for(i = 0; i < null_args.m + 1; i++) assert(missing[i] == i);
    assert(missing[++i] == -1);

    assert(rc < 0);

    free(missing);
    free(skips);
    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
    free(avail_frags);
    free(orig_data);
}

static void test_liberasurecode_get_version(void){
    uint32_t version = liberasurecode_get_version();
    assert(version == LIBERASURECODE_VERSION);
}

static void encode_decode_test_impl(const ec_backend_id_t be_id,
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

    desc = liberasurecode_instance_create(be_id, args);

    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    } else if ((args->k + args->m) > EC_MAX_FRAGMENTS) {
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

/**
 * Note: this test will attempt to reconstruct a single fragment when
 * one or more other fragments are missing (specified by skip).
 *
 * For example, if skip is [0, 0, 0, 1, 0, 0] and we are reconstructing
 * fragment 5, then it will test the reconstruction of fragment 5 when 3
 * and 5 are assumed unavailable.
 *
 * We only mark at most 2 as unavailable, as we cannot guarantee every situation
 * will be able to habndle 3 failures.
 */
static void reconstruct_test_impl(const ec_backend_id_t be_id,
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

    desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    orig_data = create_buffer(orig_data_size, 'x');
    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);
    out = malloc(encoded_fragment_len);
    assert(out != NULL);
    for (i = 0; i < num_fragments; i++) {
        char *cmp = NULL;
        // If the current fragment was not chosen as fragments to skip,
        // remove it and the chosen fragments to skip from the available list
        // and reset its state
        if (skip[i] == 0) {
            skip[i] = 1;
            num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                             encoded_parity, args, skip);
            skip[i] = 0;
        // Do not reset the skip state if the fragment was chosen as a fragment
        // to skip for this invocation of the test
        } else {
            num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                             encoded_parity, args, skip);
        }
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
        free(avail_frags);
    }
    free(orig_data);
    free(out);
    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
}

static void test_fragments_needed_impl(const ec_backend_id_t be_id,
                                      struct ec_args *args)
{
    int *fragments_to_reconstruct = NULL;
    int *fragments_to_exclude = NULL;
    int *fragments_needed = NULL;
    int *new_fragments_needed = NULL;
    int ret = -1;
    int i = 0, j = 0;
    int n = args->k + args->m;

    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
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
    liberasurecode_instance_destroy(desc);
    free(fragments_to_reconstruct);
    free(fragments_to_exclude);
    free(fragments_needed);
    free(new_fragments_needed);
}

static void test_get_fragment_metadata(const ec_backend_id_t be_id, struct ec_args *args)
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
    ec_backend_id_t rtv_be_id = -1;
    uint32_t be_version = 0;
    ec_backend_t be = NULL;

    desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);
    be = liberasurecode_backend_instance_get_by_desc(desc);
    assert(be != NULL);

    orig_data = create_buffer(orig_data_size, 'x');
    assert(orig_data != NULL);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);

    memset(&cmp_frag, -1, sizeof(fragment_metadata_t));

    for (i = 0; i < num_fragments; i++) {
        char * data = NULL;
        uint32_t ver = 0;
        char *header = NULL;
        memset(&cur_frag, -1, sizeof(fragment_metadata_t));
        if (i < args->k) {
            rc = liberasurecode_get_fragment_metadata(encoded_data[i], &cur_frag);
            data = get_data_ptr_from_fragment(encoded_data[i]);
            header = encoded_data[i];
        } else {
            rc = liberasurecode_get_fragment_metadata(encoded_parity[i - args->k], &cur_frag);
            data = get_data_ptr_from_fragment(encoded_parity[i - args->k]);
            header = encoded_parity[i - args->k];
        }
        assert(rc == 0);
        assert(cur_frag.orig_data_size == orig_data_size);
        assert(cur_frag.size != 0);
        assert(cur_frag.chksum_type == args->ct);
        validate_fragment_checksum(args, &cur_frag, data);
        rc = memcmp(&cur_frag, &cmp_frag, sizeof(fragment_metadata_t));
        assert(rc != 0);
        rc = get_libec_version(header, &ver);
        assert(rc == 0);
        assert(ver == LIBERASURECODE_VERSION);
        rc = get_backend_id(header, &rtv_be_id);
        assert(rc == 0);
        assert(rtv_be_id == be_id);
        rc = get_backend_version(header, &be_version);
        assert(rc == 0);
        assert(be_version == be->common.ec_backend_version);
    }
    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
    free(orig_data);
}

static void test_write_legacy_fragment_metadata(const ec_backend_id_t be_id, struct ec_args *args)
{
    // any value except 0 will write legacy crc
    setenv("LIBERASURECODE_WRITE_LEGACY_CRC", "1", 1);
    test_get_fragment_metadata(be_id, args);
    setenv("LIBERASURECODE_WRITE_LEGACY_CRC", "true", 1);
    test_get_fragment_metadata(be_id, args);
    // if the value is 0 or unset the value,
    // it will write non-legacy crc but it's still safe to write the crc
    setenv("LIBERASURECODE_WRITE_LEGACY_CRC", "0", 1);
    test_get_fragment_metadata(be_id, args);
    // even it's "00", it should be assumed as non-legacy
    setenv("LIBERASURECODE_WRITE_LEGACY_CRC", "00", 1);
    test_get_fragment_metadata(be_id, args);
    unsetenv("LIBERASURECODE_WRITE_LEGACY_CRC");
    test_get_fragment_metadata(be_id, args);
}

static void test_decode_with_missing_data(const ec_backend_id_t be_id,
                                          struct ec_args *args)
{
    int i;
    int *skip = create_skips_array(args, -1);
    assert(skip != NULL);
    for (i = 0; i < args->k; i++)
    {
        skip[i] = 1;
        encode_decode_test_impl(be_id, args, skip);
        skip[i] = 0;
    }
    free(skip);
}

static void test_decode_with_missing_parity(const ec_backend_id_t be_id,
                                            struct ec_args *args)
{
    int i;
    int *skip = create_skips_array(args,args->k);
    assert(skip != NULL);
    for (i = args->k; i < args->m; i++)
    {
        skip[i] = 1;
        encode_decode_test_impl(be_id, args, skip);
        skip[i] = 0;
    }
    free(skip);
}

static void test_decode_with_missing_multi_data(const ec_backend_id_t be_id,
                                                struct ec_args *args)
{
    int max_num_missing = args->k <= (args->hd - 1) ? args->k : args->hd - 1;
    int i,j;
    for (i = 0; i < args->k - max_num_missing + 1; i++) {
        int *skip = create_skips_array(args,-1);
        assert(skip != NULL);
        for (j = i; j < i + max_num_missing; j++) {
            skip[j % args->k]=1;
        }
        encode_decode_test_impl(be_id, args, skip);
        free(skip);
    }
}

static void test_decode_with_missing_multi_parity(const ec_backend_id_t be_id,
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
        encode_decode_test_impl(be_id, args, skip);
        free(skip);
    }
}

static void test_decode_with_missing_multi_data_parity(
    const ec_backend_id_t be_id, struct ec_args *args)
{
    int i,j;
    int max_num_missing = args->hd - 1;
    int end = (args->k + args->m) - max_num_missing + 1;
    for (i = 0; i < end; i++) {
        int *skip = create_skips_array(args,-1);
        assert(skip != NULL);
        for (j = i; j < i + max_num_missing; j++) {
            skip[j]=1;
        }
        encode_decode_test_impl(be_id, args, skip);
        free(skip);
    }
}

static void test_decode_with_missing_multi_data_parity_fail_with_isal(
    const ec_backend_id_t be_id, struct ec_args *args)
{
    int i,j;
    struct ec_args specific_75_args = {
        .k = 7,
        .m = 5,
    };
    int bad_positions[4][5] = {
        {0, 3, 5, 9, 10 },
        {0, 2, 5, 8, 9 },
        {1, 3, 6, 8, 9},
        {1, 4, 6, 9, 10}
    };
    for (i = 0; i < 4; i++) {
        int *skips = create_skips_array(&specific_75_args,-1);
        assert(skips != NULL);
        for (j = 0; j < 5; j++) {
            int skipped = bad_positions[i][j];
            skips[skipped]=1;
        }
        encode_decode_test_impl(be_id, &specific_75_args, skips);
        free(skips);
    }
}

static void test_isa_l_rs_vand_decode_reconstruct_specific_error_case(void)
{
    struct ec_args specific_1010_args = {
        .k = 10,
        .m = 10,
    };

    int rc = 0;
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size, 'x');
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int num_avail_frags = -1;
    char **avail_frags = NULL;
    char *decoded_data = NULL;
    char *out_frag = NULL;
    uint64_t decoded_data_len = 0;

    int *skips = create_skips_array(&specific_1010_args,-1);
    assert(skips != NULL);
    // available frags for a bad pattern: [0, 1, 2, 3, 4, 6, 7, 10, 12, 15]
    skips[5] = skips[8] = skips[9] = skips[11] = skips[13] = skips[14] =
        skips[16] = skips[17] = skips[18] = skips[19] = 1;

    desc = liberasurecode_instance_create(
        EC_BACKEND_ISA_L_RS_VAND, &specific_1010_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        free(orig_data);
        free(skips);
        return;
    }
    assert(desc > 0);

    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &specific_1010_args,
                                         skips);
    assert(num_avail_frags > 0);

    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, &decoded_data_len);
    assert(rc == -1);
    free(avail_frags);

    // 5 is a hole in available frags
    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &specific_1010_args,
                                         skips);
    out_frag = malloc(sizeof(char) * encoded_fragment_len);
    rc = liberasurecode_reconstruct_fragment(
        desc, avail_frags, 10, encoded_fragment_len, 5, out_frag);
    assert(rc == -1);

    free(out_frag);
    free(avail_frags);

    // sanity; [0, 1, 2, 3, 4, 6, 7, 10, 12, 14] is ok
    skips[15] = 1; skips[14] = 0;
    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                         encoded_parity, &specific_1010_args,
                                         skips);
    assert(num_avail_frags > 0);
    rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, &decoded_data_len);
    assert(rc == 0);
    // 5 is a hole in available frags
    out_frag = malloc(sizeof(char) * encoded_fragment_len);
    rc = liberasurecode_reconstruct_fragment(
        desc, avail_frags, 10, encoded_fragment_len, 5, out_frag);

    assert(rc == 0);

    free(out_frag);
    free(avail_frags);

    // cleanup all
    rc = liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    assert(rc == 0);

    rc = liberasurecode_decode_cleanup(desc, decoded_data);
    assert(rc == 0);

    assert(0 == liberasurecode_instance_destroy(desc));
    free(orig_data);
    free(skips);
}

static void test_jerasure_rs_cauchy_init_failure(void)
{
    struct ec_args bad_args = {
        .k = 10,
        .m = 10,
        .w = 4,
    };
    // NB: (k + m) > (1 << w) => too many frags!

    int desc = -1;
    desc = liberasurecode_instance_create(
        EC_BACKEND_JERASURE_RS_CAUCHY, &bad_args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(-EBACKENDINITERR == desc);
}

static void test_flat_xor_hd3_init_failure(void)
{
    struct ec_args bad_args[] = {
        {.k = 1, .m = 5, .hd=3},
        {.k = 5, .m = 1, .hd=3},
        {.k = 4, .m = 4, .hd=3},
        {.k = 1, .m = 3, .hd=3},
        {.k = 4, .m = 3, .hd=3},
    };

    for (int i = 0; i < sizeof(bad_args)/sizeof(bad_args[0]); ++i) {
        int desc = -1;
        desc = liberasurecode_instance_create(
            EC_BACKEND_FLAT_XOR_HD, &bad_args[i]);
        if (-EBACKENDNOTAVAIL == desc) {
            fprintf (stderr, "Backend library not available!\n");
            return;
        }
        assert(-EBACKENDINITERR == desc);
    }
}

static void test_flat_xor_too_many_failures(void)
{
    int desc = -1;
    int orig_data_size = 1024 * 1024;
    char *orig_data = NULL;
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    uint64_t decoded_data_len = 0;
    char *decoded_data = NULL;
    char **avail_frags = NULL;
    int num_avail_frags = 0;
    int rc = -1;
    struct ec_args bad_args[] = {
        {.k = 5, .m = 5, .hd=3},
        {.k = 5, .m = 5, .hd=4},
    };
    int *skip = create_skips_array(bad_args,-1);
    skip[0] = skip[1] = skip[2] = skip[3] = 1;

    for (int i = 0; i < sizeof(bad_args)/sizeof(bad_args[0]); ++i) {
        desc = liberasurecode_instance_create(
            EC_BACKEND_FLAT_XOR_HD, &bad_args[i]);
        assert(desc > 0);
        orig_data = create_buffer(orig_data_size, 'x');
        assert(orig_data != NULL);
        rc = liberasurecode_encode(desc, orig_data, orig_data_size,
                &encoded_data, &encoded_parity, &encoded_fragment_len);
        assert(0 == rc);

        num_avail_frags = create_frags_array(&avail_frags, encoded_data,
                                             encoded_parity, &bad_args[i], skip);
        assert(num_avail_frags > 0);
        rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                                   encoded_fragment_len, 1,
                                   &decoded_data, &decoded_data_len);
        assert(-1 == rc);
        assert(decoded_data == NULL);
        assert(decoded_data_len == 0);
        rc = liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
        assert(rc == 0);

        assert(0 == liberasurecode_instance_destroy(desc));
        free(orig_data);
        free(avail_frags);
    }
    free(skip);
}

static void test_simple_encode_decode(const ec_backend_id_t be_id,
                                     struct ec_args *args)
{
    int *skip = create_skips_array(args,-1);
    assert(skip != NULL);
    encode_decode_test_impl(be_id, args, skip);
    free(skip);
}

static void test_jerasure_rs_vand_simple_encode_decode_over32(void)
{
    struct ec_args over32_args = {
        .k = 30,
        .m = 20,
    };

    int *skip = create_skips_array(&over32_args, 1);
    assert(skip != NULL);
    // should return an error
    encode_decode_test_impl(EC_BACKEND_JERASURE_RS_VAND,
                            &over32_args, skip);
    free(skip);
}

static void test_simple_reconstruct(const ec_backend_id_t be_id,
                                    struct ec_args *args)
{
    int i = 0;
    for (i = 0; i < args->k + args->m; i++) {
        int *skip = create_skips_array(args,i);
        assert(skip != NULL);
        reconstruct_test_impl(be_id, args, skip);
        free(skip);
    }
}

static void test_fragments_needed(const ec_backend_id_t be_id,
                                  struct ec_args *args)
{
    test_fragments_needed_impl(be_id, args);
}

static void test_verify_stripe_metadata(const ec_backend_id_t be_id,
                                        struct ec_args *args)
{
    int orig_data_size = 1024;
    char **encoded_data = NULL, **encoded_parity = NULL;
    char **avail_frags = NULL;
    uint64_t encoded_fragment_len = 0;
    int rc = -1;
    int num_avail_frags = -1;
    int *skip = create_skips_array(args,-1);
    char *orig_data = create_buffer(orig_data_size, 'x');
    int desc = liberasurecode_instance_create(be_id, args);

    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        free(orig_data);
        free(skip);
        return;
    }
    assert(desc > 0);

    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);

    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
            encoded_parity, args, skip);

    rc = liberasurecode_verify_stripe_metadata(desc, avail_frags,
            num_avail_frags);
    assert(0 == rc);

    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
    free(orig_data);
    free(skip);
    free(avail_frags);
}

static void verify_fragment_metadata_mismatch_impl(const ec_backend_id_t be_id, struct ec_args *args,
        fragment_mismatch_scenario_t scenario)
{
    int orig_data_size = 1024;
    char **encoded_data = NULL, **encoded_parity = NULL;
    char **avail_frags = NULL;
    uint64_t encoded_fragment_len = 0;
    int rc = -1;
    int num_avail_frags = -1;
    int i = 0;
    uint32_t orig_libec_ver = 0;
    uint32_t orig_be_ver = 0;
    uint8_t orig_be_id = 0;
    uint32_t orig_frag_idx = 0;
    int *skip = create_skips_array(args,-1);
    char *orig_data = create_buffer(orig_data_size, 'x');
    int desc = liberasurecode_instance_create(be_id, args);

    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        free(orig_data);
        free(skip);
        return;
    }
    assert(desc > 0);
    assert(orig_data != NULL);
    rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == rc);
    num_avail_frags = create_frags_array(&avail_frags, encoded_data,
            encoded_parity, args, skip);
    for (i = 0; i < num_avail_frags; i++) {
        char * cur_frag = avail_frags[i];
        // corrupt fragment
        switch (scenario) {
            case LIBEC_VERSION_MISMATCH:
                orig_libec_ver = ((fragment_header_t*)cur_frag)->libec_version;
                ((fragment_header_t*)cur_frag)->libec_version = orig_libec_ver + 1;
                break;
            case MAGIC_MISMATCH:
                ((fragment_header_t*)cur_frag)->magic = 0;
                break;
            case BACKEND_ID_MISMATCH:
                orig_be_id = ((fragment_header_t*)cur_frag)->meta.backend_id;
                ((fragment_header_t*)cur_frag)->meta.backend_id = orig_be_id + 1;
                break;
            case BACKEND_VERSION_MISMATCH:
                orig_be_ver = ((fragment_header_t*)cur_frag)->meta.backend_version;
                ((fragment_header_t*)cur_frag)->meta.backend_version = orig_be_ver + 1;
                break;
            case FRAGIDX_INVALID:
                orig_frag_idx = ((fragment_header_t*)cur_frag)->meta.idx;
                ((fragment_header_t*)cur_frag)->meta.idx = -1;
                break;
            case FRAGIDX_OUT_OF_RANGE:
                orig_frag_idx = ((fragment_header_t*)cur_frag)->meta.idx;
                ((fragment_header_t*)cur_frag)->meta.idx = args->k + args->m + 1;
                break;
            default:
                assert(false);
        }
        rc = is_invalid_fragment(desc, avail_frags[i]);
        assert(rc == 1);
        // heal fragment
        switch (scenario) {
            case LIBEC_VERSION_MISMATCH:
                ((fragment_header_t*)cur_frag)->libec_version = orig_libec_ver;
                break;
            case MAGIC_MISMATCH:
                ((fragment_header_t*)cur_frag)->magic = LIBERASURECODE_FRAG_HEADER_MAGIC;
                break;
            case BACKEND_ID_MISMATCH:
                ((fragment_header_t*)cur_frag)->meta.backend_id = orig_be_id;
                break;
            case BACKEND_VERSION_MISMATCH:
                ((fragment_header_t*)cur_frag)->meta.backend_version = orig_be_ver;
                break;
            case FRAGIDX_INVALID:
            case FRAGIDX_OUT_OF_RANGE:
                ((fragment_header_t*)cur_frag)->meta.idx = orig_frag_idx;
                break;
            default:
                break;
        }
    }
    liberasurecode_encode_cleanup(desc, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(desc);
    free(avail_frags);
    free(orig_data);
    free(skip);
}
static void test_verify_stripe_metadata_libec_mismatch(
        const ec_backend_id_t be_id, struct ec_args *args)
{
    verify_fragment_metadata_mismatch_impl(be_id, args, LIBEC_VERSION_MISMATCH);
}

static void test_verify_stripe_metadata_magic_mismatch(
        const ec_backend_id_t be_id, struct ec_args *args)
{
    verify_fragment_metadata_mismatch_impl(be_id, args, MAGIC_MISMATCH);
}

static void test_verify_stripe_metadata_be_id_mismatch(
        const ec_backend_id_t be_id, struct ec_args *args)
{
    verify_fragment_metadata_mismatch_impl(be_id, args, BACKEND_ID_MISMATCH);
}

static void test_verify_stripe_metadata_be_ver_mismatch(
        const ec_backend_id_t be_id, struct ec_args *args)
{
    verify_fragment_metadata_mismatch_impl(be_id, args, BACKEND_VERSION_MISMATCH);
}

static void test_verify_stripe_metadata_frag_idx_invalid(
        const ec_backend_id_t be_id, struct ec_args *args)
{
    verify_fragment_metadata_mismatch_impl(be_id, args, FRAGIDX_INVALID);
    verify_fragment_metadata_mismatch_impl(be_id, args, FRAGIDX_OUT_OF_RANGE);
}

static void test_metadata_crcs_le(void)
{
    // We've observed headers like this in the wild, using our busted crc32
    char orig_header[] =
        "\x03\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x10\x00"
        "\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x07\x01\x0e\x02\x00\xcc\x5e\x0c\x0b\x00"
        "\x04\x01\x00\x22\xee\x45\xb9\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    char header[sizeof(orig_header)];
    memcpy(header, orig_header, sizeof(orig_header));

    fragment_metadata_t res;

    assert(liberasurecode_get_fragment_metadata(header, &res) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);
    assert(res.backend_version == _VERSION(2, 14, 1));
    assert(is_invalid_fragment_header((fragment_header_t *) header) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);

    // Switch it to zlib's implementation
    orig_header[70] = '\x18';
    orig_header[69] = '\x73';
    orig_header[68] = '\xf8';
    orig_header[67] = '\xec';
    memcpy(header, orig_header, sizeof(orig_header));

    assert(liberasurecode_get_fragment_metadata(header, &res) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);
    assert(res.backend_version == _VERSION(2, 14, 1));
    assert(is_invalid_fragment_header((fragment_header_t *) header) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);

    // Write down the wrong thing
    header[70] = '\xff';
    assert(liberasurecode_get_fragment_metadata(header, &res) == -EBADHEADER);
    assert(is_invalid_fragment_header((fragment_header_t *) header) == 1);
}

static void test_metadata_crcs_be(void)
{
    // Like above, but big-endian
    char orig_header[] =
        "\x00\x00\x00\x03\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x10\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x07\x00\x02\x0e\x01\x0b\x0c\x5e\xcc\x00"
        "\x01\x04\x00\xfa\x85\x40\x70\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    char header[sizeof(orig_header)];
    memcpy(header, orig_header, sizeof(orig_header));

    fragment_metadata_t res;

    assert(liberasurecode_get_fragment_metadata(header, &res) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);
    assert(res.backend_version == _VERSION(2, 14, 1));
    assert(is_invalid_fragment_header((fragment_header_t *) header) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);

    // Switch it to zlib's implementation
    orig_header[67] = '\xe3';
    orig_header[68] = '\x73';
    orig_header[69] = '\x88';
    orig_header[70] = '\xa0';
    memcpy(header, orig_header, sizeof(orig_header));

    assert(liberasurecode_get_fragment_metadata(header, &res) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);
    assert(res.backend_version == _VERSION(2, 14, 1));
    assert(is_invalid_fragment_header((fragment_header_t *) header) == 0);
    assert(memcmp(header, orig_header, sizeof(orig_header)) == 0);

    // Write down the wrong thing
    header[70] = '\xff';
    assert(liberasurecode_get_fragment_metadata(header, &res) == -EBADHEADER);
    assert(is_invalid_fragment_header((fragment_header_t *) header) == 1);
}

//static void test_verify_str

/* An individual test, useful to ensure the reported name
   reflects the function name */
#define TEST(test, backend, checksum) {#test, test, backend, checksum, .skip = false}
/* Block of common tests for the "real" backends */
#define TEST_SUITE(backend) \
    TEST({.with_args = test_create_and_destroy_backend},               backend, CHKSUM_NONE), \
    TEST({.with_args = test_create_and_destroy_multiple_backends},     backend, CHKSUM_NONE), \
    TEST({.with_args = test_simple_encode_decode},                     backend, CHKSUM_NONE), \
    TEST({.with_args = test_decode_with_missing_data},                 backend, CHKSUM_NONE), \
    TEST({.with_args = test_decode_with_missing_parity},               backend, CHKSUM_NONE), \
    TEST({.with_args = test_decode_with_missing_multi_data},           backend, CHKSUM_NONE), \
    TEST({.with_args = test_decode_with_missing_multi_parity},         backend, CHKSUM_NONE), \
    TEST({.with_args = test_decode_with_missing_multi_data_parity},    backend, CHKSUM_NONE), \
    TEST({.with_args = test_simple_reconstruct},                       backend, CHKSUM_NONE), \
    TEST({.with_args = test_fragments_needed},                         backend, CHKSUM_NONE), \
    TEST({.with_args = test_get_fragment_metadata},                    backend, CHKSUM_NONE), \
    TEST({.with_args = test_get_fragment_metadata},                    backend, CHKSUM_CRC32), \
    TEST({.with_args = test_write_legacy_fragment_metadata},           backend, CHKSUM_CRC32), \
    TEST({.with_args = test_verify_stripe_metadata},                   backend, CHKSUM_CRC32), \
    TEST({.with_args = test_verify_stripe_metadata_libec_mismatch},    backend, CHKSUM_CRC32), \
    TEST({.with_args = test_verify_stripe_metadata_magic_mismatch},    backend, CHKSUM_CRC32), \
    TEST({.with_args = test_verify_stripe_metadata_be_id_mismatch},    backend, CHKSUM_CRC32), \
    TEST({.with_args = test_verify_stripe_metadata_be_ver_mismatch},   backend, CHKSUM_CRC32), \
    TEST({.with_args = test_verify_stripe_metadata_frag_idx_invalid},  backend, CHKSUM_CRC32)

struct testcase testcases[] = {
    TEST({.no_args = test_backend_available_invalid_args}, EC_BACKENDS_MAX, 0),
    TEST({.no_args = test_backend_available}, EC_BACKENDS_MAX, 0),
    TEST({.no_args = test_create_backend_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_destroy_backend_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_encode_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_encode_cleanup_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_decode_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_decode_cleanup_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_reconstruct_fragment_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_get_fragment_metadata_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_verify_stripe_metadata_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_fragments_needed_invalid_args}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_get_fragment_partition}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_liberasurecode_get_version}, EC_BACKENDS_MAX, CHKSUM_TYPES_MAX),
    TEST({.no_args = test_metadata_crcs_le}, EC_BACKENDS_MAX, 0),
    TEST({.no_args = test_metadata_crcs_be}, EC_BACKENDS_MAX, 0),
    // NULL backend test
    TEST({.with_args = test_create_and_destroy_backend}, EC_BACKEND_NULL, CHKSUM_NONE),
    TEST({.with_args = test_simple_encode_decode}, EC_BACKEND_NULL, CHKSUM_NONE),
    TEST({.with_args = test_get_fragment_metadata}, EC_BACKEND_NULL, CHKSUM_NONE),
    TEST({.with_args = test_decode_with_missing_parity}, EC_BACKEND_NULL, CHKSUM_NONE),
    TEST({.with_args = test_decode_with_missing_multi_parity}, EC_BACKEND_NULL, CHKSUM_NONE),
    // Flat XOR backend tests
    TEST_SUITE(EC_BACKEND_FLAT_XOR_HD),
    TEST({.no_args = test_flat_xor_hd3_init_failure}, EC_BACKENDS_MAX, 0),
    TEST({.no_args = test_flat_xor_too_many_failures}, EC_BACKENDS_MAX, 0),
    // Jerasure RS Vand backend tests
    TEST_SUITE(EC_BACKEND_JERASURE_RS_VAND),
    TEST({.no_args = test_jerasure_rs_vand_simple_encode_decode_over32}, EC_BACKENDS_MAX, 0),
    // Jerasure RS Cauchy backend tests
    TEST_SUITE(EC_BACKEND_JERASURE_RS_CAUCHY),
    TEST({.no_args = test_jerasure_rs_cauchy_init_failure}, EC_BACKENDS_MAX, 0),
    // ISA-L rs_vand tests
    TEST_SUITE(EC_BACKEND_ISA_L_RS_VAND),
    TEST({.no_args = test_isa_l_rs_vand_decode_reconstruct_specific_error_case}, EC_BACKENDS_MAX, 0),
    // ISA-L rs cauchy tests
    TEST_SUITE(EC_BACKEND_ISA_L_RS_CAUCHY),
    TEST({.with_args = test_decode_with_missing_multi_data_parity_fail_with_isal},    EC_BACKEND_ISA_L_RS_CAUCHY, 0),
    // ISA-L rs vand inv tests
    TEST_SUITE(EC_BACKEND_ISA_L_RS_VAND_INV),
    TEST({.with_args = test_decode_with_missing_multi_data_parity_fail_with_isal},    EC_BACKEND_ISA_L_RS_VAND_INV, 0),
    // shss tests
    TEST_SUITE(EC_BACKEND_SHSS),
    // Internal RS Vand backend tests
    TEST_SUITE(EC_BACKEND_LIBERASURECODE_RS_VAND),
    // libphazr backend tests
    TEST_SUITE(EC_BACKEND_LIBPHAZR),
    { NULL, {.no_args = NULL}, 0, 0, false },
};

int main(int argc, char **argv)
{
    int ii = 0, num_cases = 0, i = 0;
    int max_backend_tests = max_tests_for_backends();
    setbuf(stdout, NULL);

    for (i = 0; i < max_backend_tests; i++) {
        for (ii = 0; testcases[ii].description != NULL; ++ii) {
            const char *testname = get_name_from_backend_id(testcases[ii].be_id);
            if (testcases[ii].skip) {
                printf("ok # SKIP %d - %s: %s (idx=%d)\n", num_cases,
                       testcases[ii].description,
                       (testname) ? testname : "", i);
                continue;
            }
            if (testcases[ii].be_id == EC_BACKENDS_MAX) {
                if (0 != i) {
                    continue;
                }
                /* EC_BACKENDS_MAX basically designed for invalid args tests
                 * and not takes the args so call the function w/o args here */
                printf("%d - %s: %s (idx=%d) ... ", num_cases,
                       testcases[ii].description,
                       (testname) ? testname : "", i);
                testcases[ii].function.no_args();
                printf("ok\n");
                num_cases++;
                continue;
            }
            struct ec_args *args = create_ec_args(testcases[ii].be_id, testcases[ii].ct, i);
            if (NULL != args) {
                printf("%d - %s: %s (idx=%d) ... ", num_cases,
                       testcases[ii].description,
                       (testname) ? testname : "", i);
                testcases[ii].function.with_args(testcases[ii].be_id, args);
                printf("ok\n");
                free(args);
                num_cases++;
            }
        }
    }
    return 0;
}
