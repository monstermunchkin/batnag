CFLAGS = -Wall -Wextra -O2
PREFIX = /usr/local
BIN = batnag

all: $(BIN)

clean:
	rm -f *.o $(BIN)

install: $(BIN)
	install $< $(PREFIX)/bin

$(BIN): main.o
	$(CC) -o $@ $<

.PHONY: all clean install
