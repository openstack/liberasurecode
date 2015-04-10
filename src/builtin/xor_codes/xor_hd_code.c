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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xor_code.h"
#include "xor_hd_code_defs.h"

/*
 * Returns -1 if not possible
 */
static int fragments_needed_one_data(xor_code_t *code_desc, int *missing_data, int *missing_parity, unsigned int *data_bm, unsigned int *parity_bm)
{
  int data_index = missing_data[0];
  int parity_index = index_of_connected_parity(code_desc, data_index, missing_parity, missing_data);

  if (parity_index < 0) {
    return -1;
  }

  // Include all data elements except for this one
  *data_bm |= (code_desc->parity_bms[parity_index-code_desc->k]);

  // Include this parity element
  *parity_bm |= (1 << (parity_index-code_desc->k));
  *data_bm &= ~((unsigned int)1 << data_index);

  return 0;
}

/*
 * Returns -1 if not possible
 */
static int fragments_needed_two_data(xor_code_t *code_desc, int *missing_data, int *missing_parity, unsigned int *data_bm, unsigned int *parity_bm)
{
  // Verify that missing_data[2] == -1?
  int data_index = missing_data[0];
  int parity_index = index_of_connected_parity(code_desc, data_index, missing_parity, missing_data);
  int ret;

  if (parity_index < 0) {
    data_index = missing_data[1];
    parity_index = index_of_connected_parity(code_desc, data_index, missing_parity, missing_data);
    if (parity_index < 0) {
      return -1;
    }
    missing_data[1] = -1;
  } else {
    missing_data[0] = missing_data[1];
    missing_data[1] = -1;
  }

  // Include all data elements except for this one
  *data_bm |= (code_desc->parity_bms[parity_index-code_desc->k]);

  // Include this parity element
  *parity_bm |= (1 << (parity_index-code_desc->k));

  ret = fragments_needed_one_data(code_desc, missing_data, missing_parity, data_bm, parity_bm);

  *data_bm &= ~((unsigned int)1 << data_index);
  
  return ret;
}

/*
 * Returns -1 if not possible
 */
static int fragments_needed_three_data(xor_code_t *code_desc, int *missing_data, int *missing_parity, unsigned int *data_bm, unsigned int *parity_bm)
{
  int i = 0;
  int parity_index = -1;
  int data_index = -1;
  int tmp_parity_bm = -1;
  int contains_2d = -1;
  int contains_3d = -1;
  int ret = 0;

  /*
   * Try to find a parity that only contains
   * one of the missing data elements.
   */
  while (missing_data[i] > -1) {
    parity_index = index_of_connected_parity(code_desc, missing_data[i], missing_parity, missing_data);
    if (parity_index > -1) {
      data_index = missing_data[i];
      tmp_parity_bm = code_desc->parity_bms[parity_index-code_desc->k];
      break;
    }
    i++;
  }
  /*
   * If we cannot find a parity that is connected to only
   * one missing element, we must find a parity that is
   * connected to exactly 2 (P) and another that is connected
   * to exactly 3 (Q) (it should exist!!!).
   *
   * We XOR those parities together and use it to recover
   * the element that is not connected to P.
   */
  if (parity_index < 0) {

    for (i=0;i < code_desc->m;i++) {
      int num_missing = num_missing_data_in_parity(code_desc, code_desc->k+i, missing_data);
      if (num_missing == 2 && contains_2d < 0) {
        contains_2d = i;
      } else if (num_missing == 3 && contains_3d < 0) {
        contains_3d = i;
      }
    }

    if (contains_2d < 0 || contains_3d < 0) {
      return -1;
    }

    // P XOR Q
    tmp_parity_bm = code_desc->parity_bms[contains_2d] ^ code_desc->parity_bms[contains_3d];

    i=0;
    data_index = -1;
    while (missing_data[i] > -1) {
      if (is_data_in_parity(missing_data[i], tmp_parity_bm)) {
        data_index = missing_data[i];
        break;
      }
      i++;
    }

    if (data_index < 0) {
      return -1;
    }
  }

  remove_from_missing_list(data_index, missing_data);

  // Include all data elements except for this one
  *data_bm |= (code_desc->parity_bms[parity_index-code_desc->k]);

  // Include this parity element
  if (parity_index > -1) {
    *parity_bm |= (1 << (parity_index-code_desc->k));
  } else {
    *parity_bm |= (1 << (contains_2d-code_desc->k));
    *parity_bm |= (1 << (contains_3d-code_desc->k));
  }

  ret = fragments_needed_two_data(code_desc, missing_data, missing_parity, data_bm, parity_bm);
  
  *data_bm &= ~((unsigned int)1 << data_index);

  return ret;
}

static int fragments_needed_one_data_local(xor_code_t *code_desc, 
                                           int fragment_to_reconstruct,
                                           int *fragments_to_exclude,
                                           unsigned int *data_bm, 
                                           unsigned int *parity_bm)
{
  int *missing_data = get_missing_data(code_desc, fragments_to_exclude);
  int *missing_parity = get_missing_parity(code_desc, fragments_to_exclude);
  int parity_index = index_of_connected_parity(code_desc, fragment_to_reconstruct, missing_parity, missing_data);
  free(missing_data);
  free(missing_parity);

  if (parity_index < 0) {
    return -1;
  }

  // Include all data elements except for this one
  *data_bm |= (code_desc->parity_bms[parity_index-code_desc->k]);

  // Include this parity element
  *parity_bm |= (1 << (parity_index-code_desc->k));
  *data_bm &= ~((unsigned int)1 << fragment_to_reconstruct);

  return 0;
}

int xor_hd_fragments_needed(xor_code_t *code_desc, int *fragments_to_reconstruct, int *fragments_to_exclude, int *fragments_needed)
{
  failure_pattern_t pattern = get_failure_pattern(code_desc, fragments_to_reconstruct);
  unsigned int data_bm = 0, parity_bm = 0;
  int ret = -1;
  int *missing_idxs = NULL;
  int i, j;

  /**
   * Re-visit this decision (KMG): This is non-optimal, but good enough in most cases.
   * If there is a single data item to reconstruct, then try to find a connected parity 
   * with no items in fragments_to_exclude.  If there is a single parity item to reconsturct
   * or more than 1 data/parity element missing, then just work fragments_to_exclude into
   * missing_idxs.
   */

  if (pattern == FAIL_PATTERN_1D_0P) {
    // Since we have landed on this failure pattern, fragments_to_reconstruct[0] is defined.
    ret = fragments_needed_one_data_local(code_desc, fragments_to_reconstruct[0], fragments_to_exclude, &data_bm, &parity_bm);
  } 

  /**
   * There is either more than one failed element, a failed parity element or 
   * we were unable to return the fragments needed for a simple reconstruction.
   */
  if (ret == -1) {
    /**
     * Add everything to missing_idxs (basically, give up on optimizing).
     */
    missing_idxs = (int*)malloc(sizeof(int)*(code_desc->k + code_desc->m));
    if (NULL == missing_idxs) {
      ret = -1;
      goto out;
    }
    
    i = 0;
    j = 0;
    while (fragments_to_reconstruct[i] > -1) {
      missing_idxs[j] = fragments_to_reconstruct[i];
      i++;
      j++;
    }
    i = 0;
    while (fragments_to_exclude[i] > -1) {
      missing_idxs[j] = fragments_to_exclude[i];
      i++;
      j++;
    }
    // End of list
    missing_idxs[j] = -1;
    
    pattern = get_failure_pattern(code_desc, missing_idxs); 

	  switch(pattern) {
	    case FAIL_PATTERN_0D_0P:
	      break;
	    case FAIL_PATTERN_1D_0P:
	    {
	      int *missing_data = get_missing_data(code_desc, missing_idxs);
	      ret = fragments_needed_one_data(code_desc, missing_data, NULL, &data_bm, &parity_bm);
	      free(missing_data);
	      break;
	    }
	    case FAIL_PATTERN_2D_0P:
	    {
	      int *missing_data = get_missing_data(code_desc, missing_idxs);
	      ret = fragments_needed_two_data(code_desc, missing_data, NULL, &data_bm, &parity_bm);
	      free(missing_data);
	      break;
	    }
	    case FAIL_PATTERN_3D_0P:
	    {
	      int *missing_data = get_missing_data(code_desc, missing_idxs);
	      ret = fragments_needed_three_data(code_desc, missing_data, NULL, &data_bm, &parity_bm);
	      free(missing_data);
	      break;
	    }
	    case FAIL_PATTERN_1D_1P:
	    {
	      int *missing_data = get_missing_data(code_desc, missing_idxs);
	      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
	      unsigned int missing_data_bm = missing_elements_bm(code_desc, missing_data, data_bit_lookup);
	      ret = fragments_needed_one_data(code_desc, missing_data, missing_parity, &data_bm, &parity_bm);
	      // OR all parities
	      i=0;
	      while (missing_parity[i] > -1) {
	        data_bm |= code_desc->parity_bms[missing_parity[i]-code_desc->k];
	        data_bm &= ~(missing_data_bm);
	        i++;
	      }
	      free(missing_parity);
	      free(missing_data);
	      break;
	    }
	    case FAIL_PATTERN_1D_2P:
	    {
	      int *missing_data = get_missing_data(code_desc, missing_idxs);
	      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
	      int missing_data_bm = missing_elements_bm(code_desc, missing_data, data_bit_lookup);
	      ret = fragments_needed_one_data(code_desc, missing_data, missing_parity, &data_bm, &parity_bm);
	      // OR all parities
	      i=0;
	      while (missing_parity[i] > -1) {
	        data_bm |= code_desc->parity_bms[missing_parity[i]-code_desc->k];
	        data_bm &= ~(missing_data_bm);
	        i++;
	      }
	      free(missing_parity);
	      free(missing_data);
	      break;
	    }
	    case FAIL_PATTERN_2D_1P:
	    {
	      int *missing_data = get_missing_data(code_desc, missing_idxs);
	      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
	      unsigned int missing_data_bm = missing_elements_bm(code_desc, missing_data, data_bit_lookup);
	      ret = fragments_needed_two_data(code_desc, missing_data, missing_parity, &data_bm, &parity_bm);
	      // OR all parities
	      i=0;
	      while (missing_parity[i] > -1) {
	        data_bm |= code_desc->parity_bms[missing_parity[i]-code_desc->k];
	        data_bm &= ~(missing_data_bm);
	        i++;
	      }
	      free(missing_parity);
	      free(missing_data);
	      break;
	    }
	    case FAIL_PATTERN_0D_1P:
	    { 
	      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
	      // OR all of the parities
	      i=0;
	      while (missing_parity[i] > -1) {
	        data_bm |= code_desc->parity_bms[missing_parity[i]-code_desc->k];
	        i++;
	      }
	      free(missing_parity);
        ret = 0;
	      break;
	    }
	    case FAIL_PATTERN_0D_2P:
	    {
	      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
	      // OR all of the parities
	      i=0;
	      while (missing_parity[i] > -1) {
	        data_bm |= code_desc->parity_bms[missing_parity[i]-code_desc->k];
	        i++;
	      }
	      free(missing_parity);
        ret = 0;
	      break;
	    }
	    case FAIL_PATTERN_0D_3P:
	    {
	      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
	      // OR all of the parities
	      i=0;
	      while (missing_parity[i] > -1) {
	        data_bm |= code_desc->parity_bms[missing_parity[i]-code_desc->k];
	        i++;
	      }
	      free(missing_parity);
        ret = 0;
	      break;
	    }
	    case FAIL_PATTERN_GE_HD:
	    default:
	      break;
	  }
  }

  if (ret >= 0) {
	  i=0;
	  j=0;
	  while (data_bm) {
	    if (data_bm & 1) {
	      fragments_needed[j] = i;
	      j++;
	    }
	    i++;
	    data_bm >>= 1;
	  }
	
	  i=0;
	  while (parity_bm) {
	    if (parity_bm & 1) {
	      fragments_needed[j] = i + code_desc->k;
	      j++;
	    }
	    i++;
	    parity_bm >>= 1;
	  }
	
	  fragments_needed[j] = -1;
  }

out:
  if (NULL != missing_idxs) {
    free(missing_idxs);
  }

  return ret;
}

/*
 * There is one unavailable data element, so any available parity connected to
 * the data element is sufficient to decode.
 */
static void decode_one_data(xor_code_t *code_desc, char **data, char **parity, int *missing_data, int *missing_parity, int blocksize)
{
  // Verify that missing_data[1] == -1? 
  int data_index = missing_data[0];
  int parity_index = index_of_connected_parity(code_desc, data_index, missing_parity, missing_data);
  int i;

  // Copy the appropriate parity into the data buffer
  fast_memcpy(data[data_index], parity[parity_index-code_desc->k], blocksize);

  for (i=0; i < code_desc->k; i++) {
    if (i != data_index && is_data_in_parity(i, code_desc->parity_bms[parity_index-code_desc->k])) {
      xor_bufs_and_store(data[i], data[data_index], blocksize);
    }
  }
}

static int decode_two_data(xor_code_t *code_desc, char **data, char **parity, int *missing_data, int *missing_parity, int blocksize)
{
  // Verify that missing_data[2] == -1?
  int data_index = missing_data[0];
  int parity_index = index_of_connected_parity(code_desc, data_index, missing_parity, missing_data);
  int i;
  
  if (parity_index < 0) {
    data_index = missing_data[1];
    parity_index = index_of_connected_parity(code_desc, data_index, missing_parity, missing_data);
    if (parity_index < 0) {
      fprintf(stderr, "Shit is broken, cannot find a proper parity!!!\n");
      return -2;
    }
    missing_data[1] = -1;
  } else {
    missing_data[0] = missing_data[1];
    missing_data[1] = -1;
  }
  
  // Copy the appropriate parity into the data buffer
  fast_memcpy(data[data_index], parity[parity_index-code_desc->k], blocksize);

  for (i=0; i < code_desc->k; i++) {
    if (i != data_index && is_data_in_parity(i, code_desc->parity_bms[parity_index-code_desc->k])) {
      xor_bufs_and_store(data[i], data[data_index], blocksize);
    }
  }
  decode_one_data(code_desc, data, parity, missing_data, missing_parity, blocksize);

  return 0;
}

static int decode_three_data(xor_code_t *code_desc, char **data, char **parity, int *missing_data, int *missing_parity, int blocksize)
{
  int i = 0;
  int parity_index = -1;
  int data_index = -1;
  unsigned int parity_bm = -1;
  char *parity_buffer = NULL;

  /*
   * Try to find a parity that only contains 
   * one of the missing data elements.
   */
  while (missing_data[i] > -1) {
    parity_index = index_of_connected_parity(code_desc, missing_data[i], missing_parity, missing_data);  
    if (parity_index > -1) {
      data_index = missing_data[i];
      parity_buffer = parity[parity_index-code_desc->k];
      parity_bm = code_desc->parity_bms[parity_index-code_desc->k];
      break;
    }
    i++;
  }

  /*
   * If we cannot find a parity that is connected to only
   * one missing element, we must find a parity that is
   * connected to exactly 2 (P) and another that is connected 
   * to exactly 3 (Q) (it should exist!!!).
   * 
   * We XOR those parities together and use it to recover
   * the element that is not connected to P.
   */
  if (parity_index < 0) {
    int contains_2d = -1; 
    int contains_3d = -1; 

    for (i=0;i < code_desc->m;i++) {
      int num_missing = num_missing_data_in_parity(code_desc, code_desc->k+i, missing_data);
      if (num_missing == 2 && contains_2d < 0) {
        contains_2d = i;
      } else if (num_missing == 3 && contains_3d < 0) {
        contains_3d = i;
      }
    }

    if (contains_2d < 0 || contains_3d < 0) {
      fprintf(stderr, "Shit is broken, cannot find a proper parity (2 and 3-connected parities)!!!\n");
      return -2;
    }

    if (posix_memalign((void **) &parity_buffer, 16, blocksize) != 0) {
      fprintf(stderr, "Can't get aligned memory!\n");
      return -1;
    }

    // P XOR Q
    parity_bm = code_desc->parity_bms[contains_2d] ^ code_desc->parity_bms[contains_3d];

    // Create buffer with P XOR Q -> parity_buffer
    fast_memcpy(parity_buffer, parity[contains_2d], blocksize);
    xor_bufs_and_store(parity[contains_3d], parity_buffer, blocksize);

    i=0;
    data_index = -1;
    while (missing_data[i] > -1) {
      if (is_data_in_parity(missing_data[i], parity_bm)) {
        data_index = missing_data[i];
        break;
      }
      i++;
    }

    if (data_index < 0) {
     fprintf(stderr, "Shit is broken, cannot construct equations to repair 3 failures!!!\n");
      return -2;
    }
    // Copy the appropriate parity into the data buffer
    fast_memcpy(data[data_index], parity_buffer, blocksize);
    // Free up the buffer we allocated above
    free(parity_buffer);
  } else {
    // Copy the appropriate parity into the data buffer
    fast_memcpy(data[data_index], parity_buffer, blocksize);
  }

  
  for (i=0; i < code_desc->k; i++) {
    if (i != data_index && is_data_in_parity(i, parity_bm)) {
      xor_bufs_and_store(data[i], data[data_index], blocksize);
    }
  }

  remove_from_missing_list(data_index, missing_data);

  return decode_two_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
}

int xor_hd_decode(xor_code_t *code_desc, char **data, char **parity, int *missing_idxs, int blocksize, int decode_parity)
{
  int ret = 0;
  failure_pattern_t pattern = get_failure_pattern(code_desc, missing_idxs);

  switch(pattern) {
    case FAIL_PATTERN_0D_0P: 
      break;
    case FAIL_PATTERN_1D_0P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      decode_one_data(code_desc, data, parity, missing_data, NULL, blocksize);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_2D_0P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      ret = decode_two_data(code_desc, data, parity, missing_data, NULL, blocksize);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_3D_0P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      ret = decode_three_data(code_desc, data, parity, missing_data, NULL, blocksize);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_1D_1P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
      decode_one_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
      if (decode_parity) {
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
      }
      free(missing_parity);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_1D_2P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
      decode_one_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
      if (decode_parity) {
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
      }
      free(missing_data);
      free(missing_parity);
      break;
    }
    case FAIL_PATTERN_2D_1P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
      ret = decode_two_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
      if (decode_parity) {
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
      }
      free(missing_parity);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_0D_1P: 
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      break;
    case FAIL_PATTERN_0D_2P: 
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      break;
    case FAIL_PATTERN_0D_3P:
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      break;
    case FAIL_PATTERN_GE_HD: 
    default:
      break;
  }

  return ret;
}

xor_code_t* init_xor_hd_code(int k, int m, int hd)
{
  xor_code_t *code_desc = NULL;
  int is_valid = 0;

  if (hd == 3) {
    if (m == 6) {
      if (k <= 15 && k >= 6) {
        is_valid = 1;
      }
    } else if (m == 5) {
      if (k <= 10 && k >= 5) {
        is_valid = 1;
      }
    } else if (m == 3) {
      is_valid = 1;
    }
  }
  
  if (hd == 4) {
    if (m == 6) {
      if (k <= 20 && k >= 6) {
        is_valid = 1;
      }
    } else if (m == 5) {
      if (k <= 10 && k >= 5) {
        is_valid = 1;
      }
    }
  }

  if (is_valid) {
    code_desc = (xor_code_t*)malloc(sizeof(xor_code_t));
    code_desc->parity_bms = PARITY_BM_ARY(k, m, hd);
    code_desc->data_bms = DATA_BM_ARY(k, m, hd);
    code_desc->k = k;
    code_desc->m = m;
    code_desc->hd = hd;
    code_desc->decode = xor_hd_decode;
    code_desc->encode = xor_code_encode;
    code_desc->fragments_needed = xor_hd_fragments_needed;
  }

  return code_desc;
}

