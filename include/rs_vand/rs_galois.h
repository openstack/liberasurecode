/* 
 * Copyright 2015 Kevin M Greenan
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
 * vi: set noai tw=79 ts=4 sw=4:
 */

// DISCLAIMER: This is a totally basic implementation of RS used if a user does not
// want to install one of the supported backends, such as Jerasure and ISA-L.
// This is not expected to perform as well as the other supported backends,
// but does not make any assumptions about the host system.  Using a library
// like Jerasure with GF-Complete will give users the ability to tune to their
// architecture (Intel or ARM), CPU and memory (lots of options).

// We are only implementing w=16 here.  If you want to use something
// else, then use Jerasure with GF-Complete or ISA-L.
#define PRIM_POLY 0x1100b
#define FIELD_SIZE (1 << 16)
#define GROUP_SIZE (FIELD_SIZE - 1)

void rs_galois_init_tables();
void rs_galois_deinit_tables();
int rs_galois_mult(int x, int y);
int rs_galois_div(int x, int y);
int rs_galois_inverse(int x);

