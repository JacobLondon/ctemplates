TARGET=cts
CC=tcc
CFLAGS=\
	-Wall \
	-Wextra \
	-Iinclude \
	-ggdb

FILES=$(wildcard src/*.c)

.PHONY: clean

$(TARGET): $(FILES)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(TARGET)
