liberasurecode
==============

liberasurecode is an Erasure Code API library written in C with pluggable Erasure Code backends.

----

Highlights
==========

 * Unified Erasure Coding interface for common storage workloads.

 * Pluggable Erasure Code backends - liberasurecode supports the following backends:

      - 'liberasurecode_rs_vand' - Native, software-only Erasure Coding implementation that supports a Reed-Solomon backend
      - 'Jerasure' - Erasure Coding library that supports Reed-Solomon, Cauchy backends [1]
      - 'ISA-L' - Intel Storage Acceleration Library - SIMD accelerated Erasure Coding backends [2]
      - 'SHSS' - NTT Lab Japan's hybrid Erasure Coding backend [4]
      - 'Flat XOR HD' - built-in to liberasurecode, based on [3]
      - 'libphazr' - Phazr.IO's erasure code backend with built-in privacy [5]
      - 'NULL' template backend implemented to help future backend writers


 * True 'plugin' architecture - liberasurecode uses Dynamically Loaded (DL)
   libraries to realize a true 'plugin' architecture.  This also allows one to
   build liberasurecode indepdendent of the Erasure Code backend libraries.

 * Cross-platform - liberasurecode is known to work on Linux (Fedora/Debian
   flavors), Solaris, BSD and Darwin/Mac OS X.

 * Community support - Developed alongside Erasure Code authority Kevin
   Greenan, liberasurecode is an actively maintained open-source project with
   growing community involvement (Openstack Swift, Ceph, PyECLib, NTT Labs).

----

Active Users
====================

 * PyECLib - Python EC library: https://github.com/openstack/pyeclib
 * Openstack Swift Object Store - https://wiki.openstack.org/wiki/Swift


----

Build and Install
=================

Install dependencies -

 Debian/Ubuntu hosts:

```sh
 $ sudo apt-get install build-essential autoconf automake libtool
```

 Fedora/RedHat/CentOS hosts:

```sh
 $ sudo yum install -y gcc make autoconf automake libtool
```

To build the liberasurecode repository, perform the following from the 
top-level directory:

``` sh
 $ ./autogen.sh
 $ ./configure
 $ make
 $ make test
 $ sudo make install
```

----

Getting Help
============

- Bugs: https://bugs.launchpad.net/liberasurecode/
- Mailing List: http://lists.openstack.org/pipermail/openstack-discuss/ (use tag `[swift][liberasurecode]`)
- IRC: #openstack-swift on OFTC

----

References
==========

 [1] Jerasure, C library that supports erasure coding in storage applications, http://jerasure.org

 [2] Intel(R) Storage Acceleration Library (Open Source Version), https://01.org/intel%C2%AE-storage-acceleration-library-open-source-version

 [3] Greenan, Kevin M et al, "Flat XOR-based erasure codes in storage systems", https://web.archive.org/web/20161001210233/https://www.kaymgee.com/Kevin_Greenan/Publications_files/greenan-msst10.pdf

 [4] Kota Tsuyuzaki <tsuyuzaki.kota@lab.ntt.co.jp>, "NTT SHSS Erasure Coding backend"

 [5] Jim Cheung <support@phazr.io>, "Phazr.IO libphazr erasure code backend with built-in privacy"
