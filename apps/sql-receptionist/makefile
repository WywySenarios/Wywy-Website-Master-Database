CC = gcc
CFLAGS = -g -I/usr/include/postgresql -Iinclude
LDFLAGS = -L/usr/lib/x86_64-linux-gnu/ -lpq -lcyaml -ljansson

SRC := $(shell find src -name "*.c")
OBJ := $(patsubst src/%.c, obj/%.o, $(SRC))
TARGET = app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(TARGET) obj/