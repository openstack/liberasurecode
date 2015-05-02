#
# Updated by KMG to support -DINTEL_SSE for GF-Complete
#
# ===========================================================================
#          http://www.gnu.org/software/autoconf-archive/ax_ext.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_EXT
#
# DESCRIPTION
#
#   Find supported SIMD extensions by requesting cpuid. When an SIMD
#   extension is found, the -m"simdextensionname" is added to SIMD_FLAGS if
#   compiler supports it. For example, if "sse2" is available, then "-msse2"
#   is added to SIMD_FLAGS.
#
#   This macro calls:
#
#     AC_SUBST(SIMD_FLAGS)
#
#   And defines:
#
#     HAVE_MMX / HAVE_SSE / HAVE_SSE2 / HAVE_SSE3 / HAVE_SSSE3 / HAVE_SSE4.1 / HAVE_SSE4.2 / HAVE_AVX
#
# LICENSE
#
#   Copyright (c) 2007 Christophe Tournayre <turn3r@users.sourceforge.net>
#   Copyright (c) 2013 Michael Petch <mpetch@capp-sysware.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 12

AC_DEFUN([AX_EXT],
[
  AC_REQUIRE([AC_CANONICAL_HOST])

  case $host_cpu in
    powerpc*)
      AC_CACHE_CHECK([whether altivec is supported], [ax_cv_have_altivec_ext],
          [
            if test `/usr/sbin/sysctl -a 2>/dev/null| grep -c hw.optional.altivec` != 0; then
                if test `/usr/sbin/sysctl -n hw.optional.altivec` = 1; then
                  ax_cv_have_altivec_ext=yes
                fi
            fi
          ])

          if test "$ax_cv_have_altivec_ext" = yes; then
            AC_DEFINE(HAVE_ALTIVEC,,[Support Altivec instructions])
            AX_CHECK_COMPILE_FLAG(-faltivec, SIMD_FLAGS="$SIMD_FLAGS -faltivec", [])
          fi
    ;;

    i[[3456]]86*|x86_64*|amd64*)

      AC_REQUIRE([AC_PROG_CC])
      AC_LANG_PUSH([C])

      # Note for the next 3 macros: m4 does not like '$' and '>', so 
      # we have to use the ascii values, 36 and 62, resp. to print the
      # characters out to the temporary shell script used to detect
      # CPUID settings
      
      AC_CACHE_VAL([ax_cv_feature_ecx], [
      AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [
        int eax, ebx, ecx, edx;
        FILE *f;
        __asm__("cpuid"
          : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
          : "a" (0x00000001));
        f = fopen("ax_cv_feature_ecx", "w");
        fprintf(f, "if (( (0x%x %c%c %c0) & 0x1 )); then echo 1; else echo 0; fi\n", ecx, 62, 62, 36);
        fclose(f);
        return 0;
        ])],
          #[ax_cv_feature_ecx=`cat ax_cv_feature_ecx`; rm -f ax_cv_feature_ecx],
          [ax_cv_feature_ecx=`cat ax_cv_feature_ecx`],
          [ax_cv_feature_ecx='echo 0'; rm -f ax_cv_feature_ecx],
          [ax_cv_feature_ecx='echo 0'])])
      
      AC_CACHE_VAL([ax_cv_feature_edx], [
      AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [
        int eax, ebx, ecx, edx;
        FILE *f;
        __asm__("cpuid"
          : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
          : "a" (0x00000001));
        f = fopen("ax_cv_feature_edx", "w");
        fprintf(f, "if (( (0x%x %c%c %c0) & 0x1 )); then echo 1; else echo 0; fi\n", edx, 62, 62, 36);
        fclose(f);
        return 0;
        ])], 
          [ax_cv_feature_edx=`cat ax_cv_feature_edx`; rm -f ax_cv_feature_edx],
          [ax_cv_feature_edx='echo 0'; rm -f ax_cv_feature_edx],
          [ax_cv_feature_edx='echo 0'])])
      
      AC_CACHE_VAL([ax_cv_vendor_ecx], [
      AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [
        int eax, ebx, ecx, edx;
        FILE *f;
        __asm__("cpuid"
          : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
          : "a" (0x00000000));
        f = fopen("ax_cv_vendor_ecx", "w");
        fprintf(f, "if (( (0x%x %c%c %c0) & 0x1)); then echo 1; else echo 0; fi\n", ecx, 62, 62, 36);
        fclose(f);
        return 0;
        ])], 
          [ax_cv_vendor_ecx=`cat ax_cv_vendor_ecx`; rm -f ax_cv_vendor_ecx],
          [ax_cv_vendor_ecx='echo 0'; rm -f ax_cv_vendor_ecx],
          [ax_cv_vendor_ecx='echo 0'])])

      AC_LANG_POP([C])

      AC_CACHE_CHECK([whether mmx is supported], [ax_cv_have_mmx_ext],
      [
        ax_cv_have_mmx_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_edx\" 23` ]]" = "1"; then
          ax_cv_have_mmx_ext=yes
        fi
      ])
      
      AC_CACHE_CHECK([whether sse is supported], [ax_cv_have_sse_ext],
      [
        ax_cv_have_sse_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_edx\" 25` ]]" = "1"; then
          ax_cv_have_sse_ext=yes
        fi
      ])
      
      AC_CACHE_CHECK([whether sse2 is supported], [ax_cv_have_sse2_ext],
      [
        ax_cv_have_sse2_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_edx\" 26` ]]" = "1"; then
          ax_cv_have_sse2_ext=yes
        fi
      ])
      
      AC_CACHE_CHECK([whether sse3 is supported], [ax_cv_have_sse3_ext],
      [
        ax_cv_have_sse3_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_ecx\" 0` ]]" = "1"; then
          ax_cv_have_sse3_ext=yes
        fi
      ])
      
      AC_CACHE_CHECK([whether ssse3 is supported], [ax_cv_have_ssse3_ext],
      [
        ax_cv_have_ssse3_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_ecx\" 9` ]]" = "1"; then
          ax_cv_have_ssse3_ext=yes
        fi
      ])
      
      AC_CACHE_CHECK([whether sse4.1 is supported], [ax_cv_have_sse41_ext],
      [
        ax_cv_have_sse41_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_ecx\" 19` ]]" = "1"; then
          ax_cv_have_sse41_ext=yes
        fi
      ])
      
      AC_CACHE_CHECK([whether sse4.2 is supported], [ax_cv_have_sse42_ext],
      [
        ax_cv_have_sse42_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_ecx\" 20` ]]" = "1"; then
          ax_cv_have_sse42_ext=yes
        fi
      ])

      AC_CACHE_CHECK([whether avx is supported by processor], [ax_cv_have_avx_cpu_ext],
      [
        ax_cv_have_avx_cpu_ext=no
        if test "$[[ `sh -c \"$ax_cv_feature_ecx\" 28` ]]" = "1"; then
          ax_cv_have_avx_cpu_ext=yes
        fi
      ])

      if test x"$ax_cv_have_avx_cpu_ext" = x"yes"; then
        AC_CACHE_CHECK([whether avx is supported by operating system], [ax_cv_have_avx_ext],
        [
          ax_cv_have_avx_ext=no

          if test "$[[ `sh -c \"$ax_cv_feature_ecx\" 27` ]]" = "1"; then
            if test "$[[ `sh -c \"$ax_cv_vendor_ecx\" 6` ]]" = "1"; then
              ax_cv_have_avx_ext=yes
            fi
          fi
        ])
        if test x"$ax_cv_have_avx_ext" = x"no"; then
          AC_MSG_WARN([Your processor supports AVX, but your operating system doesn't])
        fi
      fi

      if test "$ax_cv_have_mmx_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-mmmx, ax_cv_support_mmx_ext=yes, [])
        if test x"$ax_cv_support_mmx_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -mmmx"
          AC_DEFINE(HAVE_MMX,,[Support mmx instructions])
        else
          AC_MSG_WARN([Your processor supports mmx instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ac_cv_sizeof_long" -eq 8; then
          SIMD_FLAGS="$SIMD_FLAGS -DARCH_64"
      fi

      if test "$ax_cv_have_sse_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-msse, ax_cv_support_sse_ext=yes, [])
        if test x"$ax_cv_support_sse_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -msse -DINTEL_SSE"
          AC_DEFINE(HAVE_SSE,,[Support SSE (Streaming SIMD Extensions) instructions])
        else
          AC_MSG_WARN([Your processor supports sse instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ax_cv_have_sse2_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-msse2, ax_cv_support_sse2_ext=yes, [])
        if test x"$ax_cv_support_sse2_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -msse2 -DINTEL_SSE2"
          AC_DEFINE(HAVE_SSE2,,[Support SSE2 (Streaming SIMD Extensions 2) instructions])
        else
          AC_MSG_WARN([Your processor supports sse2 instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ax_cv_have_sse3_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-msse3, ax_cv_support_sse3_ext=yes, [])
        if test x"$ax_cv_support_sse3_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -msse3 -DINTEL_SSE3"
          AC_DEFINE(HAVE_SSE3,,[Support SSE3 (Streaming SIMD Extensions 3) instructions])
        else
          AC_MSG_WARN([Your processor supports sse3 instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ax_cv_have_ssse3_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-mssse3, ax_cv_support_ssse3_ext=yes, [])
        if test x"$ax_cv_support_ssse3_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -mssse3"
          AC_DEFINE(HAVE_SSSE3,,[Support SSSE3 (Supplemental Streaming SIMD Extensions 3) instructions])
        else
          AC_MSG_WARN([Your processor supports ssse3 instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ax_cv_have_sse41_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-msse4.1, ax_cv_support_sse41_ext=yes, [])
        if test x"$ax_cv_support_sse41_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -msse4.1 -DINTEL_SSE4"
          AC_DEFINE(HAVE_SSE4_1,,[Support SSSE4.1 (Streaming SIMD Extensions 4.1) instructions])
        else
          AC_MSG_WARN([Your processor supports sse4.1 instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ax_cv_have_sse42_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-msse4.2, ax_cv_support_sse42_ext=yes, [])
        if test x"$ax_cv_support_sse42_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -msse4.2 -DINTEL_SSE4"
          AC_DEFINE(HAVE_SSE4_2,,[Support SSSE4.2 (Streaming SIMD Extensions 4.2) instructions])
        else
          AC_MSG_WARN([Your processor supports sse4.2 instructions but not your compiler, can you try another compiler?])
        fi
      fi

      if test "$ax_cv_have_avx_ext" = yes; then
        AX_CHECK_COMPILE_FLAG(-mavx, ax_cv_support_avx_ext=yes, [])
        if test x"$ax_cv_support_avx_ext" = x"yes"; then
          SIMD_FLAGS="$SIMD_FLAGS -mavx"
          AC_DEFINE(HAVE_AVX,,[Support AVX (Advanced Vector Extensions) instructions])
        else
          AC_MSG_WARN([Your processor supports avx instructions but not your compiler, can you try another compiler?])
        fi
      fi

  ;;
  esac

  AC_SUBST(SIMD_FLAGS)
])
