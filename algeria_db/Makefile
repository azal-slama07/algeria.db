CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Iinclude
SRCS    = src/main.c src/file_parser.c src/linked_list.c \
          src/stack.c src/tree.c src/recursion.c
OBJS    = $(SRCS:.c=.o)
TARGET  = algeria_db

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) src/*.o

run: all
	./$(TARGET)

.PHONY: all clean run

test: $(TARGET)
	$(CC) $(CFLAGS) -o tests/test_runner tests/test_all.c \
	    src/file_parser.c src/linked_list.c src/stack.c src/tree.c src/recursion.c
	./tests/test_runner
