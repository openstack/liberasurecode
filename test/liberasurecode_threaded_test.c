/*
 * Copyright 2025 Tim Burke
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
 */

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "erasurecode.h"
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

typedef void (*TEST_FUNC_WITH_ARGS)(const ec_backend_id_t, struct ec_args *);
struct testcase {
    const char *description;
    TEST_FUNC_WITH_ARGS function;
    ec_backend_id_t be_id;
};

char *create_buffer(int size)
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

void* destroy_backend_in_thread(void* arg)
{
    int *desc = arg;
    int *rc = malloc(sizeof(int));
    *rc = liberasurecode_instance_destroy(*desc);
    assert(0 == *rc || -EBACKENDNOTAVAIL == *rc);
    return rc;
}

static void test_multi_thread_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *rc1, *rc2;
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);
    pthread_create(&tid1, NULL, destroy_backend_in_thread, &desc);
    pthread_create(&tid2, NULL, destroy_backend_in_thread, &desc);
    pthread_join(tid1, (void *) &rc1);
    pthread_join(tid2, (void *) &rc2);
    /* The threads race; only one succeeds */
    assert(*rc1 == 0 || *rc2 == 0);
    /* The other fails with -EBACKENDNOTAVAIL */
    assert(*rc1 == -EBACKENDNOTAVAIL || *rc2 == -EBACKENDNOTAVAIL);
    free(rc1);
    free(rc2);
}

struct create_state {
    ec_backend_id_t be_id;
    struct ec_args *args;
};

void* create_backend_in_thread(void* arg)
{
    struct create_state *s = arg;
    int *desc = malloc(sizeof(int));
    *desc = liberasurecode_instance_create(s->be_id, s->args);
    return desc;
}

static void test_multi_thread_create_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *desc1, *desc2;

    struct create_state s = {
        be_id,
        args
    };
    if (!liberasurecode_backend_available(be_id)) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }

    pthread_create(&tid1, NULL, create_backend_in_thread, &s);
    pthread_create(&tid2, NULL, create_backend_in_thread, &s);
    pthread_join(tid1, (void *) &desc1);
    pthread_join(tid2, (void *) &desc2);
    assert(*desc1 > 0);
    assert(*desc2 > 0);
    assert(*desc1 != *desc2);
    /**
     * Each can be destroyed, showing that each was properly added to the
     * registry */
    assert(0 == liberasurecode_instance_destroy(*desc1));
    assert(0 == liberasurecode_instance_destroy(*desc2));
    free(desc1);
    free(desc2);
}

struct encode_state {
    int desc1;
    int desc2;
    int data_sz;
    char *data;
};

void* encode_in_thread(void* arg)
{
    struct encode_state *s = arg;
    int *rc = malloc(sizeof(int));
    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    *rc = liberasurecode_encode(s->desc1, s->data, s->data_sz,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(0 == *rc || -EBACKENDNOTAVAIL == *rc);
    if (*rc == 0) {
        assert(liberasurecode_encode_cleanup(s->desc2, encoded_data, encoded_parity) == 0);
    }
    return rc;
}

static void test_multi_thread_encode_and_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *rc1, *rc2;
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);
    int orig_data_size = 1024 * 1024;
    struct encode_state s = {
        desc,
        /* need a second descriptor so we can clean up
         * even after first descriptor is destroyed */
        liberasurecode_instance_create(be_id, args),
        orig_data_size,
        create_buffer(orig_data_size)
    };
    assert(s.data != NULL);
    pthread_create(&tid2, NULL, encode_in_thread, &s);
    pthread_create(&tid1, NULL, destroy_backend_in_thread, &desc);
    pthread_join(tid1, (void *) &rc1);
    pthread_join(tid2, (void *) &rc2);
    /* The threads race, but destroy always succeeds */
    assert(*rc1 == 0);
    assert(liberasurecode_instance_destroy(s.desc2) == 0);
    free(rc1);
    free(rc2);
    free(s.data);
}

struct decode_state {
    int desc1;
    int desc2;
    char **available_fragments;
    int num_fragments;
    uint64_t fragment_len;
};

void* decode_in_thread(void* arg)
{
    struct decode_state *s = arg;
    int *rc = malloc(sizeof(int));
    char *decoded_data = NULL;
    uint64_t decoded_data_len = 0;
    *rc = liberasurecode_decode(s->desc1, s->available_fragments,
            s->num_fragments, s->fragment_len, 0, /* force_metadata_checks */
            &decoded_data, &decoded_data_len);
    assert(0 == *rc || -EBACKENDNOTAVAIL == *rc);
    if (*rc == 0) {
        assert(liberasurecode_decode_cleanup(s->desc2, decoded_data) == 0);
    }
    return rc;
}

static void test_multi_thread_decode_and_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *rc1, *rc2;
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size);
    assert(orig_data != NULL);

    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    /* Create available fragments array using the same method as existing tests */
    char **available_fragments = NULL;
    int *skip = create_skips_array(args, -1);
    assert(skip != NULL);
    int num_avail_frags = create_frags_array(&available_fragments, encoded_data,
                                            encoded_parity, args, skip);
    assert(num_avail_frags > 0);

    struct decode_state s = {
        desc,
        /* need a second descriptor so we can clean up
         * even after first descriptor is destroyed */
        liberasurecode_instance_create(be_id, args),
        available_fragments,
        num_avail_frags,
        encoded_fragment_len
    };
    assert(s.desc2 > 0);

    pthread_create(&tid2, NULL, decode_in_thread, &s);
    pthread_create(&tid1, NULL, destroy_backend_in_thread, &desc);
    pthread_join(tid1, (void *) &rc1);
    pthread_join(tid2, (void *) &rc2);
    /* The threads race, but destroy always succeeds */
    assert(*rc1 == 0);

    liberasurecode_encode_cleanup(s.desc2, encoded_data, encoded_parity);
    assert(liberasurecode_instance_destroy(s.desc2) == 0);

    free(available_fragments);
    free(skip);
    free(rc1);
    free(rc2);
    free(orig_data);
}

struct reconstruct_state {
    int desc1;
    int desc2;
    char **available_fragments;
    int num_fragments;
    uint64_t fragment_len;
    int missing_idx;
    char *out_fragment;
};

void* reconstruct_in_thread(void* arg)
{
    struct reconstruct_state *s = arg;
    int *rc = malloc(sizeof(int));
    *rc = liberasurecode_reconstruct_fragment(s->desc1, s->available_fragments,
            s->num_fragments, s->fragment_len, s->missing_idx, s->out_fragment);
    assert(0 == *rc || -EBACKENDNOTAVAIL == *rc);
    return rc;
}

static void test_multi_thread_reconstruct_and_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *rc1, *rc2;
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    int orig_data_size = 1024 * 1024;
    char *orig_data = create_buffer(orig_data_size);
    assert(orig_data != NULL);

    char **encoded_data = NULL, **encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;
    int rc = liberasurecode_encode(desc, orig_data, orig_data_size,
            &encoded_data, &encoded_parity, &encoded_fragment_len);
    assert(rc == 0);

    /* Create available fragments array with one missing fragment */
    char **available_fragments = NULL;
    int *skip = create_skips_array(args, -1);
    assert(skip != NULL);
    skip[0] = 1; /* skip first fragment */
    int num_avail_frags = create_frags_array(&available_fragments, encoded_data,
                                            encoded_parity, args, skip);
    assert(num_avail_frags > 0);

    /* Allocate output fragment buffer */
    char *out_fragment = malloc(encoded_fragment_len);
    assert(out_fragment != NULL);

    struct reconstruct_state s = {
        desc,
        /* need a second descriptor so we can clean up
         * even after first descriptor is destroyed */
        liberasurecode_instance_create(be_id, args),
        available_fragments,
        num_avail_frags,
        encoded_fragment_len,
        0,  /* missing fragment index */
        out_fragment
    };
    assert(s.desc2 > 0);

    pthread_create(&tid2, NULL, reconstruct_in_thread, &s);
    pthread_create(&tid1, NULL, destroy_backend_in_thread, &desc);
    pthread_join(tid1, (void *) &rc1);
    pthread_join(tid2, (void *) &rc2);
    /* The threads race, but destroy always succeeds */
    assert(*rc1 == 0);

    liberasurecode_encode_cleanup(s.desc2, encoded_data, encoded_parity);
    assert(liberasurecode_instance_destroy(s.desc2) == 0);

    free(available_fragments);
    free(out_fragment);
    free(skip);
    free(rc1);
    free(rc2);
    free(orig_data);
}

struct fragments_needed_state {
    int desc1;
    int desc2;
    int *fragments_to_reconstruct;
    int *fragments_to_exclude;
    int *fragments_needed;
};

void* fragments_needed_in_thread(void* arg)
{
    struct fragments_needed_state *s = arg;
    int *rc = malloc(sizeof(int));
    *rc = liberasurecode_fragments_needed(s->desc1, s->fragments_to_reconstruct,
            s->fragments_to_exclude, s->fragments_needed);
    assert(0 == *rc || -EBACKENDNOTAVAIL == *rc);
    return rc;
}

static void test_multi_thread_fragments_needed_and_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *rc1, *rc2;
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    int fragments_to_reconstruct[] = {0, -1};
    int fragments_to_exclude[] = {-1};
    int *fragments_needed = malloc(sizeof(int) * (args->k + args->m + 1));
    assert(fragments_needed != NULL);

    struct fragments_needed_state s = {
        desc,
        /* need a second descriptor so we can clean up
         * even after first descriptor is destroyed */
        liberasurecode_instance_create(be_id, args),
        fragments_to_reconstruct,
        fragments_to_exclude,
        fragments_needed
    };
    assert(s.desc2 > 0);

    pthread_create(&tid2, NULL, fragments_needed_in_thread, &s);
    pthread_create(&tid1, NULL, destroy_backend_in_thread, &desc);
    pthread_join(tid1, (void *) &rc1);
    pthread_join(tid2, (void *) &rc2);
    /* The threads race, but destroy always succeeds */
    assert(*rc1 == 0);

    assert(liberasurecode_instance_destroy(s.desc2) == 0);
    free(fragments_needed);
    free(rc1);
    free(rc2);
}

struct get_fragment_size_state {
    int desc1;
    int desc2;
    int data_len;
};

void* get_fragment_size_in_thread(void* arg)
{
    struct get_fragment_size_state *s = arg;
    int *rc = malloc(sizeof(int));
    *rc = liberasurecode_get_fragment_size(s->desc1, s->data_len);
    /* This function returns size on success or negative error code on failure */
    assert(*rc > 0 || *rc == -EBACKENDNOTAVAIL);
    return rc;
}

static void test_multi_thread_get_fragment_size_and_destroy_backend(
        ec_backend_id_t be_id,
        struct ec_args *args)
{
    pthread_t tid1, tid2;
    int *rc1, *rc2;
    int desc = liberasurecode_instance_create(be_id, args);
    if (-EBACKENDNOTAVAIL == desc) {
        fprintf(stderr, "Backend library not available!\n");
        return;
    }
    assert(desc > 0);

    struct get_fragment_size_state s = {
        desc,
        /* need a second descriptor so we can clean up
         * even after first descriptor is destroyed */
        liberasurecode_instance_create(be_id, args),
        1024 * 1024  /* data length */
    };
    assert(s.desc2 > 0);

    pthread_create(&tid2, NULL, get_fragment_size_in_thread, &s);
    pthread_create(&tid1, NULL, destroy_backend_in_thread, &desc);
    pthread_join(tid1, (void *) &rc1);
    pthread_join(tid2, (void *) &rc2);
    /* The threads race, but destroy always succeeds */
    assert(*rc1 == 0);

    assert(liberasurecode_instance_destroy(s.desc2) == 0);
    free(rc1);
    free(rc2);
}

#define TEST(test, backend) {#test, test, backend}
#define TEST_SUITE(backend) \
    TEST(test_multi_thread_destroy_backend,                       backend), \
    TEST(test_multi_thread_create_backend,                        backend), \
    TEST(test_multi_thread_encode_and_destroy_backend,            backend), \
    TEST(test_multi_thread_decode_and_destroy_backend,            backend), \
    TEST(test_multi_thread_reconstruct_and_destroy_backend,       backend), \
    TEST(test_multi_thread_fragments_needed_and_destroy_backend,  backend), \
    TEST(test_multi_thread_get_fragment_size_and_destroy_backend, backend)

struct testcase testcases[] = {
    TEST_SUITE(EC_BACKEND_FLAT_XOR_HD),
    TEST_SUITE(EC_BACKEND_LIBERASURECODE_RS_VAND),
    TEST_SUITE(EC_BACKEND_JERASURE_RS_VAND),
    TEST_SUITE(EC_BACKEND_JERASURE_RS_CAUCHY),
    TEST_SUITE(EC_BACKEND_ISA_L_RS_VAND),
    TEST_SUITE(EC_BACKEND_ISA_L_RS_CAUCHY),
    TEST_SUITE(EC_BACKEND_ISA_L_RS_VAND_INV),
    TEST_SUITE(EC_BACKEND_SHSS),
    TEST_SUITE(EC_BACKEND_LIBPHAZR),
    { NULL, NULL, 0},
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

struct ec_args args = {
    .k = 10,
    .m = 5,
    .w = 16,
    .hd = 4,
    .ct = CHKSUM_NONE,
};

int main(int argc, char **argv)
{
    int n;
    setbuf(stdout, NULL);

    for (n = 0; testcases[n].description != NULL; ++n) {
        printf("%d - %s: %s ... ", n,
               testcases[n].description,
               get_name_from_backend_id(testcases[n].be_id));
        if (!liberasurecode_backend_available(testcases[n].be_id)) {
            printf("skip: backend not available\n");
        } else {
            testcases[n].function(testcases[n].be_id, &args);
            printf("ok\n");
        }
    }
    return 0;
}
