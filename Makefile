CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lncurses -lm

TARGET = fruit_ninja
SRC = fruit_ninja.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
