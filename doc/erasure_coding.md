Overview
========

Erasure coding allows the distribution of data across several independent
disks, improving data durability without requiring as much overhead as
high-replica replication. Data is broken into `k` data fragments, then
`k + m` fragments are calculated and stored. Given some `n ∈ [k, k+m)`
of these stored fragments, the original data can be reconstructed. Optimal
codes ensure that all subsets of `k` stored fragments can be used for
reconstruction.

Theory
======

Any [Reed-Solomon](https://en.wikipedia.org/wiki/Reed%E2%80%93Solomon_error_correction)
code uses linear algebra over a [Galois field](https://en.wikipedia.org/wiki/Finite_field).
The `k` data fragments are represented as a series of vectors and multiplied
by a `k × (k + m)` encoding matrix `E` to produce the `k + m` fragments for
storage. To decode a set of fragments `[f₁, f₂, ..., fₙ]`, select the
corresponding columns of `E` to create a `k × n` matrix `E′` then compute
the decoding matrix `D` as a left-inverse of `E′ᵀ ` (i.e., `D × E′ᵀ  = Iₖ`).
Multiply the fragments by `D` to recover the original data.

Note that for systematic encodings, the left-most `k × k` submatrix of `E` is `Iₖ`.

The encoding matrix `E` is typically based upon either
a [Vandermonde matrix](https://en.wikipedia.org/wiki/Vandermonde_matrix) or
a [Cauchy matrix](https://en.wikipedia.org/wiki/Cauchy_matrix).

The flat XOR codes eschew matrix inversion and multiplication (which are both
expensive) in favor of XOR-ing particular subsets of fragments together to
create parity fragments. For more information, see
"[Flat XOR-based erasure codes in storage systems: Constructions, efficient recovery, and tradeoffs](https://web.archive.org/web/20161001210233/https://www.kaymgee.com/Kevin_Greenan/Publications_files/greenan-msst10.pdf)".

Relevant Projects
=================

- [liberasurecode](https://opendev.org/openstack/liberasurecode/)

  The primary entrypoint, offering a unifying interface for multiple
  possible backends.

- [pyeclib](https://opendev.org/openstack/pyeclib/)

  Python bindings for liberasurecode.

- [isa-l](https://github.com/intel/isa-l/)

  Collection of optimized low-level functions for storage applications.
  Uses [multi-binary dispatch](https://github.com/intel/isa-l/blob/master/doc/functions.md#multi-binary-dispatchers)
  to offer optimized assembly to CPUs with a range of capabilities from
  a single binary. Notably, provides fast block Reed-Solomon type erasure
  codes for arbitrary encode/decode matrices as well as two functions for
  generating specific encoding matrices.

- [jerasure](https://github.com/ceph/jerasure/)

  First Reed-Solomon codes supported by liberasurecode. Requires gf-complete.
  Written by James Plank, who has since made [the original website](jerasure.org)
  read-only and [issued a notice](https://web.eecs.utk.edu/~jplank/plank/www/software.html)
  regarding claims of patent-infringement.

- [gf-complete](https://github.com/ceph/gf-complete/)

  Galois field library used by jerasure; also written by James Plank,
  also potentially patent-encumbered.

- shss

  Proprietary; developed by NTT. Requires additional data to be stored with
  every fragment.

- libphazr

  Proprietary; developed by Phazr.io. Requires additional data to be stored
  with every fragment.

Supported Backends
==================

Provided by liberasurecode
--------------------------
- `liberasurecode_rs_vand` (added in liberasurecode 1.0.8, pyeclib 1.0.8)
- `flat_xor_hd3`
- `flat_xor_hd4`

Provided by isa-l
-----------------
- `isa_l_rs_vand`

  Uses the Reed-Solomon functions provided by isa-l with
  [an encoding matrix also provided by isa-l](https://github.com/intel/isa-l/blob/v2.31.1/erasure_code/ec_base.c#L78-L96).
  Since this matrix is constructed by extending `Iₖ` with a `k × m` Vandermond
  matrix, a sufficient condition for optimality is that `m ≤ 4`; beyond that,
  some `k × k` submatrices may not be invertible.

  Prior to liberasurecode 1.3.0, it did not detect the failure to invert `E′ᵀ `,
  leading to incidents of data corruption. See [bug #1639691](https://bugs.launchpad.net/liberasurecode/+bug/1639691)
  for more information.

- `isa_l_rs_vand_inv` (added in liberasurecode 1.7.0, pyeclib 1.7.0)

  Uses the Reed-Solomon functions provided by isa-l with an encoding matrix
  provided by liberasurecode. To construct the encoding matrix, start with a
  `k × (k + m)` Vandermond matrix `V`, define `V′` as the left-most `k × k`
  submatrix, then calculate `E = inv(V′) × V`. This makes a systematic code
  that is optimal for all `k` and `m`.

- `isa_l_rs_cauchy` (added in liberasurecode 1.4.0, pyeclib 1.4.0)

  Uses the Reed-Solomon functions provided by isa-l with
  [an encoding matrix also provided by isa-l](https://github.com/intel/isa-l/blob/v2.31.1/erasure_code/ec_base.c#L78-L96).
  Being a Cauchy matrix, it forms an optimal code for all `k` and `m`.

Provided by jerasure
--------------------
- `jerasure_rs_vand`
- `jerasure_rs_cauchy`

Proprietary
-----------
- `shss` (added in liberasurecode 1.0.0, pyeclib 1.0.1)
- `libphazr` (added in liberasurecode 1.5.0, pyeclib 1.5.0)

Classifications
===============

Required Fragments
------------------
### n ≡ k

Most supported backends are optimal erasure codes, where any `k` fragments
are sufficient to recover the original data.

### n > k

The flat XOR codes require more than `k` fragments to decode in the general
case. In particular, `flat_xor_hd3` requires at least `n ≡ k + m - 2`
fragments and `flat_xor_hd4` requires at least `n ≡ k + m - 3`.

Systematic vs. Non-systematic
-----------------------------

[Systematic codes](https://en.wikipedia.org/wiki/Systematic_code) ensure that
the first `k` fragments for storage correspond to the initial `k` data
fragments. This can greatly speed up decoding when all `k` data fragments are
available as well as provide more recovery options in certain failure cases.

Non-systematic encodings do not ensure that. Rather, they often will seek to
ensure that *none* of the original data is directly present in the storage
fragments, thus ensuring confidentiality of data when less than `n` fragments
are available. See also: [secure secret sharing](https://en.wikipedia.org/wiki/Secret_sharing).

The following backends are non-systematic:

- `shss`
- `libphazr`

All other supported backends are systematic.
