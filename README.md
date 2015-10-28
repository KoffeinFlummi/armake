flummitools
===========

(No that name is not final, leave me alone.)

This is my attempt to reimplement BI's arma tools in a cross-platform, open-source way. I'm fully aware that this might be a pipe dream, but why not.

I'm starting with PAA conversion to get my feet wet, not sure what to go for next.


### Used Libraries

- [docopt](https://github.com/docopt/docopt.c)
- [MiniLZO](http://www.oberhumer.com/opensource/lzo/)
- [STB's image libraries](https://github.com/nothings/stb)


### Sources

- https://community.bistudio.com/wiki/PAA_File_Format
- http://www.buckarooshangar.com/flightgear/tut_dds.html
- https://en.wikipedia.org/wiki/S3_Texture_Compression
- https://msdn.microsoft.com/en-us/library/bb694531(v=vs.85).aspx


### Usage

```
flummitools - Cause I suck at names.

Usage:
    flummitools img2paa [-f] [-z] [-t <paatype>] <source> <target>
    flummitools paa2img [-f] <source> <target>
    flummitools binarize <source> <target>
    flummitools debinarize <source> <target>
    flummitools build [-p] [-x <exclusions>] <source> <target>
    flummitools (-h | --help)
    flummitools (-v | --version)

Commands:
    img2paa      Convert image to PAA
    paa2img      Convert PAA to image
    binarize     Binarize a file
    debinarize   Debinarize a file
    build        Binarize and pack an addon folder

Options:
    -f --force      Overwrite the target file/folder if it already exists
    -z --compress   Compress final PAA where possible
    -t --type       PAA type. One of: DXT1, DXT2, DXT3, RGBA4444, RGBA5551, GRAY
    -x --exclude    Exclude the following types from packing
    -p --packonly   Don't binarize models, worlds and configs before packing
    -h --help       Show usage information and exit
    -v --version    Print the version number and exit
```
