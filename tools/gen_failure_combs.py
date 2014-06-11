# Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.  THIS SOFTWARE IS
# PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
# NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import optparse
import mpmath
import itertools
import sys
import random


def get_combinations(list, k):
    return itertools.combinations(list, k)

if len(sys.argv) != 3:
    print("Usage: %s <num_fragments> <num_combs>")
    sys.exit(1)

n = int(sys.argv[1])
k = int(sys.argv[2])

if n is None or k is None:
    print("Usage: %s <num_fragments> <num_combs>")
    sys.exit(1)

fragments = [i for i in range(n)]
fragment_combinations = []

for i in range(1, k + 1):
    fragment_combinations.extend(
        [list(comb) for comb in get_combinations(fragments, i)])

for comb in fragment_combinations:
    while len(comb) < 4:
        comb.append(-1)

failure_comb_format_str = \
    "int failure_combs_%d_%d[NUM_%d_%d_COMBS][%d] =  %s ;"

fragment_combination_str = (
    "%s" % fragment_combinations).replace("[", "{").replace("]", "}")

print("#define NUM_%d_%d_COMBS %d" %
      (n, k + 1, len(fragment_combinations)))
print(failure_comb_format_str %
      (n, k + 1, n, k + 1, 4, fragment_combination_str))
