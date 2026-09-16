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

.PHONY: all clean run
