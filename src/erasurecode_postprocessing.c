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
 * liberasurecode postprocessing helpers implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include "erasurecode_backend.h"
#include "erasurecode_helpers.h"
#include "erasurecode_stdinc.h"

int finalize_fragments_after_encode(ec_backend_t instance,
        int k, int m, int blocksize,
        char **encoded_data, char **encoded_parity)
{
    int i;
    // int add_checksum = instance->args.uargs.inline_chksum;
    int add_checksum = 1;

    /* finalize data fragments */
    for (i = 0; i < k; i++) {
        char *fragment = get_fragment_ptr_from_data(encoded_data[i]);
        set_fragment_idx(fragment, i);
        if (add_checksum) {
            int chksum = crc32(0, encoded_data[i], blocksize);
            set_chksum(fragment, chksum);
        }
        encoded_data[i] = fragment;
    }

    /* finalize parity fragments */
    for (i = 0; i < m; i++) {
        char *fragment = get_fragment_ptr_from_data(encoded_parity[i]);
        set_fragment_idx(fragment, i + k);
        if (add_checksum) {
            int chksum = crc32(0, encoded_parity[i], blocksize);
            set_chksum(fragment, chksum);
        }
        encoded_parity[i] = fragment;
    }

    return 0;
}
