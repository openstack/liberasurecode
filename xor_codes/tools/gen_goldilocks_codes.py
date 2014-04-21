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


def list_to_bm(list):
    bm = 0
    for elm in list:
        bm = bm | (1 << elm)
    return bm


def bm_to_list(bm):
    list = []
    tmp_bm = bm
    count = 0
    while tmp_bm != 0:
        if tmp_bm & 1:
            list.append(count)
        count = count + 1
        tmp_bm = tmp_bm >> 1
    return list


def list_of_list_to_bm_list(list):
    bm_list = []
    for elm in list:
        bm_list.append(list_to_bm(elm))

    return bm_list


def bm_list_to_list_of_list(bm_list):
    list_of_list = []
    for bm in bm_list:
        list_of_list.append(bm_to_list(bm))

    return list_of_list


def bm_in(elm, bm):
    if bm & (1 << elm):
        return True
    else:
        return False


def bm(elm):
    return (1 << elm)


def bm_insert(bm, elm):
    bm = bm | (1 << elm)
    return bm


def bm_rm(bm, elm):
    if bm_in(elm, bm):
        bm = bm ^ (1 << elm)
    return bm


def bm_is_subset(bm1, bm2):
    if (bm1 & bm2) ^ bm1 == 0:
        return True
    else:
        return False


def bm_intersection(bm1, bm2):
    return bm1 & bm2


def bm_hw(bm):
    hw = 0
    if bm == 0:
        return hw

    while bm != 0:
        if (bm & 1) == 1:
            hw += 1
        bm = bm >> 1
    return hw


def create_full_bm(num_bits):
    bm = (1 << num_bits) - 1
    return bm


def find_first(list, find_func):
    for i in range(len(list)):
        if find_func(list[i]):
            elm = list.pop(i)
            return elm

    return None


def get_num_data_in_parity(parity_bm_list):
    num_data_in_parity = [0 for i in range(m)]

    for parity_bm in parity_bm_list:
        for i in range(len(num_data_in_parity)):
            if bm_in(i, parity_bm):
                num_data_in_parity[i] += 1

    return num_data_in_parity


def get_parity_eqns(bm_parity_combinations):
    parity_bms = [0 for i in range(m)]
    data_num = 0
    for parity_comb in bm_parity_combinations:
        for i in range(m):
            if bm_in(i, parity_comb):
                parity_bms[i] = bm_insert(parity_bms[i], data_num)
        data_num += 1

    return parity_bms

if len(sys.argv) != 3:
    print("Usage: %s <num_parities> <num_bits = 2|3>")
    sys.exit(1)

m = int(sys.argv[1])
num_bits = int(sys.argv[2])

if m is None or num_bits not in [2, 3]:
    print("Usage: %s <num_parities> <num_bits = 2|3>")
    sys.exit(1)

parity_list = [i for i in range(m)]
parity_combinations = get_combinations(parity_list, num_bits)
bm_parity_combinations = list_of_list_to_bm_list(parity_combinations)

needed_parity_bm = create_full_bm(m)

parity_bms_template = "int g_%d_%d_%d_hd_code_parity_bms[] = { %s };"
data_bms_template = "int g_%d_%d_%d_hd_code_data_bms[] = { %s };"

k = mpmath.binomial(m, num_bits)
used_parities = []

while k > m:
    if bm_hw(needed_parity_bm) >= num_bits:
        # If HW >= num_bits, find any such that (elm & needed_parity_bm) == elm
        parity_bm = find_first(
            bm_parity_combinations,
            lambda elm: (
                elm & needed_parity_bm) == elm)
        if parity_bm is not None:
            needed_parity_bm ^= parity_bm
    else:
        # If HW < num_bits, find any such that (elm & needed_parity_bm) ==
        # needed_parity_bm
        parity_bm = find_first(
            bm_parity_combinations,
            lambda elm: (
                elm & needed_parity_bm) == needed_parity_bm)
        if parity_bm is not None:
            needed_parity_bm = (
                needed_parity_bm ^ parity_bm) ^ create_full_bm(m)

    # If we cannot find one, pop the last chosen element and try another
    full_bm = create_full_bm(m)
    if parity_bm is None:
        parity_to_re_add = used_parities.pop()
        # Added at the end, so we are changing the order
        bm_parity_combinations.append(parity_to_re_add)
        needed_parity_bm |= parity_to_re_add
        k += 1
    else:
        used_parities.append(parity_bm)
        k -= 1

for parity_bm in used_parities:
    num_data_in_parity = get_num_data_in_parity(bm_parity_combinations)
    parity_eqns = get_parity_eqns(bm_parity_combinations)
    # print "(%d, %d) : %s : %s : %s" % (k, m, bm_parity_combinations,
    # parity_eqns, num_data_in_parity)
    print(parity_bms_template %
          (k, m, num_bits + 1,
           ("%s" % parity_eqns).replace("[", "").replace("]", "")))
    print(data_bms_template %
          (k, m, num_bits + 1,
           ("%s" % bm_parity_combinations).replace("[", "").replace("]", "")))

    bm_parity_combinations.append(parity_bm)
    k += 1

num_data_in_parity = get_num_data_in_parity(bm_parity_combinations)
parity_eqns = get_parity_eqns(bm_parity_combinations)
# print"(%d, %d) : %s : %s" % (k, m, bm_parity_combinations,
# num_data_in_parity)
print(parity_bms_template %
      (k, m, num_bits + 1,
       ("%s" % parity_eqns).replace("[", "").replace("]", "")))
print(data_bms_template %
      (k, m, num_bits + 1,
       ("%s" % bm_parity_combinations).replace("[", "").replace("]", "")))
