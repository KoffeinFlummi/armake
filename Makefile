.RECIPEPREFIX +=

VERSION = 0.5.1
DESTDIR =
BIN = bin
SRC = src
LIB = lib
EXT =
CC = gcc
CFLAGS = -Wall -Wno-misleading-indentation -DVERSION=\"v$(VERSION)\" -std=gnu89 -ggdb
CLIBS = -I$(LIB) -lm -lcrypto

$(BIN)/armake: \
        $(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
        $(patsubst %.c, %.o, $(wildcard $(LIB)/*.c))
    @mkdir -p $(BIN)
    @echo " LINK $(BIN)/armake$(EXT)"
    @$(CC) $(CFLAGS) -o $(BIN)/armake$(EXT) \
        $(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
        $(patsubst %.c, %.o, $(wildcard $(LIB)/*.c)) \
        $(CLIBS)

$(SRC)/%.o: $(SRC)/%.c
    @echo "  CC  $<"
    @$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

$(LIB)/%.o: $(LIB)/%.c
    @echo "  CC  $<"
    @$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

test: $(BIN)/armake
    @./test/runall.sh

install: $(BIN)/armake
    mkdir -p $(DESTDIR)/usr/bin
    mkdir -p $(DESTDIR)/usr/share/bash-completion/completions
    mkdir -p $(DESTDIR)/etc/bash_completion.d
    install -m 0755 $(BIN)/armake $(DESTDIR)/usr/bin
    install -m 0644 completions/armake $(DESTDIR)/usr/share/bash-completion/completions
    install -m 0644 completions/armake $(DESTDIR)/etc/bash_completion.d

uninstall:
    rm $(DESTDIR)/usr/bin/armake

clean:
    rm -rf $(BIN) $(SRC)/*.o $(LIB)/*.o armake_*

win32:
    "$(MAKE)" CC=i686-w64-mingw32-gcc CLIBS="-I$(LIB) -lm -lcrypto -lole32 -lgdi32 -static" EXT=_w32.exe

win64:
    "$(MAKE)" CC=x86_64-w64-mingw32-gcc CLIBS="-I$(LIB) -lm -lcrypto -lole32 -lgdi32 -static" EXT=_w64.exe

docopt:
    mkdir tmp || rm -rf tmp/*
    git clone https://github.com/docopt/docopt.c tmp/docopt.c
    head -n 19 src/main.c > tmp/license
    python2 ./tmp/docopt.c/docopt_c.py -o tmp/docopt src/usage
    cat tmp/license > src/docopt.h
    printf "#pragma once\n\n\n" >> src/docopt.h
    grep -A 2 "#" tmp/docopt >> src/docopt.h
    printf "#define MAXEXCLUDEFILES 32\n" >> src/docopt.h
    printf "#define MAXINCLUDEFOLDERS 32\n" >> src/docopt.h
    printf "#define MAXWARNINGS 32\n\n\n" >> src/docopt.h
    grep -Pzo "(?s)typedef struct.*?\{.*?\} [a-zA-Z]*?;\n\n" tmp/docopt >> src/docopt.h
    sed -e 's/\x0//g' -i src/docopt.h # I don't know why I suddenly need this
    printf "\nDocoptArgs args;\n" >> src/docopt.h
    printf "char exclude_files[MAXEXCLUDEFILES][512];\n" >> src/docopt.h
    printf "char include_folders[MAXINCLUDEFOLDERS][512];\n" >> src/docopt.h
    printf "char muted_warnings[MAXWARNINGS][512];\n\n\n" >> src/docopt.h
    grep -E '^[a-zA-Z].*\(.*|^[^(]+\)' tmp/docopt >> src/docopt.h
    sed -Ei 's/\)\s*\{/);\n/' src/docopt.h
    cat tmp/license > src/docopt.c
    printf "#include \"docopt.h\"\n\n\n" >> src/docopt.c
    sed '/typedef struct/,/\} [a-zA-Z]*;/d' tmp/docopt >> src/docopt.c
    rm -rf tmp

# Use https://github.com/Infinidat/infi.docopt_completion
docopt-completion: $(BIN)/armake
    mkdir -p completions
    docopt-completion ./$(BIN)/armake --manual-bash
    mv armake.sh completions/armake
    docopt-completion ./$(BIN)/armake --manual-zsh
    mv _armake completions/_armake

debian: clean
    unexpand -t 4 Makefile | tail -c +3 > tmp
    mv tmp Makefile
    tar -czf ../armake_$(VERSION).orig.tar.gz .
    debuild -S -sa
    dput ppa:koffeinflummi/armake ../armake_*_source.changes

release:
    "$(MAKE)" clean
    mkdir armake_v$(VERSION)
    "$(MAKE)"
    rm */*.o
    "$(MAKE)" win32
    rm */*.o
    "$(MAKE)" win64
    rm */*.o
    mv bin/* armake_v$(VERSION)/
    zip -r armake_v$(VERSION).zip armake_v$(VERSION)

.PHONY: test debian release
