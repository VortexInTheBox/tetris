CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lncurses

TARGET = tetris
SRC = tetris.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

format:
	clang-format -i $(SRC)

.PHONY: all clean run
