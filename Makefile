.RECIPEPREFIX +=
%.c: %.y
%.c: %.l

VERSION = 0.6
DESTDIR =
BIN = bin
SRC = src
LIB = lib
EXT =
CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -Wno-misleading-indentation -DVERSION=\"v$(VERSION)\" -std=gnu89 -fPIC -ggdb
CLIBS = -I$(LIB) -lm -lcrypto

$(BIN)/armake: \
        $(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
        $(SRC)/rapify.tab.o $(SRC)/rapify.yy.o \
        $(patsubst %.c, %.o, $(wildcard $(LIB)/*.c))
    @mkdir -p $(BIN)
    @echo " LINK $(BIN)/armake$(EXT)"
    @$(CC) $(CFLAGS) -o $(BIN)/armake$(EXT) \
        $(patsubst %.c, %.o, $(filter-out $(SRC)/rapify.tab.c $(SRC)/rapify.yy.c, $(wildcard $(SRC)/*.c))) \
        $(SRC)/rapify.tab.o $(SRC)/rapify.yy.o \
        $(patsubst %.c, %.o, $(wildcard $(LIB)/*.c)) \
        $(CLIBS)

$(SRC)/rapify.tab.c: $(SRC)/rapify.y
    @echo " BISN $(SRC)/rapify.y"
    @$(BISON) -o $(SRC)/rapify.tab.c --defines=$(SRC)/rapify.tab.h $(SRC)/rapify.y

$(SRC)/rapify.yy.c: $(SRC)/rapify.l $(SRC)/rapify.tab.c
    @echo " FLEX $(SRC)/rapify.l"
    @$(FLEX) -w -o $(SRC)/rapify.yy.c $(SRC)/rapify.l

$(SRC)/rapify.tab.o: $(SRC)/rapify.tab.c
    @echo "  CC  $<"
    @$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

$(SRC)/rapify.yy.o: $(SRC)/rapify.yy.c
    @echo "  CC  $<"
    @$(CC) $(CFLAGS) -Wunused-function -o $@ -c $< $(CLIBS)

$(SRC)/%.o: $(SRC)/%.c $(SRC)/rapify.tab.c $(SRC)/rapify.yy.c
    @echo "  CC  $<"
    @$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

$(LIB)/%.o: $(LIB)/%.c
    @echo "  CC  $<"
    @$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

test: $(BIN)/armake
    @./test/run.sh

test-%: $(BIN)/armake
    @./test/run.sh $@

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
    rm -rf $(BIN) $(SRC)/*.o $(SRC)/*.tab.* $(SRC)/*.yy.c $(LIB)/*.o armake_*

win32:
    "$(MAKE)" CC=i686-w64-mingw32-gcc CLIBS="-I$(LIB) -lm -lcrypto -lws2_32 -lwsock32 -lole32 -lgdi32 -static" EXT=_w32.exe

win64:
    "$(MAKE)" CC=x86_64-w64-mingw32-gcc CLIBS="-I$(LIB) -lm -lcrypto -lws2_32 -lwsock32 -lole32 -lgdi32 -static" EXT=_w64.exe

# Use https://github.com/Infinidat/infi.docopt_completion
docopt-completion: $(BIN)/armake
    mkdir -p completions
    docopt-completion ./$(BIN)/armake --manual-bash
    mv armake.sh completions/armake
    docopt-completion ./$(BIN)/armake --manual-zsh
    mv _armake completions/_armake

debian: clean
    tar -czf ../armake_$(VERSION).orig.tar.gz .
    debuild -S -d -sa
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
