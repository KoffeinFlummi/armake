armake
======

A C implementation of Arma modding tools (PAA conversion, binarization/rapification, PBO packing). (WIP)


### Setup

#### From Source

```
$ make
# make install
```

There are no dependencies other than a C lib with an fts library (like glibc) on \*nix systems.

#### Arch Linux

[PKGBUILD](https://aur.archlinux.org/packages/armake-git/)

Example (using pacaur):

```
$ pacaur -S armake-git
```


### ToDo List

- [x] Config rapification
- [x] PBO building
- [ ] P3D conversion (MLOD -> ODOL)
- [ ] RTM conversion

(some things might only be working on linux right now)


### Used Libraries

- [docopt](https://github.com/docopt/docopt.c)
- [MiniLZO](http://www.oberhumer.com/opensource/lzo/)
- [STB's image libraries](https://github.com/nothings/stb)
- [Paul E. Jones's SHA-1 implementation](https://www.packetizer.com/security/sha1/)


### Usage

See `$ armake --help` or [src/usage](https://github.com/KoffeinFlummi/armake/blob/master/src/usage).
