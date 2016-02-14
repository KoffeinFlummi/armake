PREFIX = ""
BIN = bin
SRC = src
LIB = lib
EXT = ""
CC = gcc
CFLAGS = -Wall -std=gnu99
CLIBS = -I$(LIB) -lm

$(BIN)/flummitools: \
		$(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
		$(patsubst %.c, %.o, $(wildcard $(LIB)/*.c))
	@mkdir -p $(BIN)
	@echo " LINK $(BIN)/flummitools$(EXT)"
	@$(CC) $(CFLAGS) -o $(BIN)/flummitools$(EXT) \
		$(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
		$(patsubst %.c, %.o, $(wildcard $(LIB)/*.c)) \
		$(CLIBS)

$(SRC)/%.o: $(SRC)/%.c
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

$(LIB)/%.o: $(LIB)/%.c
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -o $@ -c $< $(CLIBS)

all: $(BIN)/flummitools

install: all
	install -m 0755 $(BIN)/flummitools $(PREFIX)/usr/bin

uninstall:
	rm $(PREFIX)/usr/bin/flummitools

clean:
	rm -rf $(BIN) $(SRC)/*.o $(LIB)/*.o

win32: clean
	make CC=i686-w64-mingw32-gcc EXT=.exe

win64: clean
	make CC=x86_64-w64-mingw32-gcc EXT=.exe
