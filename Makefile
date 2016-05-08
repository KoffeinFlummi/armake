VERSION = 0.2.1
DESTDIR = ""
BIN = bin
SRC = src
LIB = lib
EXT = ""
CC = gcc
CFLAGS = -Wall -DVERSION=\"v$(VERSION)\" -std=gnu89 -ggdb
CLIBS = -I$(LIB) -lm

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

all: $(BIN)/armake

test: $(BIN)/armake FORCE
	@./test/runall.sh

install: all
	mkdir -p $(DESTDIR)/usr/bin
	install -m 0755 $(BIN)/armake $(DESTDIR)/usr/bin

uninstall:
	rm $(DESTDIR)/usr/bin/armake

clean:
	rm -rf $(BIN) $(SRC)/*.o $(LIB)/*.o

win32:
	make CC=i686-w64-mingw32-gcc EXT=.exe

win64:
	make CC=x86_64-w64-mingw32-gcc EXT=.exe

docopt:
	mkdir tmp || rm -rf tmp/*
	git clone https://github.com/docopt/docopt.c tmp/docopt.c
	head -n 19 src/main.c > tmp/license
	python2 ./tmp/docopt.c/docopt_c.py -o tmp/docopt src/usage
	cat tmp/license > src/docopt.h
	echo -e "#pragma once\n\n" >> src/docopt.h
	grep -A 2 "#" tmp/docopt >> src/docopt.h
	echo "#define MAXEXCLUDEFILES 32" >> src/docopt.h
	echo "#define MAXINCLUDEFOLDERS 32" >> src/docopt.h
	echo -e "#define MAXWARNINGS 32\n\n" >> src/docopt.h
	grep -Pzo "(?s)typedef struct.*?\{.*?\} [a-zA-Z]*?;\n\n" tmp/docopt >> src/docopt.h
	sed -ie 's/\x0//g' src/docopt.h # I don't know why I suddenly need this
	echo -e "\nDocoptArgs args;" >> src/docopt.h
	echo "char exclude_files[MAXEXCLUDEFILES][512];" >> src/docopt.h
	echo "char include_folders[MAXINCLUDEFOLDERS][512];" >> src/docopt.h
	echo -e "char muted_warnings[MAXWARNINGS][512];\n\n" >> src/docopt.h
	grep -E '^[a-zA-Z].*\(.*|^[^(]+\)' tmp/docopt >> src/docopt.h
	sed -Ei 's/\)\s*\{/);\n/' src/docopt.h
	cat tmp/license > src/docopt.c
	echo -e "#include \"docopt.h\"\n\n" >> src/docopt.c
	sed '/typedef struct/,/\} [a-zA-Z]*;/d' tmp/docopt >> src/docopt.c
	rm -rf tmp

debian: clean FORCE
	tar -czf ../armake_$(VERSION).orig.tar.gz .
	debuild -S -sa
	dput ppa:koffeinflummi/armake ../armake_*_source.changes

FORCE:
