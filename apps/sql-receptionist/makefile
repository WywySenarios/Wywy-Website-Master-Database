SRC := $(shell find src -name "*.c")
CC = gcc
CFLAGS = -I/usr/include/postgresql -Iinclude
LDFLAGS = -L/usr/lib/x86_64-linux-gnu/ -g -lpq -lcyaml -ljansson
TARGET = app

app:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)