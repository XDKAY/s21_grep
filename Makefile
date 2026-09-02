CC = gcc
GCC_FLAGS = -Werror -Wall -Wextra -std=c11
TARGET = s21_grep
SRC = s21_grep.c
.PHONY: all clean rebuild test
all: $(TARGET)
$(TARGET): $(SRC) s21_grep.h
	$(CC) $(GCC_FLAGS) -o $(TARGET) $(SRC)
clean:
	rm -f $(TARGET)
rebuild: clean all

test:
	./test.sh
