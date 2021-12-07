libvl - Virtual Link Library
============================

Pre-requisites
--------------
1. An ISA extension (`DC.SVAC`, `DC.PSVAC`, `DC.FSVAC`),
which is supported in the
[gem5 implementation](https://github.com/jonathan-beard/near-data-sim.git);
2. Couple of system calls (`sys_mkvl()`, `sys_rmvl()`),
whose code is available in the subfolder `kernel` (TO BE WRITTEN).

Compile
-------
To get the library compiled:
~~~shell
export GEM5_ROOT=<path to local gem5 repo>
make
~~~
Note:
In the absence of gem5, pass `NOGEM5=true` to the `make` command,
so that the library would get compiled, but lack of functionality;
in the absence of the required system calls,
pass `NOSYSVL=true` to use the faked system calls as a workaround.

To test the library:
~~~shell
make test
~~~

To install the library:
~~~shell
make install
# by default will install the library to /usr/local
# change prefix in Makefile and pass DESTDIR to customize install location
~~~
