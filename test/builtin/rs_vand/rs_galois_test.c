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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rs_galois.h>

int test_inverse()
{
  int *uniq = (int*)malloc(sizeof(int)*FIELD_SIZE);
  int i = 0;

  memset(uniq, 0, sizeof(int)*FIELD_SIZE);

  rs_galois_init_tables();

  for (i = 1; i < FIELD_SIZE; i++) {
    if (uniq[i] != 0) {
      fprintf(stderr, "Duplicate %d: %d , %d \n", i, uniq[i], rs_galois_inverse(i));
      return 1;
    }
    uniq[i] = rs_galois_inverse(i); 
    int one = rs_galois_mult(rs_galois_inverse(i), i);
    if (one != 1) {
      fprintf(stderr, "%d is not the inverse of %d = %d\n", rs_galois_inverse(i), i, one);
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  int ret = 0;
  if (test_inverse() != 0) {
    fprintf(stderr, "test_inverse() failed\n");
    ret = 1;
  }
  return ret;
}
