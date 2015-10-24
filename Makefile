PREFIX = ""
BIN = bin
SRC = src
LIB = lib
CC = gcc
CFLAGS = -Wall -I$(LIB) -lm

$(BIN)/flummitools: \
		$(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
		$(patsubst %.c, %.o, $(wildcard $(LIB)/*.c))
	@mkdir -p $(BIN)
	@echo " LINK $(BIN)/flummitools"
	@$(CC) $(CFLAGS) -o $(BIN)/flummitools \
		$(patsubst %.c, %.o, $(wildcard $(SRC)/*.c)) \
		$(patsubst %.c, %.o, $(wildcard $(LIB)/*.c))

$(SRC)/%.o: $(SRC)/%.c
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -o $@ -c $<

$(LIB)/%.o: $(LIB)/%.c
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -o $@ -c $<

all: $(BIN)/flummitools

install: all
	install -m 0755 $(BIN)/flummitools $(PREFIX)/usr/bin

uninstall:
	rm $(PREFIX)/usr/bin/flummitools

clean:
	rm -rf $(BIN) $(SRC)/*.o $(LIB)/*.o
