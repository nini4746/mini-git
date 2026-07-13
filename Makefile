CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c11 -O2
LDLIBS   = -lz

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = minigit

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(BIN)
	bash tests/run.sh

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all test clean
