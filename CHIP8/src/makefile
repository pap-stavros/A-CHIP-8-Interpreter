CC = g++
CFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

BUILD_DIR = ../build
EXEC = $(BUILD_DIR)/chip8_emulator

SRC = $(wildcard *.cpp)

all: $(EXEC)

$(EXEC): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
