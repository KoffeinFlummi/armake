PREFIX = ""
BIN = bin
SRC = src
LIB = lib
CC = gcc
CFLAGS = -Wall -I$(LIB) -lm

$(BIN)/img2paa: $(SRC)/img2paa.c $(LIB)/minilzo.c bin
	$(CC) $(CFLAGS) -o $(BIN)/img2paa $(SRC)/img2paa.c $(LIB)/minilzo.c

all: $(BIN)/img2paa

install: all
	install -m 0755 $(BIN)/* $(PREFIX)/usr/bin

uninstall:
	rm $(PREFIX)/usr/bin/img2paa

clean:
	rm -rf $(BIN) $(SRC)/*.o $(LIB)/*.o

bin:
	mkdir -p $(BIN)
