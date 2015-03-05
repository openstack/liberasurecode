/*
 * Copyright 2014 Tushar Gohad, Kevin M Greenan, Eric Lambert
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
 * liberasurecode postprocessing helpers implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_stdinc.h"

void add_fragment_metadata(ec_backend_t be, char *fragment,
        int idx, uint64_t orig_data_size, int blocksize,
        ec_checksum_type_t ct, int add_chksum)
{
    //TODO EDL we are ignoring the return codes here, fix that
    set_libec_version(fragment);
    set_fragment_idx(fragment, idx);
    set_orig_data_size(fragment, orig_data_size);
    set_fragment_payload_size(fragment, blocksize);
    set_backend_id(fragment, be->common.id);
    set_backend_version(fragment, be->common.ec_backend_version);
    set_fragment_backend_metadata_size(fragment, be->common.backend_metadata_size);

    if (add_chksum) {
        set_checksum(ct, fragment, blocksize);
    }
}

int finalize_fragments_after_encode(ec_backend_t instance,
        int k, int m, int blocksize, uint64_t orig_data_size,
        char **encoded_data, char **encoded_parity)
{
    int i, set_chksum = 1;
    ec_checksum_type_t ct = instance->args.uargs.ct;

    /* finalize data fragments */
    for (i = 0; i < k; i++) {
        char *fragment = get_fragment_ptr_from_data(encoded_data[i]);
        add_fragment_metadata(instance, fragment, i, orig_data_size,
                blocksize, ct, set_chksum);
        encoded_data[i] = fragment;
    }

    /* finalize parity fragments */
    for (i = 0; i < m; i++) {
        char *fragment = get_fragment_ptr_from_data(encoded_parity[i]);
        add_fragment_metadata(instance, fragment, i + k, orig_data_size,
                blocksize, ct, set_chksum);
        encoded_parity[i] = fragment;
    }

    return 0;
}
