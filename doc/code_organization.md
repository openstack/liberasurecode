Code organization
=================
```
 |-- include
 |   +-- erasurecode
 |   |   +-- erasurecode.h                --> liberasurecode frontend API header
 |   |   +-- erasurecode_backend.h        --> liberasurecode backend API header
 |   +-- xor_codes                        --> headers for the built-in XOR codes
 |
 |-- src
 |   |-- erasurecode.c                    --> liberasurecode API implementation
 |   |                                        (frontend + backend)
 |   |-- backends
 |   |   +-- null
 |   |       +-- null.c                   --> 'null' erasure code backend (template backend)
 |   |   +-- xor
 |   |       +-- flat_xor_hd.c            --> 'flat_xor_hd' erasure code backend (built-in)
 |   |   +-- rs_vand
 |   |       +-- liberasurecode_rs_vand.c --> 'liberasurecode_rs_vand' erasure code backend (built-in)
 |   |   +-- jerasure
 |   |       +-- jerasure_rs_cauchy.c     --> 'jerasure_rs_vand' erasure code backend (jerasure.org)
 |   |       +-- jerasure_rs_vand.c       --> 'jerasure_rs_cauchy' erasure code backend (jerasure.org)
 |   |   +-- isa-l
 |   |       +-- isa_l_rs_vand.c          --> 'isa_l_rs_vand' erasure code backend (Intel)
 |   |       +-- isa_l_rs_vand_inv.c      --> 'isa_l_rs_vand_inv' erasure code backend (Intel)
 |   |       +-- isa_l_rs_cauchy.c        --> 'isa_l_rs_cauchy' erasure code backend (Intel)
 |   |   +-- shss
 |   |       +-- shss.c                   --> 'shss' erasure code backend (NTT Labs)
 |   |   +-- phazrio
 |   |       +-- libphazr.c               --> 'libphazr' erasure code backend (Phazr.IO)
 |   |
 |   |-- builtin
 |   |   +-- xor_codes                    --> XOR HD code backend, built-in erasure
 |   |       |                                code implementation (shared library)
 |   |       +-- xor_code.c
 |   |       +-- xor_hd_code.c
 |   |   +-- rs_vand                      --> liberasurecode native Reed Soloman codes
 |   |
 |   +-- utils
 |       +-- chksum                       --> fragment checksum utils for erasure
 |           +-- alg_sig.c                    coded fragments
 |           +-- crc32.c
 |
 |-- doc                                  --> API Documentation
 |   +-- Doxyfile
 |   +-- html
 |
 |-- test                                 --> Test routines
 |   +-- builtin
 |   |   +-- xor_codes
 |   +-- liberasurecode_test.c
 |   +-- utils
 |
 |-- autogen.sh
 |-- configure.ac
 |-- Makefile.am
 |-- README
 |-- NEWS
 |-- COPYING
 |-- AUTHORS
 |-- INSTALL
 +-- ChangeLog
```
---
