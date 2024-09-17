/* * Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
 * All rights reserved.
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
 */

#ifndef _XOR_HD_CODE_DEFS_H
#define _XOR_HD_CODE_DEFS_H



// I made these by hand...
static unsigned int g_12_6_4_hd_code_parity_bms[] = { 1649, 3235, 2375, 718, 1436, 2872 };
static unsigned int g_12_6_4_hd_code_data_bms[] = { 7, 14, 28, 56, 49, 35, 13, 26, 52, 41, 19, 38 };

static unsigned int g_10_5_3_hd_code_parity_bms[] = { 163, 300, 337, 582, 664 };
static unsigned int g_10_5_3_hd_code_data_bms[] = { 5, 9, 10, 18, 20, 3, 12, 17, 6, 24 };

static unsigned int g_3_3_3_hd_code_parity_bms[] = { 5, 6, 3 };
static unsigned int g_3_3_3_hd_code_data_bms[] = { 5, 6, 3};


// The rest were generated via the "goldilocks" code algorithm
static unsigned int g_6_6_3_hd_code_parity_bms[] = { 3, 48, 36, 24, 9, 6 };
static unsigned int g_6_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6 };
static unsigned int g_7_6_3_hd_code_parity_bms[] = { 67, 112, 36, 24, 9, 6 };
static unsigned int g_7_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3 };
static unsigned int g_8_6_3_hd_code_parity_bms[] = { 67, 112, 164, 152, 9, 6 };
static unsigned int g_8_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12 };
static unsigned int g_9_6_3_hd_code_parity_bms[] = { 67, 112, 164, 152, 265, 262 };
static unsigned int g_9_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48 };
static unsigned int g_10_6_3_hd_code_parity_bms[] = { 579, 112, 676, 152, 265, 262 };
static unsigned int g_10_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48, 5 };
static unsigned int g_11_6_3_hd_code_parity_bms[] = { 579, 1136, 676, 152, 1289, 262 };
static unsigned int g_11_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48, 5, 18 };
static unsigned int g_12_6_3_hd_code_parity_bms[] = { 579, 1136, 676, 2200, 1289, 2310 };
static unsigned int g_12_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48, 5, 18, 40 };
static unsigned int g_13_6_3_hd_code_parity_bms[] = { 4675, 1136, 676, 6296, 1289, 2310 };
static unsigned int g_13_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48, 5, 18, 40, 9 };
static unsigned int g_14_6_3_hd_code_parity_bms[] = { 4675, 9328, 676, 6296, 1289, 10502 };
static unsigned int g_14_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48, 5, 18, 40, 9, 34 };
static unsigned int g_15_6_3_hd_code_parity_bms[] = { 4675, 9328, 17060, 6296, 17673, 10502 };
static unsigned int g_15_6_3_hd_code_data_bms[] = { 17, 33, 36, 24, 10, 6, 3, 12, 48, 5, 18, 40, 9, 34, 20 };

static unsigned int g_6_6_4_hd_code_parity_bms[] = { 7, 56, 56, 11, 21, 38 };
static unsigned int g_6_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38 };
static unsigned int g_7_6_4_hd_code_parity_bms[] = { 71, 120, 120, 11, 21, 38 };
static unsigned int g_7_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7 };
static unsigned int g_8_6_4_hd_code_parity_bms[] = { 71, 120, 120, 139, 149, 166 };
static unsigned int g_8_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56 };
static unsigned int g_9_6_4_hd_code_parity_bms[] = { 327, 376, 120, 395, 149, 166 };
static unsigned int g_9_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11 };
static unsigned int g_10_6_4_hd_code_parity_bms[] = { 327, 376, 632, 395, 661, 678 };
static unsigned int g_10_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52 };
static unsigned int g_11_6_4_hd_code_parity_bms[] = { 1351, 1400, 632, 395, 1685, 678 };
static unsigned int g_11_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19 };
static unsigned int g_13_6_4_hd_code_parity_bms[] = { 5447, 5496, 2680, 2443, 1685, 6822 };
static unsigned int g_13_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35 };
static unsigned int g_14_6_4_hd_code_parity_bms[] = { 5447, 5496, 10872, 10635, 9877, 6822 };
static unsigned int g_14_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28 };
static unsigned int g_15_6_4_hd_code_parity_bms[] = { 21831, 5496, 27256, 27019, 9877, 6822 };
static unsigned int g_15_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28, 13 };
static unsigned int g_16_6_4_hd_code_parity_bms[] = { 21831, 38264, 27256, 27019, 42645, 39590 };
static unsigned int g_16_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28, 13, 50 };
static unsigned int g_17_6_4_hd_code_parity_bms[] = { 87367, 38264, 92792, 27019, 108181, 39590 };
static unsigned int g_17_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28, 13, 50, 21 };
static unsigned int g_18_6_4_hd_code_parity_bms[] = { 87367, 169336, 92792, 158091, 108181, 170662 };
static unsigned int g_18_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28, 13, 50, 21, 42 };
static unsigned int g_19_6_4_hd_code_parity_bms[] = { 349511, 169336, 354936, 158091, 108181, 432806 };
static unsigned int g_19_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28, 13, 50, 21, 42, 37 };
static unsigned int g_20_6_4_hd_code_parity_bms[] = { 349511, 693624, 354936, 682379, 632469, 432806 };
static unsigned int g_20_6_4_hd_code_data_bms[] = { 25, 41, 49, 14, 22, 38, 7, 56, 11, 52, 19, 44, 35, 28, 13, 50, 21, 42, 37, 26 };


static unsigned int g_5_5_3_hd_code_parity_bms[] = { 3, 12, 17, 6, 24 };
static unsigned int g_5_5_3_hd_code_data_bms[] = { 5, 9, 10, 18, 20 };
static unsigned int g_6_5_3_hd_code_parity_bms[] = { 35, 44, 17, 6, 24 };
static unsigned int g_6_5_3_hd_code_data_bms[] = { 5, 9, 10, 18, 20, 3 };
static unsigned int g_7_5_3_hd_code_parity_bms[] = { 35, 44, 81, 70, 24 };
static unsigned int g_7_5_3_hd_code_data_bms[] = { 5, 9, 10, 18, 20, 3, 12 };
static unsigned int g_8_5_3_hd_code_parity_bms[] = { 163, 44, 81, 70, 152 };
static unsigned int g_8_5_3_hd_code_data_bms[] = { 5, 9, 10, 18, 20, 3, 12, 17 };
static unsigned int g_9_5_3_hd_code_parity_bms[] = { 163, 300, 337, 70, 152 };
static unsigned int g_9_5_3_hd_code_data_bms[] = { 5, 9, 10, 18, 20, 3, 12, 17, 6 };

static unsigned int g_5_5_4_hd_code_parity_bms[] = { 7, 25, 14, 19, 28 };
static unsigned int g_5_5_4_hd_code_data_bms[] = { 11, 13, 21, 22, 26 };
static unsigned int g_6_5_4_hd_code_parity_bms[] = { 39, 57, 46, 19, 28 };
static unsigned int g_6_5_4_hd_code_data_bms[] = { 11, 13, 21, 22, 26, 7 };
static unsigned int g_7_5_4_hd_code_parity_bms[] = { 103, 57, 46, 83, 92 };
static unsigned int g_7_5_4_hd_code_data_bms[] = { 11, 13, 21, 22, 26, 7, 25 };
static unsigned int g_8_5_4_hd_code_parity_bms[] = { 103, 185, 174, 211, 92 };
static unsigned int g_8_5_4_hd_code_data_bms[] = { 11, 13, 21, 22, 26, 7, 25, 14 };
static unsigned int g_9_5_4_hd_code_parity_bms[] = { 359, 441, 174, 211, 348 };
static unsigned int g_9_5_4_hd_code_data_bms[] = { 11, 13, 21, 22, 26, 7, 25, 14, 19 };
static unsigned int g_10_5_4_hd_code_parity_bms[] = { 359, 441, 686, 723, 860 };
static unsigned int g_10_5_4_hd_code_data_bms[] = { 11, 13, 21, 22, 26, 7, 25, 14, 19, 28 };

// Indexed by k
static unsigned int * hd4_m5_parity[11] = { 0, 0, 0, 0, 0, g_5_5_4_hd_code_parity_bms, g_6_5_4_hd_code_parity_bms, g_7_5_4_hd_code_parity_bms, g_8_5_4_hd_code_parity_bms, g_9_5_4_hd_code_parity_bms, g_10_5_4_hd_code_parity_bms };
static unsigned int * hd4_m5_data[11] = { 0, 0, 0, 0, 0, g_5_5_4_hd_code_data_bms, g_6_5_4_hd_code_data_bms, g_7_5_4_hd_code_data_bms, g_8_5_4_hd_code_data_bms, g_9_5_4_hd_code_data_bms, g_10_5_4_hd_code_data_bms };
static unsigned int * hd4_m6_parity[21] = { 0, 0, 0, 0, 0, 0, g_6_6_4_hd_code_parity_bms, g_7_6_4_hd_code_parity_bms, g_8_6_4_hd_code_parity_bms, g_9_6_4_hd_code_parity_bms, g_10_6_4_hd_code_parity_bms, g_11_6_4_hd_code_parity_bms, g_12_6_4_hd_code_parity_bms, g_13_6_4_hd_code_parity_bms, g_14_6_4_hd_code_parity_bms, g_15_6_4_hd_code_parity_bms, g_16_6_4_hd_code_parity_bms, g_17_6_4_hd_code_parity_bms, g_18_6_4_hd_code_parity_bms, g_19_6_4_hd_code_parity_bms, g_20_6_4_hd_code_parity_bms };

static unsigned int * hd4_m6_data[21] = { 0, 0, 0, 0, 0, 0, g_6_6_4_hd_code_data_bms, g_7_6_4_hd_code_data_bms, g_8_6_4_hd_code_data_bms, g_9_6_4_hd_code_data_bms, g_10_6_4_hd_code_data_bms, g_11_6_4_hd_code_data_bms, g_12_6_4_hd_code_data_bms, g_13_6_4_hd_code_data_bms, g_14_6_4_hd_code_data_bms, g_15_6_4_hd_code_data_bms, g_16_6_4_hd_code_data_bms, g_17_6_4_hd_code_data_bms, g_18_6_4_hd_code_data_bms, g_19_6_4_hd_code_data_bms, g_20_6_4_hd_code_data_bms };

static unsigned int * hd3_m5_parity[11] = { 0, 0, 0, 0, 0, g_5_5_3_hd_code_parity_bms, g_6_5_3_hd_code_parity_bms, g_7_5_3_hd_code_parity_bms, g_8_5_3_hd_code_parity_bms, g_9_5_3_hd_code_parity_bms, g_10_5_3_hd_code_parity_bms };
static unsigned int * hd3_m5_data[11] = { 0, 0, 0, 0, 0, g_5_5_3_hd_code_data_bms, g_6_5_3_hd_code_data_bms, g_7_5_3_hd_code_data_bms, g_8_5_3_hd_code_data_bms, g_9_5_3_hd_code_data_bms, g_10_5_3_hd_code_data_bms };
static unsigned int * hd3_m6_parity[16] = { 0, 0, 0, 0, 0, 0, g_6_6_3_hd_code_parity_bms, g_7_6_3_hd_code_parity_bms, g_8_6_3_hd_code_parity_bms, g_9_6_3_hd_code_parity_bms, g_10_6_3_hd_code_parity_bms, g_11_6_3_hd_code_parity_bms, g_12_6_3_hd_code_parity_bms, g_13_6_3_hd_code_parity_bms, g_14_6_3_hd_code_parity_bms, g_15_6_3_hd_code_parity_bms };
static unsigned int * hd3_m6_data[16] = { 0, 0, 0, 0, 0, 0, g_6_6_3_hd_code_data_bms, g_7_6_3_hd_code_data_bms, g_8_6_3_hd_code_data_bms, g_9_6_3_hd_code_data_bms, g_10_6_3_hd_code_data_bms, g_11_6_3_hd_code_data_bms, g_12_6_3_hd_code_data_bms, g_13_6_3_hd_code_data_bms, g_14_6_3_hd_code_data_bms, g_15_6_3_hd_code_data_bms };

static unsigned int * hd3_m3_parity[4] = { 0, 0, 0, g_3_3_3_hd_code_parity_bms };
static unsigned int * hd3_m3_data[4] = { 0, 0, 0, g_3_3_3_hd_code_data_bms };

static unsigned int ** parity_bm_hd4 [7] = { 0, 0, 0, 0, 0, hd4_m5_parity, hd4_m6_parity };
static unsigned int ** data_bm_hd4 [7] = { 0, 0, 0, 0, 0, hd4_m5_data, hd4_m6_data };
static unsigned int ** parity_bm_hd3 [7] = { 0, 0, 0, hd3_m3_parity, 0, hd3_m5_parity, hd3_m6_parity };
static unsigned int ** data_bm_hd3 [7] = { 0, 0, 0, hd3_m3_data, 0, hd3_m5_data, hd3_m6_data };

#define PARITY_BM_ARY(k, m, hd) (hd == 3) ? parity_bm_hd3[m][k] : parity_bm_hd4[m][k]
#define DATA_BM_ARY(k, m, hd) (hd == 3) ? data_bm_hd3[m][k] : data_bm_hd4[m][k]

#endif
