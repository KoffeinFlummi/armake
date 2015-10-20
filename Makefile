BIN=bin
SRC=src
LIB=lib
CC = gcc
CFLAGS  = -g -Wall -I$(LIB) -lm

all:  $(BIN)/img2paa

$(BIN)/img2paa:  $(SRC)/img2paa.c $(LIB)/minilzo.c bin
	$(CC) $(CFLAGS) -o $(BIN)/img2paa $(SRC)/img2paa.c $(LIB)/minilzo.c

clean:
	rm -rf $(BIN) $(SRC)/*.o $(LIB)/*.o

bin:
	mkdir -p $(BIN)
