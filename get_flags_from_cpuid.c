#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define EDX_MMX_BIT 23
#define EDX_SSE_BIT 25
#define EDX_SSE2_BIT 26
#define ECX_SSE3_BIT 0
#define ECX_SSSE3_BIT 9
#define ECX_SSE41_BIT 19
#define ECX_SSE42_BIT 20
#define ECX_AVXOS_BIT 27
#define ECX_AVX_BIT 28


uint64_t get_supported_flags()
{
  uint64_t supp_comp_flgs = 0;
  char *comp_flag = NULL;
  char SUPPORTED_COMP_FLAGS[] ="FLAGSFROMAUTOCONF\0";

  for (comp_flag = strtok(SUPPORTED_COMP_FLAGS, " "); comp_flag != NULL; comp_flag = strtok(NULL, " ")) {
    if (strncmp(comp_flag, "-m", 2) != 0) {
      fprintf(stderr, "Invalid comp_flag: %s\n", comp_flag);
      exit(2);
    }
    if (strcmp(comp_flag, "-mmmx\0") == 0) {
      supp_comp_flgs |= (1 << EDX_MMX_BIT);
    }
    if (strcmp(comp_flag, "-msse\0") == 0) {
      supp_comp_flgs |= (1 << EDX_SSE_BIT);
    }
    if (strcmp(comp_flag, "-msse2\0") == 0) {
      supp_comp_flgs |= (1 << EDX_SSE2_BIT);
    }
    if (strcmp(comp_flag, "-msse3\0") == 0) {
      supp_comp_flgs |= (1 << ECX_SSE3_BIT);
    }
    if (strcmp(comp_flag, "-mssse3\0") == 0) {
      supp_comp_flgs |= (1 << ECX_SSSE3_BIT);
    }
    if (strcmp(comp_flag, "-msse4.1\0") == 0) {
      supp_comp_flgs |= (1 << ECX_SSE41_BIT);
    }
    if (strcmp(comp_flag, "-msse4.2\0") == 0) {
      supp_comp_flgs |= (1 << ECX_SSE42_BIT);
    }
    if (strcmp(comp_flag, "-mavx\0") == 0) {
      supp_comp_flgs |= (1 << ECX_AVX_BIT);
    }
  }

  return supp_comp_flgs;
}

int is_supported(int cpuid_reg, uint64_t comp_flags, int feature_bit)
{
  return ((cpuid_reg >> feature_bit) & 0x1) && ((comp_flags >> feature_bit) & 0x1);
}

int main(int argc, char** argv)
{
  int feature_eax, feature_ebx, feature_ecx, feature_edx;
  uint64_t supp_comp_flgs = get_supported_flags();
  FILE *f = fopen("compiler_flags", "w");

  __asm__("cpuid"
    : "=a" (feature_eax), "=b" (feature_ebx), "=c" (feature_ecx), "=d" (feature_edx)
    : "a" (0x00000001));

  if (is_supported(feature_edx, supp_comp_flgs, EDX_MMX_BIT)) {
    fprintf(f, "-mmmx -DINTEL_MMX ");
  } 
  if (is_supported(feature_edx, supp_comp_flgs, EDX_SSE_BIT)) {
    fprintf(f, "-msse -DINTEL_SSE ");
  } 
  if (is_supported(feature_edx, supp_comp_flgs, EDX_SSE2_BIT)) {
    fprintf(f, "-msse2 -DINTEL_SSE2 ");
  } 
  if (is_supported(feature_ecx, supp_comp_flgs, ECX_SSE3_BIT)) {
    fprintf(f, "-msse3 -DINTEL_SSE3 ");
  } 
  if (is_supported(feature_ecx, supp_comp_flgs, ECX_SSSE3_BIT)) {
    fprintf(f, "-mssse3 -DINTEL_SSSE3 ");
  } 
  if (is_supported(feature_ecx, supp_comp_flgs, ECX_SSE41_BIT)) {
    fprintf(f, "-msse4.1 -DINTEL_SSE41 ");
  } 
  if (is_supported(feature_ecx, supp_comp_flgs, ECX_SSE42_BIT)) {
    fprintf(f, "-msse4.2 -DINTEL_SSE42 ");
  } 
  if (is_supported(feature_ecx, supp_comp_flgs, ECX_AVX_BIT)) {
    if ((feature_ecx >> ECX_AVXOS_BIT) & 0x1) {
      fprintf(f, "-mavx -DINTEL_AVX ");
    }
  }
  fclose(f);
  return 0;
}

